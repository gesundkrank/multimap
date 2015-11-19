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

#include "multimap/operations.hpp"

#include <boost/filesystem/operations.hpp>
#include "internal/Base64.hpp"

namespace multimap {

Map::Stats stats(const boost::filesystem::path& directory) {}

void importFromBase64(const boost::filesystem::path& directory,
                      const boost::filesystem::path& input) {
  Options options;
  options.error_if_exists = false;
  options.create_if_missing = false;
  importFromBase64(directory, input, options);
}

void importFromBase64(const boost::filesystem::path& directory,
                      const boost::filesystem::path& input,
                      const Options& options) {
  Map map(directory, options);

  const auto import_file = [&map](const boost::filesystem::path& filepath) {
    mt::check(boost::filesystem::is_regular_file(filepath),
              "'%s' is not a regular file.", filepath.c_str());
    std::ifstream ifs(filepath.string());
    mt::check(ifs, "Could not open '%s'.", filepath.c_str());

    std::string base64_key;
    std::string base64_value;
    std::string binary_key;
    std::string binary_value;
    if (ifs >> base64_key) {
      internal::Base64::decode(base64_key, &binary_key);
      while (ifs) {
        switch (ifs.peek()) {
          case '\n':
          case '\r':
            ifs >> base64_key;
            internal::Base64::decode(base64_key, &binary_key);
            break;
          case '\f':
          case '\t':
          case '\v':
          case ' ':
            ifs.ignore();
            break;
          default:
            ifs >> base64_value;
            internal::Base64::decode(base64_value, &binary_value);
            map.put(binary_key, binary_value);
        }
      }
    }
  };

  const auto is_hidden = [](const boost::filesystem::path& path) {
    return path.filename().string().front() == '.';
  };

  if (boost::filesystem::is_regular_file(input)) {
    import_file(input);
  } else if (boost::filesystem::is_directory(input)) {
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator it(input); it != end; ++it) {
      if (!is_hidden(it->path())) {
        import_file(it->path());
      }
    }
  }
}

void exportToBase64(const boost::filesystem::path& directory,
                    const boost::filesystem::path& output) {
  exportToBase64(directory, output, Callables::Compare());
}

void exportToBase64(const boost::filesystem::path& directory,
                    const boost::filesystem::path& output,
                    Callables::Compare compare) {
  Options options;
  options.error_if_exists = false;
  options.create_if_missing = false;
  Map map(directory, options);

  std::ofstream ofs(output.string());
  mt::check(ofs, "Could not create '%s'.", output.c_str());

  std::string base64_key;
  std::string base64_value;
  std::vector<std::string> sorted_values;
  if (compare) {
    map.forEachEntry([&](const Bytes& key, Map::ListIterator&& iter) {
      // TODO Test if reusing sorted_values makes any difference.

      // Sort values
      sorted_values.clear();
      sorted_values.reserve(iter.available());
      while (iter.hasNext()) {
        sorted_values.push_back(iter.next().toString());
      }
      std::sort(sorted_values.begin(), sorted_values.end(), compare);

      // Write as Base64
      internal::Base64::encode(key, &base64_key);
      ofs << base64_key;
      for (const auto& value : sorted_values) {
        internal::Base64::encode(value, &base64_value);
        ofs << ' ' << base64_value;
      }
      ofs << '\n';
    });
  } else {
    map.forEachEntry([&](const Bytes& key, Map::ListIterator&& iter) {
      // Write as Base64
      internal::Base64::encode(key, &base64_key);
      ofs << base64_key;
      while (iter.hasNext()) {
        internal::Base64::encode(iter.next(), &base64_value);
        ofs << ' ' << base64_value;
      }
      ofs << '\n';
    });
  }
}

void optimize(const boost::filesystem::path& directory,
              const boost::filesystem::path& output, const Options& options) {
  const auto abs_dir = boost::filesystem::absolute(directory);
  mt::check(boost::filesystem::is_directory(abs_dir),
            "The path '%s' does not refer to a directory.", abs_dir.c_str());

  internal::System::DirectoryLockGuard lock(abs_dir,
                                            internal::getNameOfLockFile());
  const auto id_file = abs_dir / internal::getNameOfIdFile();
  const auto map_exists = boost::filesystem::is_regular_file(id_file);
  mt::check(map_exists, "No Multimap found in '%s'.", abs_dir.c_str());

  const auto id = internal::Id::readFromFile(id_file);
  internal::checkVersion(id.version_major, id.version_minor);

  // TODO Verify id.checksum.

  Map new_map;
  const auto prefix = abs_dir / internal::getFilePrefix();
  for (std::size_t i = 0; i != id.num_shards; ++i) {
    const auto table_prefix = prefix.string() + '.' + std::to_string(i);
    internal::Table table(table_prefix, internal::Table::Options());

    if (i == 0) {
      Options output_opts = options;
      output_opts.error_if_exists = true;
      output_opts.create_if_missing = true;
      if (output_opts.block_size == 0) {
        output_opts.block_size = table.getBlockSize();
      }
      if (output_opts.num_shards == 0) {
        output_opts.num_shards = id.num_shards;
      }
      new_map = Map(output, output_opts);
    }

    if (options.compare) {
      std::vector<std::string> sorted_values;
      // TODO Test if reusing sorted_values makes any difference.
      table.forEachEntry([&new_map, &options, &sorted_values](
          const Bytes& key, Map::ListIterator&& iter) {
        sorted_values.clear();
        sorted_values.reserve(iter.available());
        while (iter.hasNext()) {
          sorted_values.push_back(iter.next().toString());
        }
        std::sort(sorted_values.begin(), sorted_values.end(), options.compare);
        for (const auto& value : sorted_values) {
          new_map.put(key, value);
        }
      });
    } else {
      table.forEachEntry(
          [&new_map](const Bytes& key, Map::ListIterator&& iter) {
            while (iter.hasNext()) {
              new_map.put(key, iter.next());
            }
          });
    }
  }
}

} // namespace multimap