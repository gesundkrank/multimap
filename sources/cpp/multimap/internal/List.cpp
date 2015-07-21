// This file is part of the Multimap library.  http://multimap.io
//
// Copyright (C) 2015  Martin Trenkmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "multimap/internal/List.hpp"
#include "multimap/internal/BlockPool.hpp"

namespace multimap {
namespace internal {

std::mutex List::dynamic_mutex_protector;

List::Head List::Head::ReadFromStream(std::istream& stream) {
  Head head;
  stream.read(reinterpret_cast<char*>(&head.num_values_total),
              sizeof head.num_values_total);
  assert(stream.good());
  stream.read(reinterpret_cast<char*>(&head.num_values_deleted),
              sizeof head.num_values_deleted);
  assert(stream.good());
  head.block_ids = UintVector::ReadFromStream(stream);
  return head;
}

void List::Head::WriteToStream(std::ostream& stream) const {
  stream.write(reinterpret_cast<const char*>(&num_values_total),
               sizeof num_values_total);
  assert(stream.good());
  stream.write(reinterpret_cast<const char*>(&num_values_deleted),
               sizeof num_values_deleted);
  assert(stream.good());
  block_ids.WriteToStream(stream);
}

template <>
List::Iter<true>::Iter(const Head& head, const Block& block,
                       const Callbacks& callbacks)
    : head_(&head),
      block_(&block),
      block_ids_(head_->block_ids.Unpack()),
      callbacks_(callbacks) {
  assert(callbacks_.allocate_block);
  assert(callbacks_.deallocate_block);
  assert(callbacks_.request_block);
}

template <>
List::Iter<false>::Iter(Head* head, Block* block, const Callbacks& callbacks)
    : head_(head),
      block_(block),
      block_ids_(head_->block_ids.Unpack()),
      callbacks_(callbacks) {
  assert(head_);
  assert(block_);
  assert(callbacks_.allocate_block);
  assert(callbacks_.deallocate_block);
  assert(callbacks_.request_block);
  assert(callbacks_.update_block);
}

template <>
void List::Iter<false>::Delete() {
  assert(Valid());
  stats_.block_has_changed |= !block_iter_.deleted();
  block_iter_.set_deleted();
  ++head_->num_values_deleted;
}

template <>
void List::Iter<false>::Advance() {
  block_iter_.advance();
  if (!block_iter_.has_value()) {
    if (stats_.block_has_changed) {
      UpdateCurrentBlock();
    }
    RequestNextBlockAndInitIter();
  }
}

template <>
void List::Iter<false>::UpdateCurrentBlock() {
  if (stats_.block_id_index < block_ids_.size()) {
    const auto block_id = block_ids_[stats_.block_id_index];
    callbacks_.update_block(block_id, requested_block_);
  }
  // block_ is in-memory and therefore updated in-place.
}

List::List(const Head& head) : head_(head) {}

void List::Add(const Bytes& value,
               const Callbacks::AllocateBlock& allocate_block,
               const Callbacks::CommitBlock& commit_block) {
  if (!block_.has_data()) {
    block_ = allocate_block();
    std::memset(block_.data(), 0, block_.size());
  }
  auto ok = block_.TryAdd(value);
  if (!ok) {
    head_.block_ids.Add(commit_block(std::move(block_)));
    block_ = allocate_block();
    std::memset(block_.data(), 0, block_.size());
    ok = block_.TryAdd(value);
    assert(ok);
  }
  ++head_.num_values_total;
}

void List::Flush(const Callbacks::CommitBlock& commit_block) {
  if (block_.has_data()) {
    head_.block_ids.Add(commit_block(std::move(block_)));
    block_ = Block();
  }
}

List::Iterator List::NewIterator(const Callbacks& callbacks) {
  return Iterator(&head_, &block_, callbacks);
}

List::ConstIterator List::NewIterator(const Callbacks& callbacks) const {
  return ConstIterator(head_, block_, callbacks);
}

List::ConstIterator List::NewConstIterator(const Callbacks& callbacks) const {
  return ConstIterator(head_, block_, callbacks);
}

List::Head List::Copy(const Head& head, const DataFile& from, DataFile* to) {
  return Copy(head, from, to, Compare());
}

List::Head List::Copy(const Head& head, const DataFile& from, DataFile* to,
                      const Compare& compare) {
  const auto block_pool =
      BlockPool::Create(DataFile::kMaxBufferSize, to->block_size());

  Callbacks callbacks;
  callbacks.allocate_block = [&block_pool, to]() {
    auto block = block_pool->Pop();
    if (!block.has_data()) {
      to->Flush();
    }
    block = block_pool->Pop();
    assert(block.has_data());
    return block;
  };

  callbacks.deallocate_blocks =
      [&block_pool](std::vector<Block>* blocks) { block_pool->Push(blocks); };

  callbacks.commit_block =
      [to](Block&& block) { return to->Append(std::move(block)); };

  callbacks.request_block = [&from](
      std::uint32_t block_id, Block* block) { from.Read(block_id, block); };

  to->set_deallocate_blocks(callbacks.deallocate_blocks);

  List new_list;
  auto iter = List(head).NewIterator(callbacks);
  for (iter.SeekToFirst(); iter.Valid(); iter.Next()) {
    new_list.Add(iter.Value(), callbacks.allocate_block,
                 callbacks.commit_block);
  }
  new_list.Flush(callbacks.commit_block);
  return new_list.head();
}

void List::LockShared() const {
  {
    const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
    CreateMutexUnlocked();
    ++mutex_use_count_;
  }
  mutex_->lock_shared();
}

void List::LockUnique() const {
  {
    const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
    CreateMutexUnlocked();
    ++mutex_use_count_;
  }
  mutex_->lock();
}

bool List::TryLockShared() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
  CreateMutexUnlocked();
  const auto locked = mutex_->try_lock_shared();
  if (locked) {
    ++mutex_use_count_;
  }
  return locked;
}

bool List::TryLockUnique() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
  CreateMutexUnlocked();
  const auto locked = mutex_->try_lock();
  if (locked) {
    ++mutex_use_count_;
  }
  return locked;
}

void List::UnlockShared() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
  assert(mutex_use_count_ > 0);
  mutex_->unlock_shared();
  --mutex_use_count_;
  if (mutex_use_count_ == 0) {
    DeleteMutexUnlocked();
  }
}

void List::UnlockUnique() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
  assert(mutex_use_count_ > 0);
  mutex_->unlock();
  --mutex_use_count_;
  if (mutex_use_count_ == 0) {
    DeleteMutexUnlocked();
  }
}

bool List::locked() const {
  const std::lock_guard<std::mutex> lock(dynamic_mutex_protector);
  return mutex_use_count_ != 0;
}

void List::CreateMutexUnlocked() const {
  if (!mutex_) {
    mutex_.reset(new Mutex());
  }
}

void List::DeleteMutexUnlocked() const {
  assert(mutex_use_count_ == 0);
  mutex_.reset();
}

}  // namespace internal
}  // namespace multimap
