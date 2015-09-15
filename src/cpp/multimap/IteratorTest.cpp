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

#include <type_traits>
#include <boost/filesystem/operations.hpp>
#include "gmock/gmock.h"
#include "multimap/internal/System.hpp"
#include "multimap/Map.hpp"

namespace multimap {
namespace internal {

using testing::Eq;
using testing::Ne;

typedef Iterator<false> Iter;
typedef Iterator<true> ConstIter;

struct IteratorTestFixture : testing::TestWithParam<std::uint32_t> {
  void SetUp() override {
    directory = "/tmp/multimap-IteratorTestFixture";
    boost::filesystem::remove_all(directory);
    assert(boost::filesystem::create_directory(directory));

    Options options;
    options.create_if_missing = true;
    map.reset(new Map(directory, options));

    key = "key";
    for (std::size_t i = 0; i != GetParam(); ++i) {
      map->put(key, std::to_string(i));
    }
  }

  void TearDown() override {
    map.reset();  // We need the d'tor call here.
    assert(boost::filesystem::remove_all(directory));
  }

  std::string key;
  std::unique_ptr<Map> map;
  boost::filesystem::path directory;
};

struct ConstIterTest : public IteratorTestFixture {
  ConstIter GetIterator() { return map->get(key); }
};

struct IterTest : public IteratorTestFixture {
  Iter GetIterator() { return map->getMutable(key); }
};

TEST(IterTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<Iter>::value, Eq(true));
}

TEST(IterTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<Iter>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<Iter>::value, Eq(false));
}

TEST(IterTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<Iter>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<Iter>::value, Eq(true));
}

TEST(IterTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(Iter().hasValue(), Eq(false));
  ASSERT_THAT(Iter().num_values(), Eq(0));
}

TEST_P(IterTest, SeekToFirstPerformsInitialization) {
  Iter iter = GetIterator();
  ASSERT_THAT(iter.hasValue(), Eq(false));
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));

  iter.seekToFirst();
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  if (iter.num_values() == 0) {
    ASSERT_THAT(iter.hasValue(), Eq(false));
  } else {
    ASSERT_THAT(iter.hasValue(), Eq(true));
    ASSERT_THAT(iter.getValue(), Eq("0"));
  }
}

TEST_P(IterTest, SeekToTargetPerformsInitialization) {
  Iter iter = GetIterator();
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  ASSERT_THAT(iter.hasValue(), Eq(false));

  iter.seekTo(Bytes("0"));
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  if (iter.num_values() == 0) {
    ASSERT_THAT(iter.hasValue(), Eq(false));
  } else {
    ASSERT_THAT(iter.hasValue(), Eq(true));
    ASSERT_THAT(iter.getValue(), Eq("0"));
  }
}

TEST_P(IterTest, SeekToPredicatePerformsInitialization) {
  Iter iter = GetIterator();
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  ASSERT_THAT(iter.hasValue(), Eq(false));

  iter.seekTo([](const Bytes& value) { return value == Bytes("0"); });
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  if (iter.num_values() == 0) {
    ASSERT_THAT(iter.hasValue(), Eq(false));
  } else {
    ASSERT_THAT(iter.hasValue(), Eq(true));
    ASSERT_THAT(iter.getValue(), Eq("0"));
  }
}

TEST_P(IterTest, IterateAllUntilSize) {
  Iter iter = GetIterator();
  iter.seekToFirst();
  for (std::size_t i = 0; i != iter.num_values(); ++i) {
    ASSERT_THAT(iter.num_values(), Eq(GetParam()));
    ASSERT_THAT(iter.hasValue(), Eq(true));
    ASSERT_THAT(iter.getValue(), Eq(std::to_string(i)));
    iter.next();
  }
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  ASSERT_THAT(iter.hasValue(), Eq(false));
}

TEST_P(IterTest, IterateAllWhileValid) {
  Iter iter = GetIterator();
  std::size_t i = 0;
  for (iter.seekToFirst(); iter.hasValue(); iter.next(), ++i) {
    ASSERT_THAT(iter.num_values(), Eq(GetParam()));
    ASSERT_THAT(iter.getValue(), Eq(std::to_string(i)));
  }
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  ASSERT_THAT(iter.hasValue(), Eq(false));
}

TEST_P(IterTest, IterateOnceAndDeleteEvery23thValue) {
  Iter iter = GetIterator();
  std::size_t i = 0;
  std::size_t num_deleted = 0;
  for (iter.seekToFirst(); iter.hasValue(); iter.next(), ++i) {
    ASSERT_THAT(iter.getValue(), Eq(std::to_string(i)));

    if (i % 23 == 0) {
      iter.markAsDeleted();
      ++num_deleted;
      ASSERT_THAT(iter.num_values(), Eq(GetParam() - num_deleted));
      ASSERT_THAT(iter.hasValue(), Eq(false));
    }
  }
  ASSERT_THAT(iter.num_values(), Eq(GetParam() - num_deleted));
  ASSERT_THAT(iter.hasValue(), Eq(false));
}

TEST_P(IterTest, IterateTwiceAndDeleteEvery23thValueIn1stRun) {
  Iter iter = GetIterator();
  std::size_t i = 0;
  std::size_t num_deleted = 0;
  for (iter.seekToFirst(); iter.hasValue(); iter.next(), ++i) {
    if (i % 23 == 0) {
      iter.markAsDeleted();
      ++num_deleted;
    }
  }
  ASSERT_THAT(iter.num_values(), Eq(GetParam() - num_deleted));

  for (iter.seekToFirst(); iter.hasValue(); iter.next()) {
    ASSERT_THAT(stoi(iter.getValue().toString()) % 23, Ne(0));
  }
  ASSERT_THAT(iter.num_values(), Eq(GetParam() - num_deleted));
  ASSERT_THAT(iter.hasValue(), Eq(false));
}

TEST(ConstIterTest, IsDefaultConstructible) {
  ASSERT_THAT(std::is_default_constructible<ConstIter>::value, Eq(true));
}

TEST(ConstIterTest, IsNotCopyConstructibleOrAssignable) {
  ASSERT_THAT(std::is_copy_constructible<ConstIter>::value, Eq(false));
  ASSERT_THAT(std::is_copy_assignable<ConstIter>::value, Eq(false));
}

TEST(ConstIterTest, IsMoveConstructibleAndAssignable) {
  ASSERT_THAT(std::is_move_constructible<ConstIter>::value, Eq(true));
  ASSERT_THAT(std::is_move_assignable<ConstIter>::value, Eq(true));
}

TEST(ConstIterTest, DefaultConstructedHasProperState) {
  ASSERT_THAT(Iter().hasValue(), Eq(false));
  ASSERT_THAT(Iter().num_values(), Eq(0));
}

TEST_P(ConstIterTest, SeekToFirstPerformsInitialization) {
  ConstIter iter = GetIterator();
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  ASSERT_THAT(iter.hasValue(), Eq(false));

  iter.seekToFirst();
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  if (iter.num_values() == 0) {
    ASSERT_THAT(iter.hasValue(), Eq(false));
  } else {
    ASSERT_THAT(iter.hasValue(), Eq(true));
    ASSERT_THAT(iter.getValue(), Eq("0"));
  }
}

TEST_P(ConstIterTest, SeekToTargetPerformsInitialization) {
  ConstIter iter = GetIterator();
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  ASSERT_THAT(iter.hasValue(), Eq(false));

  iter.seekTo(Bytes("0"));
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  if (iter.num_values() == 0) {
    ASSERT_THAT(iter.hasValue(), Eq(false));
  } else {
    ASSERT_THAT(iter.hasValue(), Eq(true));
    ASSERT_THAT(iter.getValue(), Eq("0"));
  }
}

TEST_P(ConstIterTest, SeekToPredicatePerformsInitialization) {
  ConstIter iter = GetIterator();
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  ASSERT_THAT(iter.hasValue(), Eq(false));

  iter.seekTo([](const Bytes& value) { return value == Bytes("0"); });
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  if (iter.num_values() == 0) {
    ASSERT_THAT(iter.hasValue(), Eq(false));
  } else {
    ASSERT_THAT(iter.hasValue(), Eq(true));
    ASSERT_THAT(iter.getValue(), Eq("0"));
  }
}

TEST_P(ConstIterTest, IterateAllUntilSize) {
  ConstIter iter = GetIterator();
  iter.seekToFirst();
  for (std::size_t i = 0; i != iter.num_values(); ++i) {
    ASSERT_THAT(iter.num_values(), Eq(GetParam()));
    ASSERT_THAT(iter.hasValue(), Eq(true));
    ASSERT_THAT(iter.getValue(), Eq(std::to_string(i)));
    iter.next();
  }
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  ASSERT_THAT(iter.hasValue(), Eq(false));
}

TEST_P(ConstIterTest, IterateAllWhileValid) {
  ConstIter iter = GetIterator();
  std::size_t i = 0;
  for (iter.seekToFirst(); iter.hasValue(); iter.next(), ++i) {
    ASSERT_THAT(iter.num_values(), Eq(GetParam()));
    ASSERT_THAT(iter.getValue(), Eq(std::to_string(i)));
  }
  ASSERT_THAT(iter.num_values(), Eq(GetParam()));
  ASSERT_THAT(iter.hasValue(), Eq(false));
}

INSTANTIATE_TEST_CASE_P(Parameterized, IterTest,
                        testing::Values(0, 1, 2, 10, 100, 1000, 10000));

INSTANTIATE_TEST_CASE_P(Parameterized, ConstIterTest,
                        testing::Values(0, 1, 2, 10, 100, 1000, 10000));

// TODO Micro-benchmark this.
// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, IterTest,
//                        testing::Values(100000, 1000000));

// TODO Micro-benchmark this.
// INSTANTIATE_TEST_CASE_P(ParameterizedLongRunning, ConstIterTest,
//                        testing::Values(100000, 1000000));

}  // namespace internal
}  // namespace multimap
