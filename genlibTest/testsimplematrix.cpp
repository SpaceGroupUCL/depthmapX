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
#include "../genlib/simplematrix.h"
#include <vector>
#include <algorithm>

template<typename T> void compareMatrixContent( depthmapX::BaseMatrix<T> const & matrix, std::vector<T> const & expected ){
    REQUIRE(matrix.size() == expected.size());
    std::vector<T> result(matrix.size());
    std::copy(matrix.begin(), matrix.end(), result.begin());
    REQUIRE(result == expected);
}

TEST_CASE("Row matrix test assignemnt copy and move"){
   depthmapX::RowMatrix<std::string> matrix(2,3);
   matrix(0,0) = "0,0";
   matrix(1,0) = "1,0";
   matrix(0,1) = "0,1";
   matrix(1,1) = "1,1";
   matrix(1,2) = "1,2";
   matrix(0,2) = "0,2";

   std::vector<std::string> expected{"0,0", "0,1", "0,2", "1,0", "1,1", "1,2"};
   compareMatrixContent(matrix, expected);

   depthmapX::RowMatrix<std::string> copy(matrix);
   compareMatrixContent(matrix, expected);
   compareMatrixContent(copy, expected);

   depthmapX::RowMatrix<std::string> clone(std::move(copy));
   compareMatrixContent(clone, expected);
   REQUIRE(copy.size() == 0);

   copy = clone;
   compareMatrixContent(copy, expected);
   REQUIRE(copy.columns() == 3);
   REQUIRE(copy.rows() == 2);
   compareMatrixContent(clone, expected);

   depthmapX::RowMatrix<std::string> assignMove(1,1);
   assignMove = std::move(copy);
   compareMatrixContent(assignMove, expected);
   REQUIRE(copy.size() == 0);
}

TEST_CASE("Row matrix test exceptions"){
    depthmapX::RowMatrix<int> matrix(2, 3);
    matrix(0,0) = 1;
    matrix(1,2) = -1;
    matrix(0,1) = 2;
    matrix(0,2) = 3;
    matrix(1,0) = -23;
    matrix(1,1) = 0;

    REQUIRE(matrix(1,2) == -1);

    compareMatrixContent(matrix, std::vector<int>{1, 2, 3, -23, 0, -1});

    REQUIRE_THROWS_WITH(matrix(5, 0), Catch::Contains("row out of range"));
    REQUIRE_THROWS_WITH(matrix(0, 5), Catch::Contains("column out of range"));
}

TEST_CASE("Column matrix test assignemnt copy and move"){
   depthmapX::ColumnMatrix<std::string> matrix(2,3);
   matrix(0,0) = "0,0";
   matrix(1,0) = "1,0";
   matrix(0,1) = "0,1";
   matrix(1,1) = "1,1";
   matrix(1,2) = "1,2";
   matrix(0,2) = "0,2";

   std::vector<std::string> expected{"0,0", "1,0", "0,1", "1,1", "0,2", "1,2"};
   compareMatrixContent(matrix, expected);

   depthmapX::ColumnMatrix<std::string> copy(matrix);
   compareMatrixContent(matrix, expected);
   compareMatrixContent(copy, expected);

   depthmapX::ColumnMatrix<std::string> clone(std::move(copy));
   compareMatrixContent(clone, expected);
   REQUIRE(copy.size() == 0);

   copy = clone;
   compareMatrixContent(copy, expected);
   REQUIRE(copy.columns() == 3);
   REQUIRE(copy.rows() == 2);
   compareMatrixContent(clone, expected);

   depthmapX::ColumnMatrix<std::string> assignMove(1,1);
   assignMove = std::move(copy);
   compareMatrixContent(assignMove, expected);
   REQUIRE(copy.size() == 0);

}

TEST_CASE("Column matrix test exceptions"){
    depthmapX::ColumnMatrix<int> matrix(2, 3);
    matrix(0,0) = 1;
    matrix(1,2) = -1;
    matrix(0,1) = 2;
    matrix(0,2) = 3;
    matrix(1,0) = -23;
    matrix(1,1) = 0;

    REQUIRE(matrix(1,2) == -1);

    compareMatrixContent(matrix, std::vector<int>{1, -23, 2, 0, 3, -1});

    REQUIRE_THROWS_WITH(matrix(5, 0), Catch::Contains("row out of range"));
    REQUIRE_THROWS_WITH(matrix(0, 5), Catch::Contains("column out of range"));
}

TEST_CASE("Fill and reset"){
    depthmapX::ColumnMatrix<int> matrix(2,3);
    matrix.initialiseValues(-42);
    compareMatrixContent(matrix, std::vector<int>(6, -42));

    matrix.reset(3,4);
    REQUIRE(matrix.rows() == 3);
    REQUIRE(matrix.columns() == 4);

    matrix.initialiseValues(12);
    compareMatrixContent(matrix, std::vector<int>(12, 12));
}
