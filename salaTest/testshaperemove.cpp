// Copyright (C) 2020 Petros Koutsolampros

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
#include "salalib/mapconverter.h"
#include "salalib/mgraph.h"
#include <iostream>
#include <sstream>

TEST_CASE("Testing deleting shapes from shapemaps") {
    std::unique_ptr<ShapeMap> shapeMap(new ShapeMap("Test ShapeMap"));

    // This is a hash (#) shape for simplicity
    shapeMap->makeLineShape(Line(Point2f(0.0, 0.5), Point2f(1.5, 0.5)));
    shapeMap->makeLineShape(Line(Point2f(0.5, 0.0), Point2f(0.5, 1.5)));
    shapeMap->makeLineShape(Line(Point2f(0.0, 1.0), Point2f(1.5, 1.0)));
    shapeMap->makeLineShape(Line(Point2f(1.0, 0.0), Point2f(1.0, 1.5)));

    REQUIRE(shapeMap->getAllShapes().size() == 4);

    SECTION("Delete from simple shapemap from the beginning") {
        int shapeCount = shapeMap->getAllShapes().size();
        for (size_t idx = shapeCount; idx > 0; idx--) {
            shapeMap->removeShape(shapeMap->getAllShapes().begin()->first, false);
            REQUIRE(shapeMap->getAllShapes().size() == idx - 1);
        }
    }

    SECTION("Delete from simple shapemap from the end") {
        int shapeCount = shapeMap->getAllShapes().size();
        for (size_t idx = shapeCount; idx > 0; idx--) {
            shapeMap->removeShape(shapeMap->getAllShapes().rbegin()->first, false);
            REQUIRE(shapeMap->getAllShapes().size() == idx - 1);
        }
    }

    SECTION("Delete from simple shapemap from the middle") {
        int shapeCount = shapeMap->getAllShapes().size();
        for (size_t idx = shapeCount; idx > 0; idx--) {
            int shapeRef =
                depthmapX::getMapAtIndex(shapeMap->getAllShapes(), int(shapeMap->getAllShapes().size() / 2))->first;
            shapeMap->removeShape(shapeRef, false);
            REQUIRE(shapeMap->getAllShapes().size() == idx - 1);
        }
    }
}

TEST_CASE("Testing deleting shapes from axial maps") {
    std::unique_ptr<ShapeMap> shapeMap(new ShapeMap("Test ShapeMap"));

    // This is a hash (#) shape for simplicity
    shapeMap->makeLineShape(Line(Point2f(0.0, 0.5), Point2f(1.5, 0.5)));
    shapeMap->makeLineShape(Line(Point2f(0.5, 0.0), Point2f(0.5, 1.5)));
    shapeMap->makeLineShape(Line(Point2f(0.0, 1.0), Point2f(1.5, 1.0)));
    shapeMap->makeLineShape(Line(Point2f(1.0, 0.0), Point2f(1.0, 1.5)));

    std::unique_ptr<ShapeGraph> axialMap =
        MapConverter::convertDataToAxial(nullptr, "Axial map", *shapeMap.get(), false);

    REQUIRE(axialMap->getAllShapes().size() == 4);
    REQUIRE(axialMap->getConnections().size() == 4);

    // creates this axial map:
    //        1   3
    //        |   |
    //    0 - + - + -
    //        |   |
    //    2 - + - + -
    //        |   |

    // map of shaperef -> vector of shaperef (not indices)
    std::map<int, std::vector<int>> axialConnections;
    axialConnections[0] = {1, 3};
    axialConnections[1] = {0, 2};
    axialConnections[2] = {1, 3};
    axialConnections[3] = {0, 2};

    int axialConnectivityColIdx = axialMap->getAttributeTable().getColumnIndex("Connectivity");
    AttributeTable &axialTable = axialMap->getAttributeTable();

    // check if shapes have connectivity attribute values that reflect the expected number of connections
    for (const auto &shape : axialMap->getAllShapes()) {
        REQUIRE(axialTable.getRow(AttributeKey(shape.first)).getValue(axialConnectivityColIdx) ==
                axialConnections[shape.first].size());
    }

    // check if the shape connectors have the expected internal sizes and are connected to the expected
    // other shapes
    for (int i = 0; i < axialMap->getConnections().size(); i++) {
        Connector connector = axialMap->getConnections()[i];
        std::vector<int> expectedConnections = axialConnections[axialMap->getShapeRefFromIndex(i)->first];
        REQUIRE(connector.count(Connector::CONN_ALL) == expectedConnections.size());
        for (int otherShapeRef : expectedConnections) {
            REQUIRE(std::find(connector.m_connections.begin(), connector.m_connections.end(), otherShapeRef) !=
                    connector.m_connections.end());
        }
    }

    SECTION("Delete from an axial map from the beginning") {
        std::map<int, int> connectivitiesAfterRemoval;

        for (auto iter = axialMap->getAllShapes().begin(); iter != axialMap->getAllShapes().end(); iter++) {
            connectivitiesAfterRemoval[iter->first] = axialConnections[iter->first].size();
        }

        int shapeCount = axialMap->getAllShapes().size();
        for (size_t idx = shapeCount; idx > 0; idx--) {
            int shapeRef = axialMap->getAllShapes().begin()->first;
            axialMap->removeShape(shapeRef, false);
            REQUIRE(axialMap->getAllShapes().size() == idx - 1);
            REQUIRE(axialMap->getConnections().size() == idx - 1);

            for (auto it = axialMap->getAllShapes().begin(); it != axialMap->getAllShapes().end(); it++) {
                std::vector<int> connConnections = axialConnections[it->first];
                if (std::find(connConnections.begin(), connConnections.end(), shapeRef) != connConnections.end()) {
                    // if the other shape contains this one, then remove from its connectivity
                    connectivitiesAfterRemoval[it->first] = connectivitiesAfterRemoval[it->first] - 1;
                }

                REQUIRE(axialTable.getRow(AttributeKey(it->first)).getValue(axialConnectivityColIdx) ==
                        connectivitiesAfterRemoval[it->first]);
            }
        }
    }

    SECTION("Delete from an axial map from the end") {
        std::map<int, int> connectivitiesAfterRemoval;

        for (auto iter = axialMap->getAllShapes().begin(); iter != axialMap->getAllShapes().end(); iter++) {
            connectivitiesAfterRemoval[iter->first] = axialConnections[iter->first].size();
        }

        int shapeCount = axialMap->getAllShapes().size();
        for (size_t idx = shapeCount; idx > 0; idx--) {
            int shapeRef = axialMap->getAllShapes().rbegin()->first;
            axialMap->removeShape(shapeRef, false);
            REQUIRE(axialMap->getAllShapes().size() == idx - 1);
            REQUIRE(axialMap->getConnections().size() == idx - 1);

            for (auto it = axialMap->getAllShapes().begin(); it != axialMap->getAllShapes().end(); it++) {
                std::vector<int> connConnections = axialConnections[it->first];
                if (std::find(connConnections.begin(), connConnections.end(), shapeRef) != connConnections.end()) {
                    // if the other shape contains this one, then remove from its connectivity
                    connectivitiesAfterRemoval[it->first] = connectivitiesAfterRemoval[it->first] - 1;
                }

                REQUIRE(axialTable.getRow(AttributeKey(it->first)).getValue(axialConnectivityColIdx) ==
                        connectivitiesAfterRemoval[it->first]);
            }
        }
    }

    SECTION("Delete from an axial map from the middle") {
        std::map<int, int> connectivitiesAfterRemoval;

        for (auto iter = axialMap->getAllShapes().begin(); iter != axialMap->getAllShapes().end(); iter++) {
            connectivitiesAfterRemoval[iter->first] = axialConnections[iter->first].size();
        }

        int shapeCount = axialMap->getAllShapes().size();
        for (size_t idx = shapeCount; idx > 0; idx--) {
            int shapeRef =
                depthmapX::getMapAtIndex(axialMap->getAllShapes(), int(axialMap->getAllShapes().size() / 2))->first;
            axialMap->removeShape(shapeRef, false);
            REQUIRE(axialMap->getAllShapes().size() == idx - 1);
            REQUIRE(axialMap->getConnections().size() == idx - 1);

            for (auto it = axialMap->getAllShapes().begin(); it != axialMap->getAllShapes().end(); it++) {
                std::vector<int> connConnections = axialConnections[it->first];
                if (std::find(connConnections.begin(), connConnections.end(), shapeRef) != connConnections.end()) {
                    // if the other shape contains this one, then remove from its connectivity
                    connectivitiesAfterRemoval[it->first] = connectivitiesAfterRemoval[it->first] - 1;
                }

                REQUIRE(axialTable.getRow(AttributeKey(it->first)).getValue(axialConnectivityColIdx) ==
                        connectivitiesAfterRemoval[it->first]);
            }
        }
    }
}

TEST_CASE("Testing deleting shapes from segment maps") {
    std::unique_ptr<ShapeMap> shapeMap(new ShapeMap("Test ShapeMap"));

    // This is a hash (#) shape for simplicity
    shapeMap->makeLineShape(Line(Point2f(0.0, 0.5), Point2f(1.5, 0.5)));
    shapeMap->makeLineShape(Line(Point2f(0.5, 1.5), Point2f(0.5, 0.0)));
    shapeMap->makeLineShape(Line(Point2f(0.0, 1.0), Point2f(1.5, 1.0)));
    shapeMap->makeLineShape(Line(Point2f(1.0, 0.0), Point2f(1.0, 1.5)));

    std::unique_ptr<ShapeGraph> axialMap =
        MapConverter::convertDataToAxial(nullptr, "Axial map", *shapeMap.get(), false);

    // creates this axial map:
    //        1   3
    //        |   |
    //    0 - + - + -
    //        |   |
    //    2 - + - + -
    //        |   |

    std::unique_ptr<ShapeGraph> segmentMap =
        MapConverter::convertAxialToSegment(nullptr, *axialMap.get(), "Segment map", true);

    // axial lines have been split in three segments
    //          |       |
    //          5       11
    //          |       |
    //    - 6 - + - 7 - + - 8 -
    //          |       |
    //          4       10
    //          |       |
    //    - 0 - + - 1 - + - 2 -
    //          |       |
    //          3       9
    //          |       |

    REQUIRE(segmentMap->getAllShapes().size() == 12);
    REQUIRE(segmentMap->getConnections().size() == 12);

    std::map<int, std::vector<int>> segmentForConnections;
    std::map<int, std::vector<int>> segmentBackConnections;
    segmentForConnections[0] = {1, 3, 4};
    segmentBackConnections[0] = {};
    segmentForConnections[1] = {2, 9, 10};
    segmentBackConnections[1] = {0, 3, 4};
    segmentForConnections[2] = {};
    segmentBackConnections[2] = {1, 9, 10};
    segmentForConnections[3] = {0, 1, 4};
    segmentBackConnections[3] = {};
    segmentForConnections[4] = {5, 6, 7};
    segmentBackConnections[4] = {0, 1, 3};
    segmentForConnections[5] = {};
    segmentBackConnections[5] = {4, 6, 7};
    segmentForConnections[6] = {4, 5, 7};
    segmentBackConnections[6] = {};
    segmentForConnections[7] = {8, 10, 11};
    segmentBackConnections[7] = {4, 5, 6};
    segmentForConnections[8] = {};
    segmentBackConnections[8] = {7, 10, 11};
    segmentForConnections[9] = {1, 2, 10};
    segmentBackConnections[9] = {};
    segmentForConnections[10] = {7, 8, 11};
    segmentBackConnections[10] = {1, 2, 9};
    segmentForConnections[11] = {};
    segmentBackConnections[11] = {7, 8, 10};

    int segmentConnectivityColIdx = segmentMap->getAttributeTable().getColumnIndex("Connectivity");
    AttributeTable &segmentTable = segmentMap->getAttributeTable();

    // check if shapes have connectivity attribute values that reflect the expected number of connections
    for (auto iter = segmentMap->getAllShapes().begin(); iter != segmentMap->getAllShapes().end(); iter++) {
        REQUIRE(segmentTable.getRow(AttributeKey(iter->first)).getValue(segmentConnectivityColIdx) ==
                segmentForConnections[iter->first].size() + segmentBackConnections[iter->first].size());
    }

    // check if the shape connectors have the expected internal sizes and are connected to the expected
    // other shapes
    for (int i = 0; i < segmentMap->getConnections().size(); i++) {
        Connector connector = segmentMap->getConnections()[i];
        std::vector<int> expectedForConnections = segmentForConnections[segmentMap->getShapeRefFromIndex(i)->first];
        std::vector<int> expectedBackConnections = segmentBackConnections[segmentMap->getShapeRefFromIndex(i)->first];
        REQUIRE(connector.count(Connector::SEG_CONN_ALL) ==
                expectedForConnections.size() + expectedBackConnections.size());
        REQUIRE(connector.count(Connector::SEG_CONN_FW) == expectedForConnections.size());
        REQUIRE(connector.count(Connector::SEG_CONN_BK) == expectedBackConnections.size());

        for (int otherShapeRef : expectedForConnections) {
            bool inForConnections =
                std::find_if(connector.m_forward_segconns.begin(), connector.m_forward_segconns.end(),
                             [&otherShapeRef](const std::pair<SegmentRef, int> &segmentRef) {
                                 return segmentRef.first.ref == otherShapeRef;
                             }) != connector.m_forward_segconns.end();
            REQUIRE(inForConnections);
        }
        for (int otherShapeRef : expectedBackConnections) {
            bool inBackConnections = std::find_if(connector.m_back_segconns.begin(), connector.m_back_segconns.end(),
                                                  [&otherShapeRef](const std::pair<SegmentRef, int> &segmentRef) {
                                                      return segmentRef.first.ref == otherShapeRef;
                                                  }) != connector.m_back_segconns.end();
            REQUIRE(inBackConnections);
        }
    }

    SECTION("Delete from a segment map from the beginning") {
        std::map<int, int> connectivitiesAfterRemoval;

        for (auto iter = segmentMap->getAllShapes().begin(); iter != segmentMap->getAllShapes().end(); iter++) {
            connectivitiesAfterRemoval[iter->first] =
                segmentForConnections[iter->first].size() + segmentBackConnections[iter->first].size();
        }

        int shapeCount = segmentMap->getAllShapes().size();
        for (size_t idx = shapeCount; idx > 0; idx--) {
            int shapeRef = segmentMap->getAllShapes().begin()->first;
            segmentMap->removeShape(shapeRef, false);
            REQUIRE(segmentMap->getAllShapes().size() == idx - 1);
            REQUIRE(segmentMap->getConnections().size() == idx - 1);

            for (auto it = segmentMap->getAllShapes().begin(); it != segmentMap->getAllShapes().end(); it++) {
                std::vector<int> connForConnections = segmentForConnections[it->first];
                std::vector<int> connBackConnections = segmentBackConnections[it->first];
                if (std::find(connForConnections.begin(), connForConnections.end(), shapeRef) !=
                    connForConnections.end()) {
                    // if the other shape contains this one, then remove from its connectivity
                    // removal does not currently work for segment maps, so connectivity is not adjusted
                    connectivitiesAfterRemoval[it->first] = connectivitiesAfterRemoval[it->first] - 1;
                } else if (std::find(connBackConnections.begin(), connBackConnections.end(), shapeRef) !=
                           connBackConnections.end()) {
                    // if the other shape contains this one, then remove from its connectivity
                    // removal does not currently work for segment maps, so connectivity is not adjusted
                    connectivitiesAfterRemoval[it->first] = connectivitiesAfterRemoval[it->first] - 1;
                }

                REQUIRE(segmentTable.getRow(AttributeKey(it->first)).getValue(segmentConnectivityColIdx) ==
                        connectivitiesAfterRemoval[it->first]);
            }
        }
    }

    SECTION("Delete from a segment map from the end") {
        std::map<int, int> connectivitiesAfterRemoval;

        for (auto iter = segmentMap->getAllShapes().begin(); iter != segmentMap->getAllShapes().end(); iter++) {
            connectivitiesAfterRemoval[iter->first] =
                segmentForConnections[iter->first].size() + segmentBackConnections[iter->first].size();
        }

        int shapeCount = segmentMap->getAllShapes().size();
        for (size_t idx = shapeCount; idx > 0; idx--) {
            int shapeRef = segmentMap->getAllShapes().rbegin()->first;
            segmentMap->removeShape(shapeRef, false);
            REQUIRE(segmentMap->getAllShapes().size() == idx - 1);
            REQUIRE(segmentMap->getConnections().size() == idx - 1);

            for (auto it = segmentMap->getAllShapes().begin(); it != segmentMap->getAllShapes().end(); it++) {
                std::vector<int> connForConnections = segmentForConnections[it->first];
                std::vector<int> connBackConnections = segmentBackConnections[it->first];
                if (std::find(connForConnections.begin(), connForConnections.end(), shapeRef) !=
                    connForConnections.end()) {
                    // if the other shape contains this one, then remove from its connectivity
                    // removal does not currently work for segment maps, so connectivity is not adjusted
                    connectivitiesAfterRemoval[it->first] = connectivitiesAfterRemoval[it->first] - 1;
                } else if (std::find(connBackConnections.begin(), connBackConnections.end(), shapeRef) !=
                           connBackConnections.end()) {
                    // if the other shape contains this one, then remove from its connectivity
                    // removal does not currently work for segment maps, so connectivity is not adjusted
                    connectivitiesAfterRemoval[it->first] = connectivitiesAfterRemoval[it->first] - 1;
                }

                REQUIRE(segmentTable.getRow(AttributeKey(it->first)).getValue(segmentConnectivityColIdx) ==
                        connectivitiesAfterRemoval[it->first]);
            }
        }
    }

    SECTION("Delete from a segment map from the middle") {
        std::map<int, int> connectivitiesAfterRemoval;

        for (auto iter = segmentMap->getAllShapes().begin(); iter != segmentMap->getAllShapes().end(); iter++) {
            connectivitiesAfterRemoval[iter->first] =
                segmentForConnections[iter->first].size() + segmentBackConnections[iter->first].size();
        }

        int shapeCount = segmentMap->getAllShapes().size();
        for (size_t idx = shapeCount; idx > 0; idx--) {
            int shapeRef =
                depthmapX::getMapAtIndex(segmentMap->getAllShapes(), int(segmentMap->getAllShapes().size() / 2))
                    ->first;
            segmentMap->removeShape(shapeRef, false);
            REQUIRE(segmentMap->getAllShapes().size() == idx - 1);
            REQUIRE(segmentMap->getConnections().size() == idx - 1);

            for (auto it = segmentMap->getAllShapes().begin(); it != segmentMap->getAllShapes().end(); it++) {
                std::vector<int> connForConnections = segmentForConnections[it->first];
                std::vector<int> connBackConnections = segmentBackConnections[it->first];
                if (std::find(connForConnections.begin(), connForConnections.end(), shapeRef) !=
                    connForConnections.end()) {
                    // if the other shape contains this one, then remove from its connectivity
                    // removal does not currently work for segment maps, so connectivity is not adjusted
                    connectivitiesAfterRemoval[it->first] = connectivitiesAfterRemoval[it->first] - 1;
                } else if (std::find(connBackConnections.begin(), connBackConnections.end(), shapeRef) !=
                           connBackConnections.end()) {
                    // if the other shape contains this one, then remove from its connectivity
                    // removal does not currently work for segment maps, so connectivity is not adjusted
                    connectivitiesAfterRemoval[it->first] = connectivitiesAfterRemoval[it->first] - 1;
                }

                REQUIRE(segmentTable.getRow(AttributeKey(it->first)).getValue(segmentConnectivityColIdx) ==
                        connectivitiesAfterRemoval[it->first]);
            }
        }
    }
}
