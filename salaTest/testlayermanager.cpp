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
#include <salalib/layermanagerimpl.h>
#include <cliTest/selfcleaningfile.h>
#include <fstream>

TEST_CASE("Test layer manager")
{
    LayerManagerImpl manager;
    REQUIRE(manager.isVisible(1));
    REQUIRE(manager.getLayerName(0) == "Everything");
    REQUIRE(manager.isLayerVisible(0));
    REQUIRE(manager.getLayerIndex("Everything") == 0);
    REQUIRE(manager.getKey(0) == 1);


    size_t index1 = manager.addLayer("some layer");
    REQUIRE(index1 == 1);
    REQUIRE_FALSE(manager.isVisible(2));
    REQUIRE_FALSE(manager.isLayerVisible(1));
    REQUIRE(manager.getLayerName(1) == "some layer");
    REQUIRE(manager.getLayerIndex("some layer") == 1);
    REQUIRE(manager.getKey(1) == 2);
    REQUIRE(manager.getKey(5) == 32);

    manager.setLayerVisible(1, true);
    REQUIRE_FALSE(manager.isVisible(1));
    REQUIRE(manager.isVisible(2));

    size_t index2 = manager.addLayer("another layer");
    REQUIRE(index2 == 2);
    REQUIRE(manager.getLayerName(2) == "another layer");
    REQUIRE(manager.getLayerIndex("another layer") == 2);
    REQUIRE_FALSE(manager.isLayerVisible(2));

    manager.setLayerVisible(2, true);
    REQUIRE_FALSE(manager.isVisible(1));
    REQUIRE(manager.isVisible(2));
    REQUIRE(manager.isVisible(4));

    manager.setLayerVisible(2, false);
    REQUIRE_FALSE(manager.isVisible(1));
    REQUIRE(manager.isVisible(2));
    REQUIRE_FALSE(manager.isVisible(4));

    manager.setLayerVisible(0, false);
    REQUIRE_FALSE(manager.isVisible(1));
    REQUIRE_FALSE(manager.isVisible(2));
    REQUIRE_FALSE(manager.isVisible(4));

    manager.setLayerVisible(2, true);
    REQUIRE_FALSE(manager.isVisible(1));
    REQUIRE_FALSE(manager.isVisible(2));
    REQUIRE(manager.isVisible(4));

    manager.setLayerVisible(0, true);
    REQUIRE(manager.isVisible(1));
    REQUIRE_FALSE(manager.isVisible(2));
    REQUIRE_FALSE(manager.isVisible(4));

    REQUIRE_THROWS_AS(manager.addLayer("another layer"), LayerManager::DuplicateKeyException);

    // test read and write
    SelfCleaningFile serialisedManager("manager.bin");
    {
        std::ofstream file(serialisedManager.Filename());
        manager.write(file);
    }

    LayerManagerImpl copy;
    {
        std::ifstream file(serialisedManager.Filename());
        copy.read(file);
    }
    REQUIRE(copy.getLayerName(1) == "some layer");
    REQUIRE(copy.getLayerIndex("some layer") == 1);
    REQUIRE(copy.isVisible(1));
    REQUIRE_FALSE(copy.isVisible(2));
    REQUIRE_FALSE(copy.isVisible(4));


}
