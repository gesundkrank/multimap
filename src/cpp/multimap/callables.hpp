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

#ifndef MULTIMAP_CALLABLES_HPP_INCLUDED
#define MULTIMAP_CALLABLES_HPP_INCLUDED

#include <algorithm>
#include "multimap/Bytes.hpp"

namespace multimap {

// typedef std::function<bool(const Bytes&)> Predicate;
// Types implementing this interface can process a value and return a
// boolean. Predicates check a value for certain property and thus,
// depending on the outcome, can be used to control the path of execution.

// typedef std::function<void(const Bytes&)> Procedure;
// Types implementing this interface can process a value, but do not return
// a result. However, since objects of this type may have state, a procedure
// can be used to collect information about the processed data, and thus
// returning a result indirectly.

// typedef internal::SharedListIterator ListIterator;
// typedef std::function<void(const Bytes&, ListIterator&&)> BinaryProcedure;

// typedef std::function<std::string(const Bytes&)> Function;
// Types implementing this interface can process a value and return a new
// one. Functions map an input value to an output value. An empty or no
// result can be signaled returning an empty string. std::string is used
// here as a convenient byte buffer that may contain arbitrary bytes.

// typedef std::function<bool(const Bytes&, const Bytes&)> Compare;
// Types implementing this interface can process two values and return a
// boolean. Such functions determine the less than order of the given values
// according to the Compare concept.
// See http://en.cppreference.com/w/cpp/concept/Compare

// -----------------------------------------------------------------------------
// Predicates
// -----------------------------------------------------------------------------

struct Equal {
  explicit Equal(const Bytes& value) : value_(value) {}

  bool operator()(const Bytes& value) const { return value == value_; }

 private:
  const Bytes value_;
};

struct Contains {
  // Containment check for empty string is equivalent to std::string::find.
  // More precisely:
  //  - Contains("")("")    -> true, because std::string("").find("")    == 0
  //  - Contains("")("abc") -> true, because std::string("abc").find("") == 0

  explicit Contains(const Bytes& value) : value_(value) {}

  bool operator()(const Bytes& value) const {
    return (value_.size() > 0)
               ? std::search(value.begin(), value.end(), value_.begin(),
                             value_.end()) != value.end()
               : true;
  }

 private:
  const Bytes value_;
};

struct StartsWith {
  explicit StartsWith(const Bytes& value) : value_(value) {}

  bool operator()(const Bytes& value) const {
    return (value.size() >= value_.size())
               ? std::memcmp(value.data(), value_.data(), value_.size()) == 0
               : false;
  }

 private:
  const Bytes value_;
};

struct EndsWith {
  explicit EndsWith(const Bytes& value) : value_(value) {}

  bool operator()(const Bytes& value) const {
    const int diff = value.size() - value_.size();
    return (diff >= 0)
               ? std::memcmp(value.data() + diff, value_.data(),
                             value_.size()) == 0
               : false;
  }

 private:
  const Bytes value_;
};

}  // namespace multimap

#endif  // MULTIMAP_CALLABLES_HPP_INCLUDED