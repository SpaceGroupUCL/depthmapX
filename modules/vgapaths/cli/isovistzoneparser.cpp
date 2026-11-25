// Copyright (C) 2019 Petros Koutsolampros

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

#include "isovistzoneparser.h"
#include "depthmapXcli/exceptions.h"
#include "depthmapXcli/parsingutils.h"
#include "depthmapXcli/runmethods.h"
#include "depthmapXcli/simpletimer.h"
#include "modules/vgapaths/core/vgaisovistzone.h"
#include "salalib/entityparsing.h"
#include <cstring>
#include <sstream>

using namespace depthmapX;

void IsovistZoneParser::parse(int argc, char **argv) {

    std::vector<std::string> origins;
    std::string originsFile;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp("-izo", argv[i]) == 0) {
            ENFORCE_ARGUMENT("-izo", i)
            if (!has_only_digits_dots_commas(argv[i])) {
                std::stringstream message;
                message << "Invalid origin point provided (" << argv[i]
                        << "). Should only contain digits dots and commas" << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            origins.push_back(argv[i]);
        } else if (std::strcmp("-izr", argv[i]) == 0) {
            ENFORCE_ARGUMENT("-izr", i)
            if (!has_only_digits_dots_commas(argv[i])) {
                std::stringstream message;
                message << "Invalid restriction distance provided (" << argv[i]
                        << "). Should only contain digits dots and commas" << std::flush;
                throw CommandLineException(message.str().c_str());
            }
            m_restrictDistance = std::stof(argv[i]);
        } else if (std::strcmp("-izf", argv[i]) == 0) {
            ENFORCE_ARGUMENT("-izf", i)
            originsFile = argv[i];
        } else if (std::strcmp("-izn", argv[i]) == 0) {
            ENFORCE_ARGUMENT("-izn", i)
            m_originSets.push_back(argv[i]);
        }
    }

    if (!originsFile.empty()) {
        std::ifstream file(originsFile);
        if (!file.good()) {
            std::stringstream message;
            message << "Failed to find file " << originsFile;
            throw depthmapX::CommandLineException(message.str());
        }
        auto pointSets = EntityParsing::parsePointSets(file, ',');
        m_originSets = pointSets.first;
        m_origins = pointSets.second;
    } else if (origins.empty()) {
        throw CommandLineException("At least one origin point (-izo) or a file (-izf) must be provided");
    } else if (m_originSets.size() > 0 & origins.size() != m_originSets.size()) {
        throw CommandLineException("Origin sets must either not be provided or be provided for every origin");
    } else {
        for (const auto &origin : origins) {
            m_origins.push_back(EntityParsing::parsePoint(origin));
        }
        if (m_originSets.size() == 0) {
            for (const auto &origin : origins) {
                m_originSets.push_back("");
            }
        }
    }
}

void IsovistZoneParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const {
    auto mGraph = dm_runmethods::loadGraph(clp.getFileName().c_str(), perfWriter);

    auto &graphRegion = mGraph->getRegion();
    PointMap &map = mGraph->getDisplayedPointMap();

    std::map<std::string, std::set<PixelRef>> pixelsFrom;
    auto namesIter = m_originSets.begin();
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
        pixelsFrom[*namesIter].insert(pixelFrom);
        namesIter++;
    }

    std::cout << "ok\nCalculating isovist zone... " << std::flush;

    std::unique_ptr<Communicator> comm(new ICommunicator());

    DO_TIMED("Calculating isovist zone", VGAIsovistZone(map, pixelsFrom, m_restrictDistance).run(comm.get()));

    std::cout << " ok\nWriting out result..." << std::flush;
    DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(), METAGRAPH_VERSION, false))
    std::cout << " ok" << std::endl;
}
