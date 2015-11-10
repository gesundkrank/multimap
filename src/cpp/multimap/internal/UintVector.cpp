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

#include "multimap/internal/UintVector.hpp"

#include <cassert>
#include <cstring>
#include "multimap/internal/System.hpp"

namespace multimap {
namespace internal {

namespace {

std::size_t readUint32(const char* source, std::uint32_t* target) {
  std::memcpy(target, source, sizeof *target);
  return sizeof *target;
}

std::size_t writeUint32(std::uint32_t source, char* target) {
  std::memcpy(target, &source, sizeof source);
  return sizeof source;
}

} // namespace

UintVector::UintVector(const UintVector& other)
    : offset_(other.offset_), size_(other.size_) {
  if (other.data_) {
    data_.reset(new char[size_]);
    std::memcpy(data_.get(), other.data_.get(), size_);
  }
}

UintVector& UintVector::operator=(const UintVector& other) {
  if (&other != this) {
    size_ = other.size_;
    offset_ = other.offset_;
    data_.reset(new char[size_]);
    std::memcpy(data_.get(), other.data_.get(), size_);
  }
  return *this;
}

UintVector UintVector::readFromStream(std::FILE* stream) {
  UintVector vector;
  System::read(stream, &vector.offset_, sizeof vector.offset_);
  vector.data_.reset(new char[vector.offset_]);
  System::read(stream, vector.data_.get(), vector.offset_);
  vector.size_ = vector.offset_;
  return vector;
}

void UintVector::writeToStream(std::FILE* stream) const {
  assert(!empty());
  System::write(stream, &offset_, sizeof offset_);
  System::write(stream, data_.get(), offset_);
}

std::vector<std::uint32_t> UintVector::unpack() const {
  std::vector<std::uint32_t> values;
  if (!empty()) {
    std::uint32_t delta = 0;
    std::uint32_t value = 0;
    std::uint32_t nbytes = 0;
    std::uint32_t offset = 0;
    std::uint32_t remaining = offset_ - sizeof value;
    while (remaining > 0) {
      nbytes = Varint::readUint(data_.get() + offset, remaining, &delta);
      offset += nbytes;
      remaining -= nbytes;
      values.push_back(value + delta);
      value = values.back();
    }
  }
  return values;
}

bool UintVector::add(std::uint32_t value) {
  allocateMoreIfFull();
  if (empty()) {
    if (value <= Varint::Limits::N4_MAX_UINT) {
      offset_ += Varint::writeUint(value, current(), size_);
      offset_ += writeUint32(value, current());
      return true;
    }
  } else {
    std::uint32_t prev_value;
    offset_ -= sizeof prev_value;
    readUint32(current(), &prev_value);
    MT_ASSERT_LT(prev_value, value);
    const std::uint32_t delta = value - prev_value;
    if (delta <= Varint::Limits::N4_MAX_UINT) {
      offset_ += Varint::writeUint(delta, current(), remaining());
      offset_ += writeUint32(value, current());
      return true;
    }
  }
  return false;
}

void UintVector::allocateMoreIfFull() {
  const auto required_size = sizeof(std::uint32_t) * 2;
  if (size_ - offset_ < required_size) {
    const std::size_t new_end_offset = size_ * 1.5;
    const auto new_size = std::max(new_end_offset, required_size);
    std::unique_ptr<char[]> new_data(new char[new_size]);
    std::memcpy(new_data.get(), data_.get(), offset_);
    data_ = std::move(new_data);
    size_ = new_size;
  }
}

} // namespace internal
} // namespace multimap
