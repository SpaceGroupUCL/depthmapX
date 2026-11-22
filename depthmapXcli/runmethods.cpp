// Copyright (C) 2017 Christian Sailer
// Copyright (C) 2011-2026, Tasos Varoudis
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
#include "printcommunicator.h"
#include <memory>
#include <sstream>
#include <vector>
#include "salalib/entityparsing.h"
#include <salalib/gridproperties.h>
#include <salalib/importutils.h>

namespace dm_runmethods
{
    std::unique_ptr<MetaGraph> loadGraph(const std::string& filename, IPerformanceSink &perfWriter) {
        std::unique_ptr<MetaGraph> mgraph(new MetaGraph);
        std::cout << "Loading graph " << filename << std::flush;
        DO_TIMED( "Load graph file", auto result = mgraph->readFromFile(filename);)
        if ( result != MetaGraph::OK)
        {
            std::stringstream message;
            message << "Failed to load graph from file " << filename << ", error " << result << std::flush;
            throw depthmapX::RuntimeException(message.str().c_str());
        }
        std::cout << " ok\n" << std::flush;
        return mgraph;
    }

    std::unique_ptr<Communicator> getCommunicator(const CommandLineParser &clp) {
        if (clp.printProgress()) {
            return std::unique_ptr<Communicator>(new PrintCommunicator());
        }
        return nullptr;
    }

    static void requireCompleted(bool completed, const Communicator *comm, const char *operation) {
        if (!completed) {
            Communicator::checkCancelled(comm);
            throw depthmapX::RuntimeException(std::string(operation) + " did not complete");
        }
    }

    void importFiles(const CommandLineParser &cmdP, const ImportParser &parser, IPerformanceSink &perfWriter)
    {
        std::ifstream mainFileStream(cmdP.getFileName().c_str());
        if(!mainFileStream.good()) {
            std::stringstream message;
            message << "File not found: " << cmdP.getFileName() << std::flush;
            throw depthmapX::RuntimeException(message.str().c_str());
        }

        std::unique_ptr<MetaGraph> mgraph(new MetaGraph);
        DO_TIMED( "Load graph file", auto result = mgraph->readFromFile(cmdP.getFileName());)
        if ( result != MetaGraph::OK && result != MetaGraph::NOT_A_GRAPH)
        {
            std::stringstream message;
            message << "Failed to load graph from file " << cmdP.getFileName() << ", error " << result << std::flush;
            throw depthmapX::RuntimeException(message.str().c_str());
        }

        if ( result == MetaGraph::NOT_A_GRAPH)
        {
            // not a graph, try to import the file
            std::string ext = cmdP.getFileName().substr(cmdP.getFileName().length() - 4, cmdP.getFileName().length() - 1);
            std::ifstream file(cmdP.getFileName());

            depthmapX::ImportFileType importFileType = depthmapX::ImportFileType::TSV;
            if(dXstring::toLower(ext) == ".csv") {
                importFileType = depthmapX::ImportFileType::CSV;
            } else if (dXstring::toLower(ext) == ".dxf") {
                importFileType = depthmapX::ImportFileType::DXF;
            }

            depthmapX::importFile(*mgraph,
                                  file,
                                  getCommunicator(cmdP).get(),
                                  cmdP.getFileName(),
                                  parser.getImportMapType(),
                                  importFileType);
        } else if ( result == MetaGraph::OK) {
            if(parser.toImportAsAttrbiutes()) {

                if(mgraph->getDisplayedMapType() == ShapeMap::EMPTYMAP) {
                    throw depthmapX::RuntimeException("No map displayed to attach attributes to");
                }

                std::vector<std::string> fileNames = parser.getFilesToImport();
                for(std::string fileName: fileNames) {
                    std::string ext = fileName.substr(fileName.length() - 4, fileName.length() - 1);
                    std::ifstream file(fileName);
                    char delimiter = '\t';
                    if(dXstring::toLower(ext) == ".csv") {
                        delimiter = ',';
                    }

                    DO_TIMED("Importing attributes", depthmapX::importAttributes(mgraph->getDisplayedMapAttributes(),
                                                                                 file,
                                                                                 delimiter);)
                }
            }
        }
        DO_TIMED("Writing graph", mgraph->write(cmdP.getOuputFile().c_str(),METAGRAPH_VERSION, false);)
    }

    void linkGraph(const CommandLineParser &cmdP, const LinkParser &parser, IPerformanceSink &perfWriter)
    {

        auto mgraph = loadGraph(cmdP.getFileName().c_str(), perfWriter);

        if (parser.getLinkMode() == LinkParser::LinkMode::UNLINK
                && parser.getMapTypeGroup() == LinkParser::MapTypeGroup::SHAPEGRAPHS
                && mgraph->getDisplayedShapeGraph().getMapType() != ShapeMap::AXIALMAP) {
            throw depthmapX::RuntimeException("Unlinking is only available for axial maps and pointmaps");
        }

        char delimiter = '\t';
        std::stringstream linksStream;
        if(!parser.getLinksFile().empty())
        {
            std::ifstream fileStream(parser.getLinksFile());
            if (!linksStream)
            {
                std::stringstream message;
                message << "Failed to load file " << parser.getLinksFile() << ", error " << std::flush;
                throw depthmapX::RuntimeException(message.str().c_str());
            }
            linksStream << fileStream.rdbuf();
            fileStream.close();
        }
        else if(!parser.getManualLinks().empty())
        {
            delimiter = ',';
            std::string header = "x1,y1,x2,y2";
            if(parser.getLinkType() == LinkParser::LinkType::REFS) {
                header = "reffrom,refto";
            } else if(parser.getLinkMode() == LinkParser::LinkMode::UNLINK) {
                header = "x,y";
            }
            linksStream << header;
            auto iter = parser.getManualLinks().begin(),
                    end = parser.getManualLinks().end();
            for ( ; iter != end; ++iter )
            {
                linksStream << "\n" << *iter;
            }
        }

        SimpleTimer t;
        if(parser.getLinkMode() == LinkParser::LinkMode::LINK) {
            if(parser.getMapTypeGroup() == LinkParser::MapTypeGroup::SHAPEGRAPHS) {
                auto& shapeGraph = mgraph->getDisplayedShapeGraph();
                if(parser.getLinkType() == LinkParser::LinkType::COORDS) {
                    std::vector<Line> mergeLines = EntityParsing::parseLines(linksStream, delimiter);
                    for(auto line: mergeLines) {
                        QtRegion region(line.start(), line.start());
                        shapeGraph.setCurSel(region);
                        shapeGraph.linkShapes(line.end());
                    }
                } else {
                    auto mergePairs = EntityParsing::parseRefPairs(linksStream, delimiter);
                    for(auto pair: mergePairs) {
                        // apparently this also unlinks if already linked or crossing
                        shapeGraph.linkShapesFromRefs(pair.first, pair.second);
                    }
                }
            } else {
                std::vector<PixelRefPair> newLinks;
                PointMap& currentMap = mgraph->getDisplayedPointMap();
                if(parser.getLinkType() == LinkParser::LinkType::COORDS) {
                    std::vector<Line> mergeLines = EntityParsing::parseLines(linksStream, delimiter);
                    std::vector<PixelRefPair> linkPairsFromCoords = depthmapX::pixelateMergeLines(mergeLines, currentMap);
                    newLinks.insert(newLinks.end(), linkPairsFromCoords.begin(), linkPairsFromCoords.end());
                } else {
                    auto mergePairs = EntityParsing::parseRefPairs(linksStream, delimiter);
                    for(auto pair: mergePairs) {
                        newLinks.push_back(PixelRefPair(pair.first, pair.second));
                    }
                }
                depthmapX::mergePixelPairs(newLinks, currentMap);
            }
        } else {
            if(parser.getMapTypeGroup() == LinkParser::MapTypeGroup::SHAPEGRAPHS) {
                auto& shapeGraph = mgraph->getDisplayedShapeGraph();
                if(parser.getLinkType() == LinkParser::LinkType::COORDS) {
                    auto mergePoints = EntityParsing::parsePoints(linksStream, delimiter);
                    for(auto point: mergePoints) {
                        shapeGraph.unlinkAtPoint(point);
                    }
                } else {
                    auto mergePairs = EntityParsing::parseRefPairs(linksStream, delimiter);
                    for(auto pair: mergePairs) {
                        shapeGraph.unlinkShapesFromRefs(pair.first, pair.second);
                    }
                }
            } else {
                std::vector<PixelRefPair> newLinks;
                PointMap& currentMap = mgraph->getDisplayedPointMap();
                if(parser.getLinkType() == LinkParser::LinkType::COORDS) {
                    std::vector<Line> mergeLines = EntityParsing::parseLines(linksStream, delimiter);
                    std::vector<PixelRefPair> linkPairsFromCoords = depthmapX::pixelateMergeLines(mergeLines, currentMap);
                    newLinks.insert(newLinks.end(), linkPairsFromCoords.begin(), linkPairsFromCoords.end());
                } else {
                    auto mergePairs = EntityParsing::parseRefPairs(linksStream, delimiter);
                    for(auto pair: mergePairs) {
                        newLinks.push_back(PixelRefPair(pair.first, pair.second));
                    }
                }
                depthmapX::unmergePixelPairs(newLinks, currentMap);
            }
        }

        perfWriter.addData("Linking graph", t.getTimeInSeconds());
        DO_TIMED("Writing graph", mgraph->write(cmdP.getOuputFile().c_str(),METAGRAPH_VERSION, false);)
    }

    void runVga(const CommandLineParser &cmdP, const VgaParser &vgaP, const IRadiusConverter &converter, IPerformanceSink &perfWriter)
    {
        auto mgraph = loadGraph(cmdP.getFileName().c_str(), perfWriter);

        std::unique_ptr<Options> options(new Options());

        std::cout << "Getting options..." << std::flush;
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
        std::cout << " ok\nAnalysing graph..." << std::flush;

        if (!mgraph->viewingProcessedPoints()) {
            throw depthmapX::RuntimeException("VGA requires a displayed visibility graph");
        }
        auto comm = getCommunicator(cmdP);
        DO_TIMED("Run VGA", requireCompleted(mgraph->analyseGraph(comm.get(), *options, cmdP.simpleMode()),
                                            comm.get(), "VGA"))
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
        graph.makePoints(point, 0, nullptr);
    }

    void runVisualPrep(
            const CommandLineParser &clp,
            double gridSize, const std::vector<Point2f> &fillPoints,
            double maxVisibility,
            bool boundaryGraph,
            bool makeGraph,
            bool unmakeGraph,
            bool removeLinksWhenUnmaking,
            IPerformanceSink &perfWriter)
    {
        auto mGraph = loadGraph(clp.getFileName().c_str(),perfWriter);

        std::cout << "Initial checks... " << std::flush;
        auto state = mGraph->getState();
        if (~state & MetaGraph::LINEDATA)
        {
            throw depthmapX::RuntimeException("Graph must have line data before preparing VGA");
        }
        if(gridSize > 0) {
            // Create a new pointmap and set tha grid
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
            mGraph->addNewPointMap();
            DO_TIMED("Setting grid", mGraph->setGrid(gridSize, Point2f(0.0, 0.0)))
        } else if(mGraph->getPointMaps().empty()) {
            std::stringstream message;
            message << "No map exists to use. Please create a new one by providing a grid size" << std::flush;
            throw depthmapX::RuntimeException(message.str());
        }

        if(unmakeGraph) {
            if(!mGraph->getDisplayedPointMap().isProcessed()) {
                std::stringstream message;
                message << "Current map has not had its graph made so there's nothing to unmake" << std::flush;
                throw depthmapX::RuntimeException(message.str());
            }
            DO_TIMED("Unmaking graph", mGraph->getDisplayedPointMap().unmake(removeLinksWhenUnmaking))
        } else {
            if(fillPoints.size() > 0) {
                std::cout << "ok\nFilling grid... " << std::flush;
                DO_TIMED("Filling grid",
                         for_each(fillPoints.begin(), fillPoints.end(), [&mGraph](const Point2f &point)->void{fillGraph(*mGraph, point);}))
            }
            if(makeGraph) {
                std::cout << "ok\nMaking graph... " << std::flush;
                DO_TIMED("Making graph", mGraph->makeGraph(getCommunicator(clp).get(), boundaryGraph ? 1 : 0, maxVisibility))
            }
        }

        std::cout << " ok\nWriting out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
                std::cout << " ok" << std::endl;
    }

    void runAxialAnalysis(const CommandLineParser &clp, const AxialParser &ap, IPerformanceSink &perfWriter)
    {
        auto mGraph = loadGraph(clp.getFileName().c_str(), perfWriter);

        auto comm = getCommunicator(clp);
        auto state = mGraph->getState();
        if ( ap.runAllLines())
        {
           if (~state & MetaGraph::LINEDATA)
           {
               throw depthmapX::RuntimeException("Line drawing must be loaded before axial map can be constructed");
           }
           std::cout << "Making all line map... " << std::flush;
           DO_TIMED("Making all axes map", for_each (ap.getAllAxesRoots().begin(),ap.getAllAxesRoots().end(),
                                                     [&](const Point2f &point) { requireCompleted(mGraph->makeAllLineMap(comm.get(), point), comm.get(), "All-line map construction"); } ))
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
            DO_TIMED("Fewest line map", requireCompleted(mGraph->makeFewestLineMap(comm.get(), 1), comm.get(), "Fewest-line map construction"))
            std::cout << "ok" << std::endl;
        }

        if (ap.runAnalysis())
        {
            if (!mGraph->viewingProcessedLines() || !mGraph->getDisplayedShapeGraph().isAxialMap()) {
                throw depthmapX::RuntimeException("Axial analysis requires a displayed axial map");
            }
            std::cout << "Running axial analysis... " << std::flush;
            Options options;
            const std::vector<double>& radii = ap.getRadii();
            options.radius_set.insert(radii.begin(), radii.end());
            options.choice = ap.useChoice();
            options.local = ap.useLocal();
            options.fulloutput = ap.calculateRRA();
            options.weighted_measure_col = -1;

            if(!ap.getAttribute().empty()) {
                const ShapeGraph& map = mGraph->getDisplayedShapeGraph();
                const AttributeTable& table = map.getAttributeTable();
                for (int i = 0; i < table.getNumColumns(); i++) {
                    if(ap.getAttribute() == table.getColumnName(i).c_str()) {
                        options.weighted_measure_col = i;
                    }
                }
                if(options.weighted_measure_col == -1) {
                    throw depthmapX::RuntimeException("Given attribute (" + ap.getAttribute() +
                                                      ") does not exist in currently selected map");
                }
            }
            DO_TIMED("Axial analysis", requireCompleted(mGraph->analyseAxial(comm.get(), options, clp.simpleMode()), comm.get(), "Axial analysis"))
            std::cout << "ok\n" << std::flush;

        }
        std::cout << "Writing out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
        std::cout << " ok" << std::endl;

    }

    void runSegmentAnalysis(const CommandLineParser &clp, const SegmentParser &sp, IPerformanceSink &perfWriter)
    {
        auto mGraph = loadGraph(clp.getFileName().c_str(), perfWriter);

        if (!mGraph->viewingProcessedLines() || !mGraph->getDisplayedShapeGraph().isSegmentMap()) {
            throw depthmapX::RuntimeException("Segment analysis requires a displayed segment map");
        }
        auto comm = getCommunicator(clp);

        std::cout << "Running segment analysis... " << std::flush;
        Options options;
        const std::vector<double>& radii = sp.getRadii();
        options.radius_set.insert(radii.begin(), radii.end());
        options.choice = sp.includeChoice();
        options.tulip_bins = sp.getTulipBins();
        options.weighted_measure_col = -1;

        if(!sp.getAttribute().empty()) {
            const ShapeGraph& map = mGraph->getDisplayedShapeGraph();
            const AttributeTable& table = map.getAttributeTable();
            for (int i = 0; i < table.getNumColumns(); i++) {
                if(sp.getAttribute() == table.getColumnName(i).c_str()) {
                    options.weighted_measure_col = i;
                }
            }
            if(options.weighted_measure_col == -1) {
                throw depthmapX::RuntimeException("Given attribute (" + sp.getAttribute() +
                                                  ") does not exist in currently selected map");
            }
        }

        switch(sp.getRadiusType()) {
            case SegmentParser::RadiusType::SEGMENT_STEPS: {
                options.radius_type = Options::RADIUS_STEPS;
                break;
            }
            case SegmentParser::RadiusType::METRIC: {
                options.radius_type = Options::RADIUS_METRIC;
                break;
            }
            case SegmentParser::RadiusType::ANGULAR: {
                options.radius_type = Options::RADIUS_ANGULAR;
                break;
            }
            case SegmentParser::RadiusType::NONE:
                break;
        }
        switch(sp.getAnalysisType()) {
            case SegmentParser::AnalysisType::ANGULAR_TULIP: {
                DO_TIMED("Segment tulip analysis", requireCompleted(mGraph->analyseSegmentsTulip(comm.get(), options), comm.get(), "Segment analysis"))
                break;
            }
            case SegmentParser::AnalysisType::ANGULAR_FULL: {
                DO_TIMED("Segment angular analysis", requireCompleted(mGraph->analyseSegmentsAngular(comm.get(), options), comm.get(), "Segment analysis"))
                break;
            }
            case SegmentParser::AnalysisType::TOPOLOGICAL: {
                options.output_type = 0;
                DO_TIMED("Segment topological", requireCompleted(mGraph->analyseTopoMetMultipleRadii(comm.get(), options), comm.get(), "Segment analysis"))
                break;
            }
            case SegmentParser::AnalysisType::METRIC: {
                options.output_type = 1;
                DO_TIMED("Segment metric", requireCompleted(mGraph->analyseTopoMetMultipleRadii(comm.get(), options), comm.get(), "Segment analysis"))
                break;
            }
            case SegmentParser::AnalysisType::NONE:
                throw depthmapX::RuntimeException("No segment analysis type given");
        }
        std::cout << "ok\n" << std::flush;

        std::cout << "Writing out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
        std::cout << " ok" << std::endl;

    }

    void runAgentAnalysis(const CommandLineParser &cmdP, const AgentParser &agentP, IPerformanceSink &perfWriter) {

        auto mgraph = loadGraph(cmdP.getFileName().c_str(), perfWriter);

        PointMap& currentMap = mgraph->getDisplayedPointMap();

        AgentEngine& eng = mgraph->getAgentEngine();

        // set up eng here...
        if (!eng.agentSets.size()) {
           eng.agentSets.push_back(AgentSet());
        }

        eng.m_timesteps = agentP.totalSystemTimestemps();
        eng.agentSets.back().m_release_rate = agentP.releaseRate();
        eng.agentSets.back().m_lifetime = agentP.agentLifeTimesteps();
        if (agentP.agentFOV() == 32) {
           eng.agentSets.back().m_vbin = -1;
        }
        else {
           eng.agentSets.back().m_vbin = (agentP.agentFOV() - 1) / 2;
        }
        eng.agentSets.back().m_steps = agentP.agentStepsBeforeTurnDecision();
        switch(agentP.getAgentMode()) {
            case AgentParser::NONE:
            case AgentParser::STANDARD:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_STANDARD;
                break;
            case AgentParser::LOS_LENGTH:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_LOS;
                break;
            case AgentParser::OCC_LENGTH:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_LOS_OCC;
                break;
            case AgentParser::OCC_ANY:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_OCC_ALL;
                break;
            case AgentParser::OCC_GROUP_45:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_OCC_BIN45;
                break;
            case AgentParser::OCC_GROUP_60:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_OCC_BIN60;
                break;
            case AgentParser::OCC_FURTHEST:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_OCC_STANDARD;
                break;
            case AgentParser::BIN_FAR_DIST:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_OCC_WEIGHT_DIST;
                break;
            case AgentParser::BIN_ANGLE:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_OCC_WEIGHT_ANG;
                break;
            case AgentParser::BIN_FAR_DIST_ANGLE:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_OCC_WEIGHT_DIST_ANG;
                break;
            case AgentParser::BIN_MEMORY:
                eng.agentSets.back().m_sel_type = AgentProgram::SEL_OCC_MEMORY;
                break;
        }

        // if the m_release_locations is not set the locations are
        // set later by picking random pixels
        if(agentP.randomReleaseLocationSeed() >= 0) {
            eng.agentSets.back().m_release_locations_seed = agentP.randomReleaseLocationSeed();
        }
        else {
            eng.agentSets.back().m_release_locations.clear();
            for_each(agentP.getReleasePoints().begin(), agentP.getReleasePoints().end(),
                     [&eng, &currentMap](const Point2f &point)
                     ->void{eng.agentSets.back().m_release_locations.push_back(currentMap.pixelate(point, false));});
        }

        // the ui and code suggest that the results can be put on a separate
        // 'data map', but the functionality does not seem to actually be
        // there thus it is skipped for now
        // eng.m_gatelayer = m_gatelayer;

        // note, trails currently per run, but output per engine
        if (agentP.recordTrailsForAgents() == 0) {
            eng.m_record_trails = true;
        }
        else if (agentP.recordTrailsForAgents() > 0) {
                eng.m_record_trails = true;
                eng.m_trail_count = agentP.recordTrailsForAgents();
        }

        std::cout << "ok\nRunning agent analysis... " << std::flush;
        DO_TIMED("Running agent analysis", eng.run(getCommunicator(cmdP).get(), &currentMap))
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
                    std::ofstream gatecountStream(cmdP.getOuputFile().c_str());
                    DO_TIMED("Writing gatecounts", currentMap.outputSummary(gatecountStream, ','))
                    break;
                }
                case AgentParser::OutputType::TRAILS:
                {
                    std::ofstream trailStream(cmdP.getOuputFile().c_str());
                    ShapeMap trailMap("Agent Trails");
                    eng.insertTrailsInMap(trailMap);
                    DO_TIMED("Writing trails", mgraph->writeMapShapesAsCat(trailMap, trailStream))
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
                std::ofstream gatecountStream(outFile.c_str());
                DO_TIMED("Writing gatecounts", currentMap.outputSummary(gatecountStream, ','))
            }
            if(std::find(resultTypes.begin(), resultTypes.end(), AgentParser::OutputType::TRAILS) != resultTypes.end()) {
                std::string outFile = cmdP.getOuputFile() + "_trails.cat";
                std::ofstream trailStream(outFile.c_str());
                ShapeMap trailMap("Agent Trails");
                eng.insertTrailsInMap(trailMap);
                DO_TIMED("Writing trails", mgraph->writeMapShapesAsCat(trailMap, trailStream))
            }
        }
    }

    void runIsovists(const CommandLineParser &clp, const std::vector<IsovistDefinition> &isovists, IPerformanceSink &perfWriter)
    {
        auto mGraph = loadGraph(clp.getFileName().c_str(),perfWriter);

        std::cout << "Making " << isovists.size() << " isovists... "  << std::flush;
        DO_TIMED("Make isovists", std::for_each(isovists.begin(), isovists.end(),
            [&mGraph, &clp](const IsovistDefinition &isovist)->void{
                mGraph->makeIsovist(getCommunicator(clp).get(), isovist.getLocation(), isovist.getLeftAngle(), isovist.getRightAngle(), clp.simpleMode());
            }))
        std::cout << " ok\nWriting out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
        std::cout << " ok" << std::endl;
    }

    void exportData(const CommandLineParser &cmdP, const ExportParser &exportP, IPerformanceSink &perfWriter ) {

        auto mgraph = loadGraph(cmdP.getFileName().c_str(), perfWriter);

        switch(exportP.getExportMode()) {
            case ExportParser::POINTMAP_DATA_CSV:
            {
                PointMap& currentMap = mgraph->getDisplayedPointMap();
                std::ofstream stream(cmdP.getOuputFile().c_str());
                DO_TIMED("Writing pointmap data", currentMap.outputSummary(stream, ','))
                stream.close();
                break;
            }
            case ExportParser::POINTMAP_CONNECTIONS_CSV:
            {
                PointMap& currentMap = mgraph->getDisplayedPointMap();
                std::ofstream stream(cmdP.getOuputFile().c_str());
                DO_TIMED("Writing pointmap connections", currentMap.outputConnectionsAsCSV(stream, ","))
                stream.close();
                break;
            }
            case ExportParser::POINTMAP_LINKS_CSV:
            {
                PointMap& currentMap = mgraph->getDisplayedPointMap();
                std::ofstream stream(cmdP.getOuputFile().c_str());
                DO_TIMED("Writing pointmap connections", currentMap.outputLinksAsCSV(stream, ","))
                stream.close();
                break;
            }
            case ExportParser::SHAPEGRAPH_MAP_CSV:
            {
                ShapeGraph& currentMap = mgraph->getDisplayedShapeGraph();
                std::ofstream stream(cmdP.getOuputFile().c_str());
                DO_TIMED("Writing pointmap connections", currentMap.output(stream, ','))
                stream.close();
                break;
            }
            case ExportParser::SHAPEGRAPH_MAP_MIF:
            {
                ShapeGraph& currentMap = mgraph->getDisplayedShapeGraph();
                std::string fileName = cmdP.getOuputFile().c_str();
                std::string mifFile = fileName + ".mif";
                std::string midFile = fileName + ".mid";
                if (0 == fileName.compare(fileName.length() - 4, 4, ".mif")) {
                    // we are given the .mif
                    mifFile = fileName;
                    midFile = fileName.substr(0,  fileName.length() - 4) + ".mid";

                } else if (0 == fileName.compare(fileName.length() - 4, 4, ".mid")) {
                    // we are given the .mid
                    mifFile = fileName.substr(0, fileName.length() - 4) + ".mif";
                    midFile = fileName;
                }
                std::ofstream mifStream(mifFile);
                std::ofstream midStream(midFile);
                DO_TIMED("Writing pointmap connections", currentMap.outputMifMap(mifStream, midStream))
                mifStream.close();
                midStream.close();
                break;
            }
            case ExportParser::SHAPEGRAPH_CONNECTIONS_CSV:
            {
                ShapeGraph& currentMap = mgraph->getDisplayedShapeGraph();
                std::ofstream stream(cmdP.getOuputFile().c_str());
                DO_TIMED("Writing shapegraph connections",
                         currentMap.isAxialMap() ? currentMap.writeAxialConnectionsAsPairsCSV(stream) :
                                                   currentMap.writeSegmentConnectionsAsPairsCSV(stream))
                stream.close();
                break;
            }
            case ExportParser::SHAPEGRAPH_LINKS_UNLINKS_CSV:
            {
                ShapeGraph& currentMap = mgraph->getDisplayedShapeGraph();
                std::ofstream stream(cmdP.getOuputFile().c_str());
                DO_TIMED("Writing shapegraph links and unlinks",
                         currentMap.writeLinksUnlinksAsPairsCSV(stream))
                stream.close();
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
            const StepDepthParser::StepType &stepType,
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

        switch (stepType) {
            case StepDepthParser::StepType::ANGULAR:
                options.point_depth_selection = 3;
                break;
            case StepDepthParser::StepType::METRIC:
                options.point_depth_selection = 2;
                break;
            case StepDepthParser::StepType::VISUAL:
                options.point_depth_selection = 1;
                break;
            default: { throw depthmapX::SetupCheckException("Error, unsupported step type"); }
        }

        auto comm = getCommunicator(clp);
        DO_TIMED("Calculating step-depth", requireCompleted(mGraph->analyseGraph(comm.get(), options, false),
                                                           comm.get(), "Step depth"))

        std::cout << " ok\nWriting out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
                std::cout << " ok" << std::endl;
    }

    void runMapConversion(const CommandLineParser &clp, const MapConvertParser &mcp, IPerformanceSink &perfWriter)
    {
        auto mGraph = loadGraph(clp.getFileName().c_str(), perfWriter);

        int currentMapType = mGraph->getDisplayedMapType();

        if (currentMapType == ShapeMap::EMPTYMAP) {
            if(mGraph->hasVisibleDrawingLayers()) {
                currentMapType = ShapeMap::DRAWINGMAP;
            } else {
                throw depthmapX::RuntimeException("No currently available map to convert from");
            }
        }

        if (mcp.copyAttributes()) {
            if(currentMapType != ShapeMap::DATAMAP &&
                    currentMapType != ShapeMap::AXIALMAP &&
                    currentMapType != ShapeMap::SEGMENTMAP) {
                throw depthmapX::RuntimeException("Copying attributes is only available when "\
                                                  "converting between Data, Axial and Segment maps "\
                                                  "(current map type is not of those types)");
            }
            if(mcp.outputMapType() != ShapeMap::DATAMAP &&
                    mcp.outputMapType() != ShapeMap::AXIALMAP &&
                    mcp.outputMapType() != ShapeMap::SEGMENTMAP) {
                throw depthmapX::RuntimeException("Copying attributes is only available when "\
                                                  "converting between Data, Axial and Segment maps "\
                                                  "(selected output map type is not of those types)");
            }
        }
        if (mcp.removeStubLength() > 0) {
            if(currentMapType != ShapeMap::AXIALMAP) {
                throw depthmapX::RuntimeException("Removing stubs (-crsl) is only available when"\
                                                  "converting from Axial to Segment maps"\
                                                  "(current map type is not Axial)");
            }
            if(mcp.outputMapType() != ShapeMap::SEGMENTMAP) {
                throw depthmapX::RuntimeException("Removing stubs (-crsl) is only available when"\
                                                  "converting from Axial to Segment maps"\
                                                  "(selected output map type is not Segment)");
            }
        }

        auto comm = getCommunicator(clp);
        bool converted = false;
        switch(mcp.outputMapType()) {
        case ShapeMap::DRAWINGMAP: {
            DO_TIMED("Converting to drawing",
                     converted = mGraph->convertToDrawing(comm.get(), mcp.outputMapName(), currentMapType == ShapeMap::DATAMAP));
            break;
        }
        case ShapeMap::AXIALMAP: {
            switch(currentMapType) {
            case ShapeMap::DRAWINGMAP: {
                DO_TIMED("Converting from drawing to axial",
                         converted = mGraph->convertDrawingToAxial(comm.get(), mcp.outputMapName()));
                break;
            }
            case ShapeMap::DATAMAP: {
                DO_TIMED("Converting from data to axial",
                         converted = mGraph->convertDataToAxial(comm.get(), mcp.outputMapName(),
                                                    !mcp.removeInputMap(), mcp.copyAttributes()));
                break;
            }
            default: {
                throw depthmapX::RuntimeException("Unsupported conversion to axial");
            }
            }
            break;
        }
        case ShapeMap::SEGMENTMAP: {
            switch(currentMapType) {
            case ShapeMap::DRAWINGMAP: {
                DO_TIMED("Converting from drawing to segment",
                         converted = mGraph->convertDrawingToSegment(comm.get(), mcp.outputMapName()));
                break;
            }
            case ShapeMap::AXIALMAP: {
                DO_TIMED("Converting from axial to segment",
                         converted = mGraph->convertAxialToSegment(comm.get(), mcp.outputMapName(), !mcp.removeInputMap(),
                                                       mcp.copyAttributes(), mcp.removeStubLength() / 100.0));
                break;
            }
            case ShapeMap::DATAMAP: {
                DO_TIMED("Converting from data to segment",
                         converted = mGraph->convertDataToSegment(comm.get(), mcp.outputMapName(),
                                                      !mcp.removeInputMap(), mcp.copyAttributes()));
                break;
            }
            default: {
                throw depthmapX::RuntimeException("Unsupported conversion to segment");
            }
            }
            break;
        }
        case ShapeMap::DATAMAP: {
            DO_TIMED("Converting to data",
                     converted = mGraph->convertToData(comm.get(), mcp.outputMapName(),
                                           !mcp.removeInputMap(), currentMapType, mcp.copyAttributes()));
            break;
        }
        case ShapeMap::CONVEXMAP: {
            DO_TIMED("Converting to convex",
                     converted = mGraph->convertToConvex(comm.get(), mcp.outputMapName(),
                                             !mcp.removeInputMap(), currentMapType, mcp.copyAttributes()));
            break;
        }
        default: {
            throw depthmapX::RuntimeException("Unsupported conversion");
        }
        }

        requireCompleted(converted, comm.get(), "Map conversion");
        std::cout << " ok\nWriting out result..." << std::flush;
        DO_TIMED("Writing graph", mGraph->write(clp.getOuputFile().c_str(),METAGRAPH_VERSION, false))
                std::cout << " ok" << std::endl;
    }
}
