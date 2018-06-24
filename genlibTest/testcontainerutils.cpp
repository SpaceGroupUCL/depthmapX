// Copyright (C) 2018 Christian Sailer

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "catch.hpp"
#include <genlib/containerutils.h>
#include <vector>


TEST_CASE("Test binary search helper with container", ""){
    std::vector<int> testVec{ 1, 2, 4, 5};

    REQUIRE(*depthmapX::findBinary(testVec, 2) == 2);
    REQUIRE(depthmapX::findBinary(testVec, 3) == testVec.end());
    REQUIRE(depthmapX::findBinary(testVec, 6) == testVec.end());
    auto iter = depthmapX::findBinary(testVec, 2);
    *iter = 3;
    REQUIRE(*depthmapX::findBinary(testVec, 3) == 3);

    const std::vector<int>& constVec = testVec;
    REQUIRE(*depthmapX::findBinary(constVec, 3) == 3);
    REQUIRE(depthmapX::findBinary(constVec, 2) == testVec.end());
    REQUIRE(depthmapX::findBinary(constVec, 6) == testVec.end());
}
