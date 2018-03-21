// Copyright (C) 2017 Petros Koutsolampros

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
#include "../depthmapXcli/importparser.h"
#include "argumentholder.h"

TEST_CASE("Import args valid", "valid")
{
    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "IMPORT", "-if", "importfile"};
        ImportParser cmdP;
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.getFilesToImport().size() == 1);
        REQUIRE(cmdP.getFilesToImport()[0] == "importfile");
    }
    {
        ArgumentHolder ah{"prog", "-f", "infile", "-o", "outfile", "-m", "IMPORT", "-if", "importfile1", "-if", "importfile2"};
        ImportParser cmdP;
        cmdP.parse(ah.argc(), ah.argv());
        REQUIRE(cmdP.getFilesToImport().size() == 2);
        REQUIRE(cmdP.getFilesToImport()[0] == "importfile1");
        REQUIRE(cmdP.getFilesToImport()[1] == "importfile2");
    }
}
