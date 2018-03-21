// Copyright (C) 2017 Christian Sailer
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

#include "runmethods.h"
#include "salalib/mgraph.h"
#include "salalib/linkutils.h"
#include "radiusconverter.h"
#include "exceptions.h"
#include "simpletimer.h"
#include <memory>
#include <sstream>
#include <vector>
#include <salalib/gridproperties.h>
#include <genlib/legacyconverters.h>
#include <salalib/importutils.h>

namespace dm_runmethods
{


#define CONCAT_(x,y) x##y
#define CONCAT(x,y) CONCAT_(x,y)
#define DO_TIMED(message, code)\
    SimpleTimer CONCAT(t_, __LINE__); \
    code; \
    perfWriter.addData(message, CONCAT(t_, __LINE__).getTimeInSeconds());


    std::unique_ptr<MetaGraph> loadGraph(const std::string& filename, IPerformanceSink &perfWriter) {
        std::unique_ptr<MetaGraph> mgraph(new MetaGraph);
        std::cout << "Loading graph " << filename << std::flush;
        DO_TIMED( "Load graph file", auto result = mgraph->readFromFile(filename);)
        if ( result != MetaGraph::OK)
        {
            std::stringstream message;
            message << "Failed to load graph from file " << filename << ", error " << result << flush;
            throw depthmapX::RuntimeException(message.str().c_str());
        }
        std::cout << " ok\n" << std::flush;
        return mgraph;
    }

    void importFiles(const CommandLineParser &cmdP, const std::vector<string> &filesToImport, IPerformanceSink &perfWriter)
    {
        std::ifstream mainFileStream(cmdP.getFileName().c_str());
        if(!mainFileStream.good()) {
            std::stringstream message;
            message << "File not found: " << cmdP.getFileName() << flush;
            throw depthmapX::RuntimeException(message.str().c_str());
        }

        std::unique_ptr<MetaGraph> mgraph(new MetaGraph);
        DO_TIMED( "Load graph file", auto result = mgraph->readFromFile(cmdP.getFileName());)
        if ( result != MetaGraph::OK && result != MetaGraph::NOT_A_GRAPH)
        {
            std::stringstream message;
            message << "Failed to load graph from file " << cmdP.getFileName() << ", error " << result << flush;
            throw depthmapX::RuntimeException(message.str().c_str());
        }

        if ( result == MetaGraph::NOT_A_GRAPH)
        {
            // not a graph, try to import the file
            std::string ext = cmdP.getFileName().substr(cmdP.getFileName().length() - 4, cmdP.getFileName().length() - 1);
            ifstream file(cmdP.getFileName());

            std::unique_ptr<Communicator> comm(new ICommunicator());

            depthmapX::ImportFileType importFileType = depthmapX::ImportFileType::TSV;
            if(dXstring::toLower(ext) == ".csv") {
                importFileType = depthmapX::ImportFileType::CSV;
            } else if (dXstring::toLower(ext) == ".dxf") {
                importFileType = depthmapX::ImportFileType::DXF;
            }

            depthmapX::importFile(*mgraph,
                                  file,
                                  false,
                                  cmdP.getFileName(),
                                  depthmapX::ImportType::DRAWINGMAP,
                                  importFileType);
        }
        DO_TIMED("Writing graph", mgraph->write(cmdP.getOuputFile().c_str(),METAGRAPH_VERSION, false);)
    }

    void linkGraph(const CommandLineParser &cmdP, const std::vector<Line> &mergeLines, IPerformanceSink &perfWriter)
    {
        auto mgraph = loadGraph(cmdP.getFileName().c_str(), perfWriter);
        SimpleTimer t;
        PointMap& currentMap = mgraph->getDisplayedPointMap();

        vector<PixelRefPair> newLinks = depthmapX::pixelateMergeLines(mergeLines, currentMap);
        depthmapX::mergePixelPairs(newLinks, currentMap);

        perfWriter.addData("Linking graph", t.getTimeInSeconds());
        DO_TIMED("Writing graph", mgraph->write(cmdP.getOuputFile().c_str(),METAGRAPH_VERSION, false);)
    }

    void runVga(const CommandLineParser &cmdP, const VgaParser &vgaP, const IRadiusConverter &converter, IPerformanceSink &perfWriter)
    {
        auto mgraph = loadGraph(cmdP.getFileName().c_str(), perfWriter);

        std::unique_ptr<Communicator> comm(new ICommunicator());
        std::unique_ptr<Options> options(new Options());

        cout << "Getting options..." << std::flush;
        switch(vgaP.getVgaMode())
        {
            case VgaParser::VgaMode::VISBILITY:
                options->output_type = Options::OUTPUT_VISUAL;
                options->local = vgaP.localMeasures();
                options->global = vgaP.globalMeasures();
                if (options->global )
                {
                    options->radius = converter.ConvertForVisibility(vgaP.getRadius());
                }
                break;
            case VgaParser::VgaMode::METRIC:
                options->output_type = Options::OUTPUT_METRIC;
                options->radius = converter.ConvertForMetric(vgaP.getRadius());
                break;
            case VgaParser::VgaMode::ANGULAR:
                options->output_type = Options::OUTPUT_ANGULAR;
                break;
            case VgaParser::VgaMode::ISOVIST:
                options->output_type = Options::OUTPUT_ISOVIST;
                break;
            case VgaParser::VgaMode::THRU_VISION:
                options->output_type = Options::OUTPUT_THRU_VISION;
                break;
            default:
                throw depthmapX::SetupCheckException("Unsupported VGA mode");
        }
        cout << " ok\nAnalysing graph..." << std::flush;

        DO_TIMED("Run VGA", mgraph->analyseGraph(comm.get(), *options, cmdP.simpleMode() ))
        std::cout << " ok\nWriting out result..." << std::flush;
        DO_TIMED("Writing graph", mgraph->write(cmdP.getOuputFile().c_str(),METAGRAPH_VERSION, false))
        std::cout << " ok" << std::endl;
    }

    void fillGraph(MetaGraph& graph, const Point2f& point)
    {
        auto r = graph.getRegion();
        if (!r.contains(point))
        {
            throw depthmapX::RuntimeException("Point outside of target region");
        }
        graph.makePoints(point, 0, 0);
    }

    void runVisualPrep(
            const CommandLineParser &clp,
            double gridSize, const std::vector<Point2f> &fillPoints,
            double maxVisibility,
            bool boundaryGraph,
            IPerformanceSink &perfWriter)
    {
        auto mGraph = loadGraph(clp.getFileName().c_str(),perfWriter);

        std::cout << "Initial checks... " << std::flush;
        auto state = mGraph->getState();
        if (~state & MetaGraph::LINEDATA)
        {
            throw depthmapX::RuntimeException("Graph must have line data before preparing VGA");
        }
        // set grid
        QtRegion r = mGraph->getRegion();

        GridProperties gp(__max(r.width(), r.height()));
        if ( gridSize > gp.getMax() ||  gridSize < gp.getMin())
        {
            std::stringstream message;
            message << "Chosen grid spacing " << gridSize << " is outside of the expected interval of "
                    << gp.getMin() << " <= spacing <= " << gp.getMax() << std::flush;
            throw depthmapX::RuntimeException(message.str());
        }

        std::cout << "ok\nSetting up grid... " << std::flush;
        if (!mGraph->PointMaps::size() || mGraph->getDisplayedPointMap().isProcessed()) {
           // this can happen if there are no displayed maps -- so flag new map required:
            mGraph->addNewMap();
        }
        DO_TIMED("Setting grid", mGraph->setGrid(gridSize, Point2f(0.0, 0.0)))

        std::cout << "ok\nFilling grid... " << std::flush;
        DO_TIMED("Filling grid",
                 for_each(fillPoints.begin(), fillPoints.end(), [&mGraph](const Point2f &point)->void{fillGraph(*mGraph, point);}))


        std::cout << "ok\nCalculating connectivity... " << std::flush;
        DO_TIMED("Calculate Connectivity", mGraph->makeGraph(0, boundaryGraph ? 1 : 0, maxVisibility))
        std::cout << " ok\nWriting out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
                std::cout << " ok" << std::endl;
    }

    void runAxialAnalysis(const CommandLineParser &clp, const AxialParser &ap, IPerformanceSink &perfWriter)
    {
        auto mGraph = loadGraph(clp.getFileName().c_str(), perfWriter);

        auto state = mGraph->getState();
        if ( ap.runAllLines())
        {
           if (~state & MetaGraph::LINEDATA)
           {
               throw depthmapX::RuntimeException("Line drawing must be loaded before axial map can be constructed");
           }
           std::cout << "Making all line map... " << std::flush;
           DO_TIMED("Making all axes map", for_each (ap.getAllAxesRoots().begin(),ap.getAllAxesRoots().end(), [&mGraph](const Point2f &point)->void{mGraph->makeAllLineMap(0, point);} ))
           std::cout << "ok" << std::endl;
        }

        if (ap.runFewestLines())
        {
            if (~state & MetaGraph::LINEDATA)
            {
                throw depthmapX::RuntimeException("Line drawing must be loaded before fewest line map can be constructed");
            }
            if (!mGraph->hasAllLineMap())
            {
                throw depthmapX::RuntimeException("All line map must be constructed before fewest lines can be constructed. Use -aa to do this");
            }
            std::cout << "Constructing fewest line map... " << std::flush;
            DO_TIMED("Fewest line map", mGraph->makeFewestLineMap(0,1))
            std::cout << "ok" << std::endl;
        }

        if (ap.runAnalysis())
        {
            std::cout << "Running axial analysis... " << std::flush;
            Options options;
            options.radius_list = genshim::toPVector(ap.getRadii());
            options.choice = ap.useChoice();
            options.local = ap.useLocal();
            options.fulloutput = ap.calculateRRA();
            DO_TIMED("Axial analysis", mGraph->analyseAxial(0, options, clp.simpleMode()))
            std::cout << "ok\n" << std::flush;

        }
        std::cout << "Writing out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
        std::cout << " ok" << std::endl;

    }

    void runAgentAnalysis(const CommandLineParser &cmdP, const AgentParser &agentP, IPerformanceSink &perfWriter) {

        std::unique_ptr<Communicator> comm(new ICommunicator());

        auto mgraph = loadGraph(cmdP.getFileName().c_str(), perfWriter);

        PointMap& currentMap = mgraph->getDisplayedPointMap();

        AgentEngine& eng = mgraph->getAgentEngine();

        // set up eng here...
        if (!eng.size()) {
           eng.push_back(AgentSet());
        }

        eng.m_timesteps = agentP.totalSystemTimestemps();
        eng.tail().m_release_rate = agentP.releaseRate();
        eng.tail().m_lifetime = agentP.agentLifeTimesteps();
        if (agentP.agentFOV() == 32) {
           eng.tail().m_vbin = -1;
        }
        else {
           eng.tail().m_vbin = (agentP.agentFOV() - 1) / 2;
        }
        eng.tail().m_steps = agentP.agentStepsBeforeTurnDecision();
        switch(agentP.getAgentMode()) {
            case AgentParser::NONE:
            case AgentParser::STANDARD:
                eng.tail().m_sel_type = AgentProgram::SEL_STANDARD;
                break;
            case AgentParser::LOS_LENGTH:
                eng.tail().m_sel_type = AgentProgram::SEL_LOS;
                break;
            case AgentParser::OCC_LENGTH:
                eng.tail().m_sel_type = AgentProgram::SEL_LOS_OCC;
                break;
            case AgentParser::OCC_ANY:
                eng.tail().m_sel_type = AgentProgram::SEL_OCC_ALL;
                break;
            case AgentParser::OCC_GROUP_45:
                eng.tail().m_sel_type = AgentProgram::SEL_OCC_BIN45;
                break;
            case AgentParser::OCC_GROUP_60:
                eng.tail().m_sel_type = AgentProgram::SEL_OCC_BIN60;
                break;
            case AgentParser::OCC_FURTHEST:
                eng.tail().m_sel_type = AgentProgram::SEL_OCC_STANDARD;
                break;
            case AgentParser::BIN_FAR_DIST:
                eng.tail().m_sel_type = AgentProgram::SEL_OCC_WEIGHT_DIST;
                break;
            case AgentParser::BIN_ANGLE:
                eng.tail().m_sel_type = AgentProgram::SEL_OCC_WEIGHT_ANG;
                break;
            case AgentParser::BIN_FAR_DIST_ANGLE:
                eng.tail().m_sel_type = AgentProgram::SEL_OCC_WEIGHT_DIST_ANG;
                break;
            case AgentParser::BIN_MEMORY:
                eng.tail().m_sel_type = AgentProgram::SEL_OCC_MEMORY;
                break;
        }

        // if the m_release_locations is not set the locations are
        // set later by picking random pixels
        if(agentP.randomReleaseLocationSeed() >= 0) {
            eng.tail().m_release_locations_seed = agentP.randomReleaseLocationSeed();
        }
        else {
            eng.tail().m_release_locations.clear();
            for_each(agentP.getReleasePoints().begin(), agentP.getReleasePoints().end(),
                     [&eng, &currentMap](const Point2f &point)
                     ->void{eng.tail().m_release_locations.push_back(currentMap.pixelate(point, false));});
        }

        // the ui and code suggest that the results can be put on a separate
        // 'data map', but the functionality does not seem to actually be
        // there thus it is skipped for now
        // eng.m_gatelayer = m_gatelayer;

        // note, trails currently per run, but output per engine
        if (agentP.recordTrailsForAgents() == 0) {
            eng.m_record_trails = true;
            eng.m_trail_count = MAX_TRAILS;
        }
        else if (agentP.recordTrailsForAgents() > 0) {
                eng.m_record_trails = true;
                eng.m_trail_count = agentP.recordTrailsForAgents();
        }

        std::cout << "ok\nRunning agent analysis... " << std::flush;
        DO_TIMED("Running agent analysis", eng.run(comm.get(), &currentMap))
        std::cout << " ok\nWriting out result..." << std::flush;
        std::vector<AgentParser::OutputType> resultTypes = agentP.outputTypes();
        if(resultTypes.size() == 0)
        {
            // if no choice was made for an output type assume the user just
            // wants a graph file

            DO_TIMED("Writing graph", mgraph->write(cmdP.getOuputFile().c_str(),METAGRAPH_VERSION, false))
        }
        else if(resultTypes.size() == 1)
        {
            // if only one type of output is given, assume that the user has
            // correctly entered a name with the correct extension and export
            // exactly with that name and extension

            switch(resultTypes[0]) {
                case AgentParser::OutputType::GRAPH:
                {
                    DO_TIMED("Writing graph", mgraph->write(cmdP.getOuputFile().c_str(),METAGRAPH_VERSION, false))
                    break;
                }
                case AgentParser::OutputType::GATECOUNTS:
                {
                    ofstream gatecountStream(cmdP.getOuputFile().c_str());
                    DO_TIMED("Writing gatecounts", currentMap.outputSummary(gatecountStream, ','))
                    break;
                }
                case AgentParser::OutputType::TRAILS:
                {
                    ofstream trailStream(cmdP.getOuputFile().c_str());
                    DO_TIMED("Writing trails", eng.outputTrails(trailStream))
                    break;
                }
            }
        }
        else
        {
            // if more than one output type is given assume the user has given
            // a filename without an extension and thus the new file must have
            // an extension. Also to avoid name clashes in cases where the user
            // asked for outputs that would yield the same extension also add
            // a related suffix

            if(std::find(resultTypes.begin(), resultTypes.end(), AgentParser::OutputType::GRAPH) != resultTypes.end()) {
                std::string outFile = cmdP.getOuputFile() + ".graph";
                DO_TIMED("Writing graph", mgraph->write(outFile.c_str(),METAGRAPH_VERSION, false))
            }
            if(std::find(resultTypes.begin(), resultTypes.end(), AgentParser::OutputType::GATECOUNTS) != resultTypes.end()) {
                std::string outFile = cmdP.getOuputFile() + "_gatecounts.csv";
                ofstream gatecountStream(outFile.c_str());
                DO_TIMED("Writing gatecounts", currentMap.outputSummary(gatecountStream, ','))
            }
            if(std::find(resultTypes.begin(), resultTypes.end(), AgentParser::OutputType::TRAILS) != resultTypes.end()) {
                std::string outFile = cmdP.getOuputFile() + "_trails.cat";
                ofstream trailStream(outFile.c_str());
                 DO_TIMED("Writing trails", eng.outputTrails(trailStream))
            }
        }
    }

    void runIsovists(const CommandLineParser &clp, const std::vector<IsovistDefinition> &isovists, IPerformanceSink &perfWriter)
    {
        auto mGraph = loadGraph(clp.getFileName().c_str(),perfWriter);

        auto communicator = std::unique_ptr<Communicator>(new ICommunicator);
        std::cout << "Making " << isovists.size() << " isovists... "  << std::flush;
        DO_TIMED("Make isovists", std::for_each(isovists.begin(), isovists.end(),
            [&mGraph, &communicator, &clp](const IsovistDefinition &isovist)->void{
                mGraph->makeIsovist(communicator.get(), isovist.getLocation(), isovist.getLeftAngle(), isovist.getRightAngle(), clp.simpleMode());
            }))
        std::cout << " ok\nWriting out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
        std::cout << " ok" << std::endl;
    }

    void exportData(const CommandLineParser &cmdP, const ExportParser &exportP, IPerformanceSink &perfWriter ) {

        std::unique_ptr<Communicator> comm(new ICommunicator());

        auto mgraph = loadGraph(cmdP.getFileName().c_str(), perfWriter);

        PointMap& currentMap = mgraph->getDisplayedPointMap();

        switch(exportP.getExportMode()) {
            case ExportParser::POINTMAP_DATA_CSV:
            {
                ofstream stream(cmdP.getOuputFile().c_str());
                DO_TIMED("Writing pointmap data", currentMap.outputSummary(stream, ','))
                break;
            }
            case ExportParser::POINTMAP_CONNECTIONS_CSV:
            {
                ofstream stream(cmdP.getOuputFile().c_str());
                DO_TIMED("Writing pointmap connections", currentMap.outputConnectionsAsCSV(stream, ","))
                break;
            }
            default:
            {
                throw depthmapX::SetupCheckException("Error, unsupported export mode");
            }
        }
    }

    void runStepDepth(
            const CommandLineParser &clp,
            const std::vector<Point2f> &stepDepthPoints,
            IPerformanceSink &perfWriter)
    {
        auto mGraph = loadGraph(clp.getFileName().c_str(),perfWriter);

        std::cout << "ok\nSelecting cells... " << std::flush;

        for( auto & point : stepDepthPoints ) {
            auto graphRegion = mGraph->getRegion();
            if (!graphRegion.contains(point))
            {
                throw depthmapX::RuntimeException("Point outside of target region");
            }
            QtRegion r(point, point);
            mGraph->setCurSel(r, true);
        }

        std::cout << "ok\nCalculating step-depth... " << std::flush;

        Options options;
        options.global = 0;
        options.point_depth_selection = 1;

        std::unique_ptr<Communicator> comm(new ICommunicator());

        DO_TIMED("Calculating step-depth", mGraph->analyseGraph( comm.get(), options, false))

        std::cout << " ok\nWriting out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
                std::cout << " ok" << std::endl;
    }
}
