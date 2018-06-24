// Copyright (C) 2017 Christian Sailer

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

#include <catch.hpp>
#include <genlib/vectorhelpers.h>
#include <cliTest/selfcleaningfile.h>
#include <fstream>
#include <genlib/paftl.h>

TEST_CASE("vector reading and writing")
{
    using namespace dXvector;
    std::vector<int> intVec{1, 5, 34, -2, 5};
    SelfCleaningFile intFile("integers.bin");
    {
        std::ofstream outfile(intFile.Filename());
        writeVector(outfile, intVec);
    }

    {
        std::ifstream infile(intFile.Filename());
        auto copy = readVector<int>(infile);
        REQUIRE(copy == intVec);
    }

    std::vector<int> intCopy;
    {
        std::ifstream infile(intFile.Filename());
        readIntoVector(infile, intCopy);
    }
    REQUIRE(intCopy == intVec);

}

template<typename T> void comparePvecAndStdVec( const pvector<T> &pvec, const std::vector<T> &stdVec )
{
    REQUIRE(pvec.size() == stdVec.size());
    for (size_t i = 0; i < stdVec.size(); ++i )
    {
        REQUIRE(pvec[i] == stdVec[i]);
    }
}

TEST_CASE("Comaptibility between vector pvector streaming")
{
    std::vector<int> intVec{1, 5, 34, -2, 5};
    SelfCleaningFile intFile("integers.bin");
    {
        std::ofstream outfile(intFile.Filename());
        dXvector::writeVector(outfile, intVec);
    }

    pvector<int> pveci;
    {
        std::ifstream infile(intFile.Filename());
        pveci.read(infile);
    }
    comparePvecAndStdVec(pveci, intVec);

    pvector<int> intPvec;
    intPvec.push_back(324);
    intPvec.push_back(-23);
    intPvec.push_back(87764);
    intPvec.push_back(-9);

    {
        std::ofstream outfile(intFile.Filename());
        intPvec.write(outfile);
    }

    std::vector<int> copyVec;
    {
        std::ifstream infile(intFile.Filename());
        dXvector::readIntoVector(infile, copyVec);
    }

    comparePvecAndStdVec(intPvec, copyVec);


}
