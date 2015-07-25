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

#ifndef MULTIMAP_MAP_HPP
#define MULTIMAP_MAP_HPP

#include <map>
#include <memory>
#include <string>
#include <boost/filesystem/path.hpp>
#include "multimap/Options.hpp"
#include "multimap/Iterator.hpp"
#include "multimap/internal/BlockPool.hpp"
#include "multimap/internal/Callbacks.hpp"
#include "multimap/internal/DataFile.hpp"
#include "multimap/internal/Table.hpp"

namespace multimap {

class Map {
 public:
  typedef Iterator<false> Iter;
  typedef Iterator<true> ConstIter;

  ~Map();

  static std::unique_ptr<Map> Open(const boost::filesystem::path& directory,
                                   const Options& options);

  void Put(const Bytes& key, const Bytes& value);

  ConstIter Get(const Bytes& key) const;

  Iter GetMutable(const Bytes& key);

  bool Contains(const Bytes& key) const;

  std::size_t Delete(const Bytes& key);

  typedef std::function<bool(const Bytes&)> ValuePredicate;

  bool DeleteFirst(const Bytes& key, ValuePredicate predicate);

  bool DeleteFirstEqual(const Bytes& key, const Bytes& value);

  std::size_t DeleteAll(const Bytes& key, ValuePredicate predicate);

  std::size_t DeleteAllEqual(const Bytes& key, const Bytes& value);

  typedef std::function<std::string(const Bytes&)> ValueFunction;

  bool ReplaceFirst(const Bytes& key, ValueFunction function);

  bool ReplaceFirstEqual(const Bytes& key, const Bytes& old_value,
                         const Bytes& new_value);

  std::size_t ReplaceAll(const Bytes& key, ValueFunction function);

  std::size_t ReplaceAllEqual(const Bytes& key, const Bytes& old_value,
                              const Bytes& new_value);

  typedef std::function<void(const Bytes&)> KeyProcedure;

  void ForEachKey(KeyProcedure procedure) const;

  std::map<std::string, std::string> GetProperties() const;

  void PrintProperties() const;

  static void Copy(const boost::filesystem::path& from,
                   const boost::filesystem::path& to);

  static void Copy(const boost::filesystem::path& from,
                   const boost::filesystem::path& to,
                   std::size_t new_block_size);

  static void Copy(const boost::filesystem::path& from,
                   const boost::filesystem::path& to, const Compare& compare);

  static void Copy(const boost::filesystem::path& from,
                   const boost::filesystem::path& to, const Compare& compare,
                   std::size_t new_block_size);

 private:
  enum class Match { kAll, kOne };

  std::size_t Delete(const Bytes& key, ValuePredicate predicate, Match match);

  std::size_t Update(const Bytes& key, ValueFunction function, Match match);

  void InitCallbacks();

  // Members may access each other in destructor calls,
  // thus the order of declaration matters - don't reorder.
  internal::Callbacks callbacks_;
  std::unique_ptr<internal::BlockPool> block_pool_;
  std::unique_ptr<internal::DataFile> data_file_;
  std::unique_ptr<internal::Table> table_;
  boost::filesystem::path directory_;
  Options options_;
};

}  // namespace multimap

#endif  // MULTIMAP_MAP_HPP
