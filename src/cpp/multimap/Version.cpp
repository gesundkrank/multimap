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

#include "multimap/Version.hpp"

#include "multimap/thirdparty/mt/mt.hpp"

namespace multimap {

void Version::checkCompatibility(int major, int minor) {
  mt::check(major == MAJOR && minor <= MINOR,
            "Version check failed. Please install Multimap version %u.%u.",
            major, minor);
}

}  // namespace multimap
