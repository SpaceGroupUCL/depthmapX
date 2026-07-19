#include "alllinemap.h"
#include "salalib/axialminimiser.h"
#include "salalib/tolerances.h"
#include "genlib/exceptions.h"
#include "genlib/parallel.h"
#include <time.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <utility>
#include <unordered_set>

namespace {

bool axialLinesMatch(const Line &a, const Line &b, double tolerance) {
   return approxeq(a.start(), b.start(), tolerance) && approxeq(a.end(), b.end(), tolerance);
}

void compactDuplicateAxialLinesSerial(std::vector<Line> &axiallines, KeyVertices &preaxialdata,
                                      double tolerance) {
   std::vector<bool> duplicate(axiallines.size(), false);
   for (size_t j = 0; j < axiallines.size(); ++j) {
      if (duplicate[j]) {
         continue;
      }
      for (size_t k = j + 1; k < axiallines.size(); ++k) {
         if (!duplicate[k] && axialLinesMatch(axiallines[j], axiallines[k], tolerance)) {
            preaxialdata[j].insert(preaxialdata[k].begin(), preaxialdata[k].end());
            duplicate[k] = true;
         }
      }
   }

   size_t writeIndex = 0;
   for (size_t readIndex = 0; readIndex < axiallines.size(); ++readIndex) {
      if (duplicate[readIndex]) {
         continue;
      }
      if (writeIndex != readIndex) {
         axiallines[writeIndex] = std::move(axiallines[readIndex]);
         preaxialdata[writeIndex] = std::move(preaxialdata[readIndex]);
      }
      ++writeIndex;
   }
   axiallines.resize(writeIndex);
   preaxialdata.resize(writeIndex);
}

void compactDuplicateAxialLines(std::vector<Line> &axiallines, KeyVertices &preaxialdata,
                                double tolerance) {
   const size_t lineCount = axiallines.size();
   const size_t workerCount = depthmapX::configuredAnalysisThreadCount(lineCount);
   if (workerCount == 1 || lineCount < 512 ||
       lineCount > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) {
      compactDuplicateAxialLinesSerial(axiallines, preaxialdata, tolerance);
      return;
   }

   using DuplicatePair = std::pair<uint32_t, uint32_t>;
   constexpr size_t MAX_STORED_PAIRS = size_t(4) * 1024 * 1024;
   const size_t perWorkerLimit = std::max<size_t>(1, MAX_STORED_PAIRS / workerCount);
   std::vector<std::vector<DuplicatePair>> workerPairs(workerCount);
   std::atomic_bool overflow{false};

   depthmapX::parallelFor(lineCount, workerCount, nullptr, [&](size_t j, size_t worker) {
      if (overflow.load(std::memory_order_relaxed)) {
         return;
      }
      std::vector<DuplicatePair> &pairs = workerPairs[worker];
      for (size_t k = j + 1; k < lineCount; ++k) {
         if (axialLinesMatch(axiallines[j], axiallines[k], tolerance)) {
            if (pairs.size() >= perWorkerLimit) {
               overflow.store(true, std::memory_order_relaxed);
               return;
            }
            pairs.emplace_back(static_cast<uint32_t>(j), static_cast<uint32_t>(k));
         }
      }
   });

   if (overflow.load(std::memory_order_relaxed)) {
      // Pathological duplicate-heavy inputs would require too much temporary
      // memory. Fall back to the exact serial algorithm instead.
      compactDuplicateAxialLinesSerial(axiallines, preaxialdata, tolerance);
      return;
   }

   size_t pairCount = 0;
   for (const auto &pairs : workerPairs) {
      pairCount += pairs.size();
   }
   std::vector<DuplicatePair> pairs;
   pairs.reserve(pairCount);
   for (auto &workerResult : workerPairs) {
      pairs.insert(pairs.end(), workerResult.begin(), workerResult.end());
   }
   std::sort(pairs.begin(), pairs.end());

   // Apply the matches in the same j/k order as the original serial loop. This
   // preserves which representative survives even though comparisons ran in parallel.
   std::vector<bool> duplicate(lineCount, false);
   size_t pairIndex = 0;
   for (size_t j = 0; j < lineCount; ++j) {
      while (pairIndex < pairs.size() && pairs[pairIndex].first < j) {
         ++pairIndex;
      }
      size_t groupEnd = pairIndex;
      while (groupEnd < pairs.size() && pairs[groupEnd].first == j) {
         ++groupEnd;
      }
      if (!duplicate[j]) {
         for (size_t index = pairIndex; index < groupEnd; ++index) {
            const size_t k = pairs[index].second;
            if (!duplicate[k]) {
               preaxialdata[j].insert(preaxialdata[k].begin(), preaxialdata[k].end());
               duplicate[k] = true;
            }
         }
      }
      pairIndex = groupEnd;
   }

   size_t writeIndex = 0;
   for (size_t readIndex = 0; readIndex < lineCount; ++readIndex) {
      if (duplicate[readIndex]) {
         continue;
      }
      if (writeIndex != readIndex) {
         axiallines[writeIndex] = std::move(axiallines[readIndex]);
         preaxialdata[writeIndex] = std::move(preaxialdata[readIndex]);
      }
      ++writeIndex;
   }
   axiallines.resize(writeIndex);
   preaxialdata.resize(writeIndex);
}

struct DivisionTaskResult {
   size_t radialIndex = std::numeric_limits<size_t>::max();
   std::vector<int> axialShapeRefs;
};

} // namespace

AllLineMap::AllLineMap(Communicator *comm,
                       std::vector<SpacePixelFile> &drawingLayers,
                       const Point2f& seed,
                       const std::string& name):
    ShapeGraph(name, ShapeMap::ALLLINEMAP)
{
   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 3 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }
   // this has a nasty habit of crashing if reused...
   // reset everything at the top level, including any existing all-line map:
   m_polygons.clear();
   m_poly_connections.clear();
   m_radial_lines.clear();

   // starting off... finding a polygon...
   // for ease, I'm just going to make a construction line set from all the visible lines...

   QtRegion region;

   std::vector<Line> lines;

   // add all visible layers to the set of polygon lines...
   for (const auto& pixelGroup: drawingLayers) {
      for (const auto& pixel: pixelGroup.m_spacePixels) {
         if (pixel.isShown()) {
            if (region.atZero()) {
               region = pixel.getRegion();
            }
            else {
               region = runion(region, pixel.getRegion());
            }
            std::vector<SimpleLine> newLines = pixel.getAllShapesAsLines();
            for (const auto& line: newLines) {
               lines.push_back(Line(line.start(), line.end()));
            }
         }
      }
   }

   region.grow(1.30);
   m_polygons.init(lines, region);
   m_polygons.m_handled_list.clear();

   // find a corner visible from the seed:
   AxialVertexKey seedvertex = m_polygons.seedVertex( seed );

   if (seedvertex == NoVertex) {
      // oops... can't find a visible vertex
       throw depthmapX::RuntimeException("No visible vertices found");
   }

   // okay, we've got as far as finding a seed corner, now the real fun begins...
   // test outwards from corner, add other corners to
   // test set...
   std::vector<Line> axiallines;
   KeyVertices preaxialdata;
   // also poly_connections used in fewest line axial map construction:
   m_poly_connections.clear();
   m_radial_lines.clear();

   AxialVertex vertex = m_polygons.makeVertex(seedvertex, seed);
   if (!vertex.m_initialised) {
      // oops... can't init for some reason
      throw depthmapX::RuntimeException("Failed to initialise axial vertices");
   }


   time_t atime = 0;
   int count = 0;
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_polygons.m_vertex_possibles.size() );
   }

   std::set<AxialVertex> openvertices;
   openvertices.insert(vertex);
   while (!openvertices.empty()) {
      m_polygons.makeAxialLines(openvertices, axiallines, preaxialdata, m_poly_connections, m_radial_lines);
      count++;
      //
      if (comm) {
         if (qtimer( atime, 500 )) {
            if (comm->IsCancelled()) {
               throw Communicator::CancelledException();
            }
            comm->CommPostMessage( Communicator::CURRENT_RECORD, count );
         }
      }

   }

   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 3 );
      comm->CommPostMessage( Communicator::CURRENT_RECORD, 0 );
   }

   // Duplicate comparisons are independent and can be distributed across
   // workers. Their matches are merged in the original serial order so the
   // surviving line and key-vertex data remain deterministic.
   const double maxdim = __max(region.width(), region.height());
   compactDuplicateAxialLines(axiallines, preaxialdata, maxdim * TOLERANCE_B);

   region.grow(0.99); // <- this paired with crop code below to prevent error
   init(axiallines.size(), m_polygons.getRegion());  // used to be double density here
   initialiseAttributesAxial();
   for (size_t k = 0; k < axiallines.size(); k++) {
      axiallines[k].crop(region); // <- should be cropped anyway, but causing an error
      makeLineShape(axiallines[k]);
   }

   // n.b. make connections also initialises attributes
   // -> don't know what this was for: alllinemap.sortBins(m_poly_connections);
   makeConnections(preaxialdata);

   setKeyVertexCount(m_polygons.m_vertex_possibles.size());
}

std::tuple<std::unique_ptr<ShapeGraph>, std::unique_ptr<ShapeGraph>> AllLineMap::extractFewestLineMaps(Communicator *comm)
{

   if (comm) {
      comm->CommPostMessage( Communicator::NUM_STEPS, 2 );
      comm->CommPostMessage( Communicator::CURRENT_STEP, 1 );
   }

   pafsrand((unsigned int)time(NULL));

   // make one rld for each radial line...
   std::map<RadialKey, std::set<int> > radialdivisions;
   size_t i;
   for (auto& radial_line: m_radial_lines) {
      radialdivisions.insert(std::make_pair( (RadialKey) radial_line, std::set<int>() ));
   }

   // also, a list of radial lines cut by each axial line
   std::map<int, std::set<int> > ax_radial_cuts;
   std::map<int, std::set<int> > ax_seg_cuts;
   for (auto shape: getAllShapes()) {
      ax_radial_cuts.insert(std::make_pair(shape.first, std::set<int>()));
      ax_seg_cuts.insert(std::make_pair(shape.first, std::set<int>()));
   }

   // make divisions -- this is the slow part and the comm updates
   makeDivisions(m_poly_connections, m_radial_lines, radialdivisions, ax_radial_cuts, comm);

   // the slow part is over, we're into the final straight... reset the current record flag:
   if (comm) {
      comm->CommPostMessage( Communicator::CURRENT_STEP, 2 );
      comm->CommPostMessage( Communicator::CURRENT_RECORD, 0 );
   }

   // a little further setting up is still required...
   std::map<RadialKey,RadialSegment> radialsegs;

   auto iter = m_radial_lines.begin();
   if (iter != m_radial_lines.end()) {
       // now make radial segments from the radial lines... (note, start at 1)
       auto prevIter = m_radial_lines.begin();
       ++iter;
       for (;iter != m_radial_lines.end();) {
          if (iter->vertex == prevIter->vertex && iter->ang != prevIter->ang) {
                    radialsegs.insert(std::make_pair( (RadialKey)(*iter), (RadialSegment)(*prevIter)));
          }
          ++iter;
          ++prevIter;
       }
   }

   // and segment divisors from the axial lines...
   // TODO: (CS) Restructure this to get rid of all those brittle parallel data structure
   auto axIter = ax_radial_cuts.begin();
   auto axSeg = ax_seg_cuts.begin();
   for (i = 0; i < getAllShapes().size(); i++) {
       auto axRadCutIter = axIter->second.begin();
       if(axRadCutIter != axIter->second.end()) {
          auto axRadCutIterPrev = axIter->second.begin();
          ++axRadCutIter;
          for (size_t j = 1; j < axIter->second.size(); ++j) {
             // note similarity to loop above
             RadialKey& rk_end = m_radial_lines[size_t(*axRadCutIter)];
             RadialKey& rk_start = m_radial_lines[size_t(*axRadCutIterPrev)];
             if (rk_start.vertex == rk_end.vertex) {
                auto radialSegIter = radialsegs.find(rk_end);
                if (radialSegIter != radialsegs.end() && rk_start == radialSegIter->second.radial_b) {
                   radialSegIter->second.indices.insert(axIter->first);
                   axSeg->second.insert(std::distance(radialsegs.begin(), radialSegIter));
                }
             }
             ++axRadCutIter;
             ++axRadCutIterPrev;
          }
      }
      axIter++;
      axSeg++;
   }

   // and a little more setting up: key vertex relationships
   std::vector<std::vector<int> > keyvertexconns;
   std::vector<int> keyvertexcounts(static_cast<size_t>(m_keyvertexcount), 0);
   // this sets up a two step relationship: looks for the key vertices for all lines connected to you
   for (size_t y = 0; y < m_connectors.size(); y++) {
      keyvertexconns.push_back(std::vector<int>());
      auto &conn = keyvertexconns.back();
      Connector& axa = m_connectors[y];
      for (size_t z = 0; z < axa.m_connections.size(); z++) {
         std::set<int>& axb = m_keyvertices[axa.m_connections[z]];
         for (int axbi: axb) {
            auto res = std::lower_bound(conn.begin(), conn.end(), axbi);
            if (res  == conn.end() || axbi < *res )
            {
                conn.insert(res, axbi);
                keyvertexcounts[axbi] += 1;
            }
         }
      }
   }

   // ok, after this fairly tedious set up, we are ready to go...
   // note axradialcuts aren't required anymore...

   AxialMinimiser minimiser(*this, ax_seg_cuts.size(), radialsegs.size());

   std::vector<Line> lines_s, lines_m;

   minimiser.removeSubsets(ax_seg_cuts, radialsegs, radialdivisions, m_radial_lines, keyvertexconns, keyvertexcounts);

   // make new lines here (assumes line map has only lines)
   int k = -1;
   for (auto& shape: m_shapes) {
      k++;
      if (!minimiser.removed(k)) {
         lines_s.push_back( shape.second.getLine() );
      }
   }

   minimiser.fewestLongest(ax_seg_cuts, radialsegs, radialdivisions, m_radial_lines, keyvertexconns, keyvertexcounts);

   // make new lines here (assumes line map has only lines
   for (int k = 0; k < int(getAllShapes().size()); k++) {
      if (!minimiser.removed(k)) {
         lines_m.push_back( depthmapX::getMapAtIndex(getAllShapes(), k)->second.getLine() );
      }
   }

   std::unique_ptr<ShapeGraph> fewestlinemap_subsets(new ShapeGraph("Fewest-Line Map (Subsets)", ShapeMap::AXIALMAP));
   fewestlinemap_subsets->clearAll();
   fewestlinemap_subsets->init(int(lines_s.size()),m_polygons.getRegion());

   fewestlinemap_subsets->initialiseAttributesAxial();
   for (size_t k = 0; k < lines_s.size(); k++) {
      fewestlinemap_subsets->makeLineShape(lines_s[k]);
   }
   fewestlinemap_subsets->makeConnections();


   std::unique_ptr<ShapeGraph> fewestlinemap_minimal(new ShapeGraph("Fewest-Line Map (Minimal)", ShapeMap::AXIALMAP));
   fewestlinemap_minimal->clearAll();
   fewestlinemap_minimal->init(int(lines_m.size()),m_polygons.getRegion()); // used to have a '2' for double pixel density

   fewestlinemap_minimal->initialiseAttributesAxial();
   for (size_t k = 0; k < lines_m.size(); k++) {
      fewestlinemap_minimal->makeLineShape(lines_m[k]);
   }
   fewestlinemap_minimal->makeConnections();

   return std::make_tuple(std::move(fewestlinemap_subsets), std::move(fewestlinemap_minimal));
}

void AllLineMap::makeDivisions(const std::vector<PolyConnector>& polyconnections, const std::vector<RadialLine> &radiallines,
                               std::map<RadialKey,std::set<int> >& radialdivisions, std::map<int, std::set<int> > &axialdividers,
                               Communicator *comm)
{
   if (comm) {
      comm->CommPostMessage(Communicator::NUM_RECORDS, static_cast<int>(polyconnections.size()));
   }

   // std::distance on a std::map is linear. Cache its stable map-order index
   // once rather than walking from begin() for every polygon connector.
   std::map<RadialKey, size_t> radialIndices;
   size_t radialIndex = 0;
   for (const auto &entry : radialdivisions) {
      radialIndices.emplace(entry.first, radialIndex++);
   }

   const auto &shapesByRef = m_shapes;
   const auto &pixelShapes = m_pixel_shapes;
   const AllLineMap &readOnlyMap = *this;
   std::vector<DivisionTaskResult> taskResults(polyconnections.size());
   const size_t workerCount = depthmapX::configuredAnalysisThreadCount(polyconnections.size());

   depthmapX::parallelFor(polyconnections.size(), workerCount, comm, [&](size_t i, size_t) {
      const auto connIndexIter = radialIndices.find(polyconnections[i].key);
      if (connIndexIter == radialIndices.end() || connIndexIter->second >= radiallines.size()) {
         return;
      }

      DivisionTaskResult &result = taskResults[i];
      result.radialIndex = connIndexIter->second;
      const Line &connectionLine = polyconnections[i].line;
      const double tolerance = sqrt(TOLERANCE_A);
      const PixelRefVector pixels = readOnlyMap.pixelateLine(connectionLine);
      std::unordered_set<unsigned int> testedShapes;

      for (const PixelRef &pixel : pixels) {
         const auto &shapes = pixelShapes(static_cast<size_t>(pixel.y),
                                          static_cast<size_t>(pixel.x));
         for (const ShapeRef &shape : shapes) {
            if (!testedShapes.insert(shape.m_shape_ref).second) {
               continue;
            }

            const auto shapeIter = shapesByRef.find(static_cast<int>(shape.m_shape_ref));
            if (shapeIter == shapesByRef.end()) {
               continue;
            }
            const Line &line = shapeIter->second.getLine();
            const double lineTolerance = tolerance * line.length();
            if (!intersect_region(line, connectionLine, lineTolerance)) {
               continue;
            }

            const int intersectionType = intersect_line_distinguish(line, connectionLine, lineTolerance);
            if (intersectionType == 2 ||
                (intersectionType == 1 && radiallines[result.radialIndex].cuts(line))) {
               result.axialShapeRefs.push_back(static_cast<int>(shape.m_shape_ref));
            }
         }
      }

      std::sort(result.axialShapeRefs.begin(), result.axialShapeRefs.end());
      result.axialShapeRefs.erase(
          std::unique(result.axialShapeRefs.begin(), result.axialShapeRefs.end()),
          result.axialShapeRefs.end());
   });

   // Commit results in polygon-connection order. Only this phase mutates the
   // maps, keeping the worker phase read-only and deterministic.
   for (size_t i = 0; i < taskResults.size(); ++i) {
      const DivisionTaskResult &result = taskResults[i];
      if (result.radialIndex == std::numeric_limits<size_t>::max()) {
         continue;
      }
      auto radialDivisionIter = radialdivisions.find(polyconnections[i].key);
      if (radialDivisionIter == radialdivisions.end()) {
         continue;
      }
      for (int shapeRef : result.axialShapeRefs) {
         auto axialDividerIter = axialdividers.find(shapeRef);
         if (axialDividerIter == axialdividers.end()) {
            continue;
         }
         axialDividerIter->second.insert(static_cast<int>(result.radialIndex));
         radialDivisionIter->second.insert(shapeRef);
      }
   }
}
