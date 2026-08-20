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

#include "shortestpathparser.h"
#include "depthmapXcli/exceptions.h"
#include "depthmapXcli/parsingutils.h"
#include "depthmapXcli/runmethods.h"
#include "depthmapXcli/simpletimer.h"
#include "modules/vgapaths/core/vgaangularshortestpath.h"
#include "modules/vgapaths/core/vgametricshortestpath.h"
#include "modules/vgapaths/core/vgametricshortestpathtomany.h"
#include "modules/vgapaths/core/vgavisualshortestpath.h"
#include "salalib/entityparsing.h"
#include <cstring>
#include <sstream>

using namespace depthmapX;

void ShortestPathParser::parse(int argc, char **argv) {

    std::vector<std::string> origins;
    std::vector<std::string> destinations;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp("-spo", argv[i]) == 0) {
            ENFORCE_ARGUMENT("-spo", i)
            if (!has_only_digits_dots_commas(argv[i])) {
                std::stringstream message;
                message << "Invalid origin point provided (" << argv[i]
                        << "). Should only contain digits dots and commas" << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            origins.push_back(argv[i]);
        } else if (std::strcmp("-spd", argv[i]) == 0) {
            ENFORCE_ARGUMENT("-spd", i)
            if (!has_only_digits_dots_commas(argv[i])) {
                std::stringstream message;
                message << "Invalid destination point provided (" << argv[i]
                        << "). Should only contain digits dots and commas" << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            destinations.push_back(argv[i]);
        } else if (std::strcmp("-spt", argv[i]) == 0) {
            ENFORCE_ARGUMENT("-spt", i)
            if (std::strcmp(argv[i], "angular") == 0) {
                m_shortestPathType = ShortestPathType::ANGULAR;
            } else if (std::strcmp(argv[i], "metric") == 0) {
                m_shortestPathType = ShortestPathType::METRIC;
            } else if (std::strcmp(argv[i], "visual") == 0) {
                m_shortestPathType = ShortestPathType::VISUAL;
            } else {
                throw CommandLineException(std::string("Invalid Shortest Path type: ") + argv[i]);
            }
        }
    }

    if (origins.empty()) {
        throw CommandLineException("At least one origin point (-spo) must be provided");
    } else {
        for (const auto &origin : origins) {
            m_origins.push_back(EntityParsing::parsePoint(origin));
        }
    }
    if (destinations.empty()) {
        throw CommandLineException("At least one destination point (-spd) must be provided");
    } else {
        for (const auto &destination : destinations) {
            m_destinations.push_back(EntityParsing::parsePoint(destination));
        }
    }
    if (m_shortestPathType == ShortestPathType::NONE) {
        throw CommandLineException("Shortest path type (-spt) must be provided");
    }
    if (m_shortestPathType != ShortestPathType::METRIC && m_destinations.size() > 1) {
        throw CommandLineException("Multple shortest paths may only be calculated with metric depth currently");
    }
}

void ShortestPathParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const {
    auto mGraph = dm_runmethods::loadGraph(clp.getFileName().c_str(), perfWriter);

    std::cout << "ok\nSelecting cells... " << std::flush;

    auto &graphRegion = mGraph->getRegion();
    PointMap &map = mGraph->getDisplayedPointMap();

    std::set<PixelRef> pixelsFrom;
    for (const Point2f &origin : m_origins) {
        if (!graphRegion.contains(origin) || !map.getRegion().contains(origin)) {
            throw depthmapX::RuntimeException("Origin point " + std::to_string(origin.x) + ", " +
                                              std::to_string(origin.y) + " outside of target region");
        }
        PixelRef pixelFrom = map.pixelate(origin);
        if (!map.getPoint(pixelFrom).filled()) {
            throw depthmapX::RuntimeException("Origin point " + std::to_string(origin.x) + ", " +
                                              std::to_string(origin.y) + " not filled in target pointmap");
        }
        pixelsFrom.insert(pixelFrom);
    }
    std::set<PixelRef> pixelsTo;
    for (const Point2f &destination : m_destinations) {

        if (!graphRegion.contains(destination) || !map.getRegion().contains(destination)) {
            throw depthmapX::RuntimeException("Destination point " + std::to_string(destination.x) + ", " +
                                              std::to_string(destination.y) + " outside of target region");
        }
        PixelRef pixelTo = map.pixelate(destination);
        if (!map.getPoint(pixelTo).filled()) {
            throw depthmapX::RuntimeException("Destination point " + std::to_string(destination.x) + ", " +
                                              std::to_string(destination.y) + " not filled in target pointmap");
        }
        pixelsTo.insert(pixelTo);
    }

    std::cout << "ok\nCalculating shortest path... " << std::flush;

    std::unique_ptr<Communicator> comm(new ICommunicator());

    DO_TIMED(
        "Calculating shortest path", switch (m_shortestPathType) {
            case ShortestPathParser::ShortestPathType::ANGULAR:
                VGAAngularShortestPath(map, *pixelsFrom.begin(), *pixelsTo.begin()).run(comm.get());
                break;
            case ShortestPathParser::ShortestPathType::METRIC:
                if (pixelsTo.size() == 1) {
                    VGAMetricShortestPath(map, pixelsFrom, *pixelsTo.begin()).run(comm.get());
                } else {
                    VGAMetricShortestPathToMany(map, pixelsFrom, pixelsTo).run(comm.get());
                }
                break;
            case ShortestPathParser::ShortestPathType::VISUAL:
                VGAVisualShortestPath(map, *pixelsFrom.begin(), *pixelsTo.begin()).run(comm.get());
                break;
            default: {
                throw depthmapX::SetupCheckException("Error, unsupported shortest path mode");
            }
        });

    std::cout << " ok\nWriting out result..." << std::flush;
    DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(), METAGRAPH_VERSION, false))
    std::cout << " ok" << std::endl;
}
