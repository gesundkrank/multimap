// This file is part of Multimap.  http://multimap.io
//
// Copyright (C) 2015-2016  Martin Trenkmann
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

#include <thread>
#include <type_traits>
#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/internal/Generator.hpp"
#include "multimap/internal/List.hpp"

namespace multimap {
namespace internal {

using testing::Eq;

// -----------------------------------------------------------------------------
// class List
// -----------------------------------------------------------------------------

TEST(ListTest, IsDefaultConstructible) {
  ASSERT_TRUE(std::is_default_constructible<List>::value);
}

TEST(ListTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_copy_constructible<List>::value);
  ASSERT_FALSE(std::is_copy_assignable<List>::value);
}

TEST(ListTest, IsNotMoveConstructibleOrAssignable) {
  ASSERT_FALSE(std::is_move_constructible<List>::value);
  ASSERT_FALSE(std::is_move_assignable<List>::value);
}

TEST(ListTest, DefaultConstructedHasProperState) {
  List list;
  const auto stats = list.getStatsUnlocked();
  ASSERT_THAT(stats.num_values_removed, Eq(0));
  ASSERT_THAT(stats.num_values_total, Eq(0));
  ASSERT_THAT(list.empty(), Eq(true));
  ASSERT_THAT(list.size(), Eq(0));
}

// -----------------------------------------------------------------------------
// class List / Iteration
// -----------------------------------------------------------------------------

struct ListTestIteration : testing::TestWithParam<uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap.ListTestIteration";
    boost::filesystem::remove_all(directory);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(directory));

    store.reset(new Store(directory / "store", Store::Options()));
  }

  void TearDown() override {
    store.reset();  // Destructor flushes all data to disk.
    MT_ASSERT_TRUE(boost::filesystem::remove_all(directory));
  }

  Store* getStore() { return store.get(); }
  Arena* getArena() { return &arena; }

 private:
  boost::filesystem::path directory;
  std::unique_ptr<Store> store;
  Arena arena;
};

TEST_P(ListTestIteration, AddSmallValuesAndIterateOnce) {
  List list;
  for (size_t i = 0; i != GetParam(); ++i) {
    list.append(std::to_string(i), getStore(), getArena());
    ASSERT_EQ(list.getStatsUnlocked().num_values_removed, 0);
    ASSERT_EQ(list.getStatsUnlocked().num_values_total, i + 1);
  }
  ASSERT_EQ(list.getStatsUnlocked().num_values_valid(), GetParam());

  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), std::to_string(i));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);
}

TEST_P(ListTestIteration, AddSmallValuesAndIterateTwice) {
  List list;
  for (size_t i = 0; i != GetParam(); ++i) {
    list.append(std::to_string(i), getStore(), getArena());
    ASSERT_EQ(list.getStatsUnlocked().num_values_removed, 0);
    ASSERT_EQ(list.getStatsUnlocked().num_values_total, i + 1);
  }
  ASSERT_EQ(list.getStatsUnlocked().num_values_valid(), GetParam());

  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), std::to_string(i));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);

  iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), std::to_string(i));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);
}

TEST_P(ListTestIteration, AddLargeValuesAndIterateOnce) {
  List list;
  SequenceGenerator generator;
  const size_t size = getStore()->getBlockSize() * 2.5;
  for (size_t i = 0; i != GetParam(); ++i) {
    list.append(generator.generate(size), getStore(), getArena());
    ASSERT_EQ(list.getStatsUnlocked().num_values_removed, 0);
    ASSERT_EQ(list.getStatsUnlocked().num_values_total, i + 1);
  }
  ASSERT_EQ(list.getStatsUnlocked().num_values_valid(), GetParam());

  generator.reset();
  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), generator.generate(size));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);
}

TEST_P(ListTestIteration, AddLargeValuesAndIterateTwice) {
  List list;
  SequenceGenerator generator;
  const size_t size = getStore()->getBlockSize() * 2.5;
  for (size_t i = 0; i != GetParam(); ++i) {
    list.append(generator.generate(size), getStore(), getArena());
    ASSERT_EQ(list.getStatsUnlocked().num_values_removed, 0);
    ASSERT_EQ(list.getStatsUnlocked().num_values_total, i + 1);
  }
  ASSERT_EQ(list.getStatsUnlocked().num_values_valid(), GetParam());

  generator.reset();
  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), generator.generate(size));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);

  generator.reset();
  iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), generator.generate(size));
  }
  ASSERT_FALSE(iter->hasNext());
  ASSERT_EQ(iter->available(), 0);
}

TEST_P(ListTestIteration, FlushValuesBetweenAddingThemAndIterate) {
  List list;
  const auto part_size = 1 + GetParam() / 5;
  for (size_t i = 0; i != GetParam(); ++i) {
    list.append(std::to_string(i), getStore(), getArena());
    if (list.size() % part_size == 0) {
      list.flush(getStore());
    }
  }

  auto iter = list.newIterator(*getStore());
  for (size_t i = 0; i != GetParam(); ++i) {
    ASSERT_TRUE(iter->hasNext());
    ASSERT_THAT(iter->next(), Eq(std::to_string(i)));
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, ListTestIteration,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

// -----------------------------------------------------------------------------
// class List / Concurrency
// -----------------------------------------------------------------------------

TEST(ListTest, ReaderDoesNotBlockReader) {
  List list;
  Store store;

  // First reader
  auto iter = list.newIterator(store);

  // Second reader
  bool second_reader_has_finished = false;
  std::thread([&] {
    auto iter = list.newIterator(store);
    second_reader_has_finished = true;
  }).join();

  ASSERT_TRUE(second_reader_has_finished);
}

TEST(ListTest, ReaderDoesNotBlockReader2) {
  List list;
  Store store;

  // First reader
  auto iter = list.newIterator(store);

  // Second readers
  List::Stats stats;
  ASSERT_TRUE(list.tryGetStats(&stats));
  ASSERT_THAT(list.size(), Eq(0));
  ASSERT_TRUE(list.empty());
}

TEST(ListTest, ReaderBlocksWriter) {
  List list;
  Store store;

  // Reader
  auto iter = list.newIterator(store);

  // Writer
  bool writer_has_finished = false;
  std::thread writer([&] {
    Arena arena;
    list.append("value", &store, &arena);
    writer_has_finished = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(writer_has_finished);

  iter.reset();  // Releases reader lock

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(writer_has_finished);
  writer.join();
}

TEST(ListTest, WriterBlocksReader) {
  List list;
  Arena arena;
  Store store;
  list.append("value", &store, &arena);

  // Writer
  std::thread writer([&] {
    list.removeOne([](const Bytes& /* value */) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      return false;
    }, &store);
  });

  // Reader
  bool reader_has_finished = false;
  std::thread reader([&] {
    auto iter = list.newIterator(store);
    reader_has_finished = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(reader_has_finished);

  writer.join();  // Wait for release of writer lock

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(reader_has_finished);
  reader.join();
}

TEST(ListTest, WriterBlocksReader2) {
  List list;
  Arena arena;
  Store store;
  list.append("value", &store, &arena);

  // Writer
  std::thread writer([&] {
    list.removeOne([](const Bytes& /* value */) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      return false;
    }, &store);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Reader
  List::Stats stats;
  ASSERT_FALSE(list.tryGetStats(&stats));

  writer.join();  // Wait for release of writer lock

  ASSERT_TRUE(list.tryGetStats(&stats));
}

TEST(ListTest, WriterBlocksWriter) {
  List list;
  Arena arena;
  Store store;
  list.append("value", &store, &arena);

  // First writer
  std::thread writer1([&] {
    const auto removed = list.removeOne([](const Bytes& /* value */) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      return false;
    }, &store);
    ASSERT_FALSE(removed);
  });

  // Second writer
  bool second_writer_has_finished = false;
  std::thread writer2([&] {
    const auto removed =
        list.removeOne([](const Bytes& /* value */) { return false; }, &store);
    ASSERT_FALSE(removed);
    second_writer_has_finished = true;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_FALSE(second_writer_has_finished);

  writer1.join();  // Wait for release of first writer lock

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_TRUE(second_writer_has_finished);
  writer2.join();
}

struct ListIteratorTestWithParam : testing::TestWithParam<uint32_t> {
  void SetUp() override {
    boost::filesystem::remove_all(dir);
    MT_ASSERT_TRUE(boost::filesystem::create_directory(dir));

    Store::Options options;
    store.reset(new Store(boost::filesystem::path(dir) / "store", options));

    for (size_t i = 0; i != GetParam(); ++i) {
      list.append(std::to_string(i), store.get(), &arena);
    }
  }

  void TearDown() override { boost::filesystem::remove_all(dir); }

  std::unique_ptr<Iterator> newIterator() { return list.newIterator(*store); }

  const std::string dir = "/tmp/multimap.ListIteratorTestWithParam";
  const std::string key = "key";
  std::unique_ptr<Store> store;
  Arena arena;
  List list;
};

TEST_P(ListIteratorTestWithParam, Iterate) {
  auto iter = newIterator();
  ASSERT_EQ(iter->available(), GetParam());
  for (size_t i = 0; iter->hasNext(); ++i) {
    ASSERT_EQ(iter->available(), GetParam() - i);
    ASSERT_EQ(iter->next(), std::to_string(i));
  }
  ASSERT_EQ(iter->available(), 0);
  ASSERT_FALSE(iter->hasNext());
}

INSTANTIATE_TEST_CASE_P(Parameterized, ListIteratorTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));

// TODO Move that into List::remove() unit tests
/*
TEST_P(ExclusiveListIteratorTestWithParam, IterateOnceAndRemoveEvery23thValue) {
  auto iter = getIterator();
  size_t num_removed = 0;
  for (size_t i = 0; iter->hasNext(); ++i) {
    ASSERT_EQ(iter->next(), std::to_string(i));
    if (i % 23 == 0) {
      iter->remove();
      ++num_removed;
    }
  }
  ASSERT_EQ(iter->available(), 0);
  ASSERT_FALSE(iter->hasNext());
}

TEST_P(ExclusiveListIteratorTestWithParam,
       IterateTwiceAndRemoveEvery23thValueIn1stRun) {
  size_t num_removed = 0;
  {
    auto iter = getIterator();
    for (size_t i = 0; iter->hasNext(); ++i) {
      iter->next();
      if (i % 23 == 0) {
        iter->remove();
        ++num_removed;
      }
    }
    ASSERT_EQ(iter->available(), 0);
    ASSERT_FALSE(iter->hasNext());
  }
  {
    auto iter = getIterator();
    ASSERT_EQ(iter->available(), GetParam() - num_removed);
    for (size_t i = 0; iter->hasNext(); ++i) {
      if (i % 23 != 0) {
        ASSERT_NE(std::stoi(iter->next().toString()) % 23, 0);
      }
    }
    ASSERT_EQ(iter->available(), 0);
    ASSERT_FALSE(iter->hasNext());
  }
}

INSTANTIATE_TEST_CASE_P(Parameterized, ExclusiveListIteratorTestWithParam,
                        testing::Values(0, 1, 2, 10, 100, 1000, 1000000));
*/

}  // namespace internal
}  // namespace multimap
