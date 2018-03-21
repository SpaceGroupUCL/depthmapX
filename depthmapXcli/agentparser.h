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

#pragma once

#include <string>
#include "imodeparser.h"
#include "genlib/p2dpoly.h"
#include "commandlineparser.h"

class AgentParser : public IModeParser
{
public:
    virtual std::string getModeName() const
    {
        return "AGENTS";
    }

    virtual std::string getHelp() const
    {
        return    "Mode options for AGENTS:\n"\
                  "-am <agent mode> one of:\n"
                  "    standard\n"\
                  "    los-length (Line of Sight length)\n"\
                  "    occ-length (Occluded length)\n"\
                  "    occ-any (Any occlusions)\n"\
                  "    occ-group-45 (Occlusion group bins - 45 degrees)\n"\
                  "    occ-group-60 (Occlusion group bins - 60 degrees)\n"\
                  "    occ-furthest (Furthest occlusion per bin)\n"\
                  "    bin-far-dist (Per bin far distance weighted)\n"\
                  "    bin-angle (Per bin angle weighted)\n"\
                  "    bin-far-dist-angle (Per bin far-distance and angle weighted)\n"\
                  "    bin-memory (Per bin memory)\n"\
                  "-ats <timesteps> set total system timesteps\n"\
                  "-arr <rate> set agent release rate (likelyhood of release per timestep)\n"\
                  "-atrails <no of trails> record trails for this amount of agents (set to 0 to record all"\
                           ", with max possible currently = 50)\n"\
                  "-afov <fov> set agent field-of-view (bins)\n"\
                  "-asteps <steps> set agent steps before turn decision\n"\
                  "-alife <timesteps> set agent total lifetime (in timesteps)\n"\
                  "-alocseed <seed> set agents to start at random locations with specific seed (0 to 10)\n"\
                  "-alocfile <agent starting points file>\n"\
                  "-aloc <single agent starting point coordinates> provided in csv (x1,y1) "\
                  "for example \"0.1,0.2\". Provide multiple times for multiple links\n"\
                  "-ot <output type> available output types (may use more than one):"\
                  "    graph (graph file, default)"\
                  "    gatecounts (csv with cells of grid with gate counts)"\
                  "    trails (csv with lines showing path traversed by each agent)";
    }

public:
    AgentParser();
    virtual void parse(int argc, char *argv[]);
    virtual void run(const CommandLineParser &clp, IPerformanceSink& perfWriter) const;

    enum AgentMode{
        NONE,
        STANDARD,
        LOS_LENGTH,
        OCC_LENGTH,
        OCC_ANY,
        OCC_GROUP_45,
        OCC_GROUP_60,
        OCC_FURTHEST,
        BIN_FAR_DIST,
        BIN_ANGLE,
        BIN_FAR_DIST_ANGLE,
        BIN_MEMORY
    };

    enum OutputType{
        GRAPH,
        GATECOUNTS,
        TRAILS
    };

    // agent options
    AgentMode getAgentMode() const { return _agentMode; }

    int totalSystemTimestemps() const { return _totalSystemTimestemps; }
    double releaseRate() const { return _releaseRate; }
    int recordTrailsForAgents() const { return _recordTrailsForAgents; }
    int randomReleaseLocationSeed() const { return _randomReleaseLocationSeed; }

    int agentFOV() const { return _agentFOV; }
    int agentStepsBeforeTurnDecision() const { return _agentStepsBeforeTurnDecision; }
    int agentLifeTimesteps() const { return _agentLifeTimesteps; }

    std::vector<Point2f> getReleasePoints() const { return _releasePoints; }

    std::vector<OutputType> outputTypes() const { return _outputTypes; }

private:
    // agent options
    AgentMode _agentMode;

    int _totalSystemTimestemps = 0;
    double _releaseRate = 0.0;
    int _recordTrailsForAgents = -1;

    int _agentFOV = 0; // Field of view (bins)
    int _agentStepsBeforeTurnDecision = 0; // Steps before turn decision
    int _agentLifeTimesteps = 0; // Timesteps in system

    int _randomReleaseLocationSeed = -1;
    std::vector<Point2f> _releasePoints;

    std::vector<OutputType> _outputTypes;
};

