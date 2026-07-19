// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2000-2010, University College London, Alasdair Turner
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2017-2018, Petros Koutsolampros

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

#include "salalib/segmmodules/segmtulip.h"

#include "genlib/parallel.h"
#include "genlib/stringutils.h"

#include <cstdint>

bool SegmentTulip::run(Communicator *comm, ShapeGraph &map, bool) {

    if (map.getMapType() != ShapeMap::SEGMENTMAP) {
        return false;
    }

    // TODO: Understand what these parameters do. They were never truly provided in the original function
    int weighting_col2 = m_weighted_measure_col2;
    int routeweight_col = m_routeweight_col;
    bool interactive = m_interactive;

    AttributeTable &attributes = map.getAttributeTable();

    int processed_rows = 0;

    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS,
                              (m_sel_only ? map.getSelSet().size() : map.getConnections().size()));
    }

    // note: radius must be sorted lowest to highest, but if -1 occurs ("radius n") it needs to be last...
    // ...to ensure no mess ups, we'll re-sort here:
    bool radius_n = false;
    std::vector<double> radius_unconverted;
    for (int radius : m_radius_set) {
        if (radius == -1.0) {
            radius_n = true;
        } else {
            radius_unconverted.push_back(radius);
        }
    }
    if (radius_n) {
        radius_unconverted.push_back(-1.0);
    }

    // retrieve weighted col data, as this may well be overwritten in the new analysis:
    std::vector<float> weights;
    std::vector<float> routeweights; // EF
    std::string weighting_col_text;

    int tulip_bins = m_tulip_bins;

    if (m_weighted_measure_col != -1) {
        weighting_col_text = attributes.getColumnName(m_weighted_measure_col);
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights.push_back(map.getAttributeRowFromShapeIndex(i).getValue(m_weighted_measure_col));
        }
    } else { // Normal run // TV
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights.push_back(1.0f);
        }
    }
    // EF routeweight*
    std::string routeweight_col_text;
    if (routeweight_col != -1) {
        // we normalise the column values between 0 and 1 and reverse it so that high values can be treated as a 'low
        // cost' - similar to the angular cost
        double max_value = attributes.getColumn(routeweight_col).getStats().max;
        routeweight_col_text = attributes.getColumnName(routeweight_col);
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            routeweights.push_back(1.0 - (map.getAttributeRowFromShapeIndex(i).getValue(routeweight_col) /
                                          max_value)); // scale and revert!
        }
    } else { // Normal run // TV
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            routeweights.push_back(1.0f);
        }
    }
    //*EF routeweight

    // EFEF*
    // for origin-destination weighting
    std::vector<float> weights2;
    std::string weighting_col_text2;
    if (weighting_col2 != -1) {
        weighting_col_text2 = attributes.getColumnName(weighting_col2);
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights2.push_back(map.getAttributeRowFromShapeIndex(i).getValue(weighting_col2));
        }
    } else { // Normal run // TV
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights2.push_back(1.0f);
        }
    }
    //*EFEF

    std::string tulip_text = std::string("T") + dXstring::formatString(tulip_bins, "%d");

    // first enter the required attribute columns:
    size_t r;
    for (r = 0; r < radius_unconverted.size(); r++) {
        std::string radius_text = makeRadiusText(m_radius_type, radius_unconverted[r]);
        if (m_choice) {
            // EF routeweight *
            if (routeweight_col != -1) {
                std::string choice_col_text =
                    tulip_text + " Choice [Route weight by " + routeweight_col_text + "]" + radius_text;
                attributes.insertOrResetColumn(choice_col_text.c_str());
                if (m_weighted_measure_col != -1) {
                    std::string w_choice_col_text = tulip_text + " Choice [[Route weight by " + routeweight_col_text +
                                                    "][" + weighting_col_text + " Wgt]]" + radius_text;

                    attributes.insertOrResetColumn(w_choice_col_text.c_str());
                }
                // EFEF*
                if (weighting_col2 != -1) {
                    std::string w_choice_col_text2 = tulip_text + " Choice [[Route weight by " + routeweight_col_text +
                                                     "][" + weighting_col_text + "-" + weighting_col_text2 + " Wgt]]" +
                                                     radius_text;

                    attributes.insertOrResetColumn(w_choice_col_text2.c_str());
                }
                //*EFEF
            }
            //*EF routeweight
            else { // Normal run // TV
                std::string choice_col_text = tulip_text + " Choice" + radius_text;
                attributes.insertOrResetColumn(choice_col_text.c_str());
                if (m_weighted_measure_col != -1) {
                    std::string w_choice_col_text =
                        tulip_text + " Choice [" + weighting_col_text + " Wgt]" + radius_text;
                    attributes.insertOrResetColumn(w_choice_col_text.c_str());
                }
                // EFEF*
                if (weighting_col2 != -1) {
                    std::string w_choice_col_text2 = tulip_text + " Choice [" + weighting_col_text + "-" +
                                                     weighting_col_text2 + " Wgt]" + radius_text;
                    attributes.insertOrResetColumn(w_choice_col_text2.c_str());
                }
                //*EFEF
            }
        }

        // EF routeweight *
        if (routeweight_col != -1) {
            std::string integ_col_text = tulip_text + " Integration [Route weight by " + routeweight_col_text + "]" +
                                         radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_integ_col_text = tulip_text + " Integration [[Route weight by " + routeweight_col_text +
                                           "][" + weighting_col_text + " Wgt]]" + radius_text;

            std::string count_col_text = tulip_text + " Node Count [Route weight by " + routeweight_col_text + "]" +
                                         radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string td_col_text = tulip_text + " Total Depth [Route weight by " + routeweight_col_text + "]" +
                                      radius_text; // <- note, the fact this is a tulip is unnecessary
            // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
            std::string w_td_text = tulip_text + " Total Depth [[Route weight by " + routeweight_col_text + "][" +
                                    weighting_col_text + " Wgt]]" + radius_text;
            std::string total_weight_text = tulip_text + " Total " + weighting_col_text + " [Route weight by " +
                                            routeweight_col_text + "]" + radius_text;

            attributes.insertOrResetColumn(integ_col_text.c_str());
            attributes.insertOrResetColumn(count_col_text.c_str());
            attributes.insertOrResetColumn(td_col_text.c_str());
            if (m_weighted_measure_col != -1) {
                attributes.insertOrResetColumn(w_integ_col_text.c_str());
                attributes.insertOrResetColumn(w_td_text.c_str());
                attributes.insertOrResetColumn(total_weight_text.c_str());
            }
        }
        //*EF routeweight
        else { // Normal run // TV
            std::string integ_col_text =
                tulip_text + " Integration" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_integ_col_text = tulip_text + " Integration [" + weighting_col_text + " Wgt]" + radius_text;

            std::string count_col_text =
                tulip_text + " Node Count" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string td_col_text =
                tulip_text + " Total Depth" + radius_text; // <- note, the fact this is a tulip is unnecessary
            // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
            std::string w_td_text = tulip_text + " Total Depth [" + weighting_col_text + " Wgt]" + radius_text;
            std::string total_weight_text = tulip_text + " Total " + weighting_col_text + radius_text;

            attributes.insertOrResetColumn(integ_col_text.c_str());
            attributes.insertOrResetColumn(count_col_text.c_str());
            attributes.insertOrResetColumn(td_col_text.c_str());
            if (m_weighted_measure_col != -1) {
                attributes.insertOrResetColumn(w_integ_col_text.c_str());
                attributes.insertOrResetColumn(w_td_text.c_str());
                attributes.insertOrResetColumn(total_weight_text.c_str());
            }
        }
    }
    std::vector<int> choice_col, w_choice_col, w_choice_col2, count_col, integ_col, w_integ_col, td_col, w_td_col,
        total_weight_col;
    // then look them up! eek....
    for (r = 0; r < radius_unconverted.size(); r++) {
        std::string radius_text = makeRadiusText(m_radius_type, radius_unconverted[r]);
        if (m_choice) {
            // EF routeweight *
            if (routeweight_col != -1) {
                std::string choice_col_text =
                    tulip_text + " Choice [Route weight by " + routeweight_col_text + "]" + radius_text;
                choice_col.push_back(attributes.getColumnIndex(choice_col_text.c_str()));
                if (m_weighted_measure_col != -1) {
                    std::string w_choice_col_text = tulip_text + " Choice [[Route weight by " + routeweight_col_text +
                                                    "][" + weighting_col_text + " Wgt]]" + radius_text;
                    w_choice_col.push_back(attributes.getColumnIndex(w_choice_col_text.c_str()));
                }
                // EFEF*
                if (weighting_col2 != -1) {
                    std::string w_choice_col_text2 = tulip_text + " Choice [[Route weight by " + routeweight_col_text +
                                                     "][" + weighting_col_text + "-" + weighting_col_text2 + " Wgt]]" +
                                                     radius_text;
                    w_choice_col2.push_back(attributes.getColumnIndex(w_choice_col_text2.c_str()));
                }
                //*EFEF
            }
            //* EF routeweight
            else { // Normal run // TV
                std::string choice_col_text = tulip_text + " Choice" + radius_text;
                choice_col.push_back(attributes.getColumnIndex(choice_col_text.c_str()));
                if (m_weighted_measure_col != -1) {
                    std::string w_choice_col_text =
                        tulip_text + " Choice [" + weighting_col_text + " Wgt]" + radius_text;
                    w_choice_col.push_back(attributes.getColumnIndex(w_choice_col_text.c_str()));
                }
                // EFEF*
                if (weighting_col2 != -1) {
                    std::string w_choice_col_text2 = tulip_text + " Choice [" + weighting_col_text + "-" +
                                                     weighting_col_text2 + " Wgt]" + radius_text;
                    w_choice_col2.push_back(attributes.getColumnIndex(w_choice_col_text2.c_str()));
                }
                //*EFEF
            }
        }
        // EF routeweight *
        if (routeweight_col != -1) {
            std::string integ_col_text = tulip_text + " Integration [Route weight by " + routeweight_col_text + "]" +
                                         radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_integ_col_text = tulip_text + " Integration [[Route weight by " + routeweight_col_text +
                                           "][" + weighting_col_text + " Wgt]]" + radius_text;

            std::string count_col_text = tulip_text + " Node Count [Route weight by " + routeweight_col_text + "]" +
                                         radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string td_col_text = tulip_text + " Total Depth [Route weight by " + routeweight_col_text + "]" +
                                      radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_td_text = tulip_text + " Total Depth [[Route weight by " + routeweight_col_text + "][" +
                                    weighting_col_text + " Wgt]]" + radius_text;
            std::string total_weight_col_text = tulip_text + " Total " + weighting_col_text + " [Route weight by " +
                                                routeweight_col_text + "]" + radius_text;

            integ_col.push_back(attributes.getColumnIndex(integ_col_text.c_str()));
            count_col.push_back(attributes.getColumnIndex(count_col_text.c_str()));
            td_col.push_back(attributes.getColumnIndex(td_col_text.c_str()));
            if (m_weighted_measure_col != -1) {
                // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
                w_integ_col.push_back(attributes.getColumnIndex(w_integ_col_text.c_str()));
                w_td_col.push_back(attributes.getColumnIndex(w_td_text.c_str()));
                total_weight_col.push_back(attributes.getColumnIndex(total_weight_col_text.c_str()));
            }
        }
        //* EF routeweight
        else { // Normal run // TV
            std::string integ_col_text =
                tulip_text + " Integration" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_integ_col_text = tulip_text + " Integration [" + weighting_col_text + " Wgt]" + radius_text;

            std::string count_col_text =
                tulip_text + " Node Count" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string td_col_text =
                tulip_text + " Total Depth" + radius_text; // <- note, the fact this is a tulip is unnecessary
            std::string w_td_text = tulip_text + " Total Depth [" + weighting_col_text + " Wgt]" + radius_text;
            std::string total_weight_col_text = tulip_text + " Total " + weighting_col_text + radius_text;

            integ_col.push_back(attributes.getColumnIndex(integ_col_text.c_str()));
            count_col.push_back(attributes.getColumnIndex(count_col_text.c_str()));
            td_col.push_back(attributes.getColumnIndex(td_col_text.c_str()));
            if (m_weighted_measure_col != -1) {
                // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
                w_integ_col.push_back(attributes.getColumnIndex(w_integ_col_text.c_str()));
                w_td_col.push_back(attributes.getColumnIndex(w_td_text.c_str()));
                total_weight_col.push_back(attributes.getColumnIndex(total_weight_col_text.c_str()));
            }
        }
    }

    tulip_bins /= 2; // <- actually use semicircle of tulip bins
    tulip_bins += 1;

    std::vector<std::vector<SegmentData>> bins(tulip_bins);

    // TODO: Replace these with STL
    AnalysisInfo ***audittrail;
    unsigned int **uncovered;
    audittrail = new AnalysisInfo **[map.getConnections().size()];
    uncovered = new unsigned int *[map.getConnections().size()];
    for (size_t i = 0; i < map.getConnections().size(); i++) {
        audittrail[i] = new AnalysisInfo *[radius_unconverted.size()];
        for (size_t j = 0; j < radius_unconverted.size(); j++) {
            audittrail[i][j] = new AnalysisInfo[2];
        }
        uncovered[i] = new unsigned int[2];
    }
    std::vector<double> radius;
    for (r = 0; r < radius_unconverted.size(); r++) {
        if (m_radius_type == Options::RADIUS_ANGULAR && radius_unconverted[r] != -1) {
            radius.push_back(floor(radius_unconverted[r] * tulip_bins * 0.5));
        } else {
            radius.push_back(radius_unconverted[r]);
        }
    }
    // entered once for each segment
    int length_col = attributes.getColumnIndex("Segment Length");
    std::vector<float> lengths;
    if (length_col != -1) {
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            AttributeRow& row = map.getAttributeRowFromShapeIndex(i);
            lengths.push_back(row.getValue(length_col));
        }
    }

    int radiussize = radius.size();
    int radiusmask = 0;
    for (int i = 0; i < radiussize; i++) {
        radiusmask |= (1 << i);
    }

    // Choice accumulates data across all origins and therefore keeps the original
    // serial implementation below. Without Choice, each origin is independent and can
    // safely run with private traversal state on a worker thread.
    if (!m_choice) {
        const size_t shapeCount = map.getConnections().size();
        std::vector<size_t> roots;
        roots.reserve(shapeCount);
        for (size_t cursor = 0; cursor < shapeCount; ++cursor) {
            if (!m_sel_only || map.getAttributeRowFromShapeIndex(cursor).isSelected()) {
                roots.push_back(cursor);
            }
        }

        struct TulipResult {
            double nodeCount = 0.0;
            double totalDepth = 0.0;
            double totalWeight = 0.0;
            double totalWeightedDepth = 0.0;
        };

        struct TulipWorkerState {
            std::vector<std::vector<SegmentData>> bins;
            std::vector<unsigned int> uncovered;
            std::vector<int> depths;

            TulipWorkerState(size_t binCount, size_t shapeCount, size_t radiusCount)
                : bins(binCount), uncovered(shapeCount * 2), depths(shapeCount * radiusCount * 2) {}
        };

        const size_t resultStride = static_cast<size_t>(radiussize);
        std::vector<TulipResult> results(roots.size() * resultStride);

        const size_t bytesPerWorker =
            shapeCount * 2 * sizeof(unsigned int) +
            shapeCount * resultStride * 2 * sizeof(int) +
            static_cast<size_t>(tulip_bins) * sizeof(std::vector<SegmentData>);
        const size_t workerCount = depthmapX::configuredAnalysisThreadCount(roots.size(), bytesPerWorker);

        std::vector<TulipWorkerState> workerStates;
        workerStates.reserve(workerCount);
        for (size_t worker = 0; worker < workerCount; ++worker) {
            workerStates.emplace_back(static_cast<size_t>(tulip_bins), shapeCount, resultStride);
        }

        auto depthIndex = [resultStride](size_t ref, size_t radiusIndex, int dir) {
            return ((ref * resultStride + radiusIndex) * 2) + static_cast<size_t>(dir);
        };
        auto uncoveredIndex = [](size_t ref, int dir) {
            return ref * 2 + static_cast<size_t>(dir);
        };

        depthmapX::parallelFor(roots.size(), workerCount, comm, [&](size_t task, size_t workerIndex) {
            const size_t cursor = roots[task];
            TulipWorkerState &state = workerStates[workerIndex];

            for (std::vector<SegmentData> &bin : state.bins) {
                bin.clear();
            }
            std::fill(state.uncovered.begin(), state.uncovered.end(), static_cast<unsigned int>(radiusmask));
            std::fill(state.depths.begin(), state.depths.end(), 0);

            const double rootseglength = lengths[cursor];
            SegmentData rootData(0, static_cast<int>(cursor), SegmentRef(), 0,
                                 static_cast<float>(0.5 * rootseglength), radiusmask);
            auto rootIter = std::lower_bound(state.bins[0].begin(), state.bins[0].end(), rootData);
            if (rootIter == state.bins[0].end() || rootData != *rootIter) {
                state.bins[0].insert(rootIter, rootData);
            }

            int depthlevel = 0;
            int opencount = 1;
            size_t currentbin = 0;
            size_t processed = 0;
            while (opencount) {
                if (comm && ((processed++ & 0x3ff) == 0) && comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }

                while (state.bins[currentbin].empty()) {
                    ++depthlevel;
                    ++currentbin;
                    if (currentbin == static_cast<size_t>(tulip_bins)) {
                        currentbin = 0;
                    }
                }

                SegmentData lineindex = state.bins[currentbin].back();
                state.bins[currentbin].pop_back();
                --opencount;

                const int ref = lineindex.ref;
                const int dir = (lineindex.dir == 1) ? 0 : 1;
                const unsigned int coverage =
                    lineindex.coverage & state.uncovered[uncoveredIndex(static_cast<size_t>(ref), dir)];
                if (coverage == 0) {
                    continue;
                }

                int rbin = 0;
                int rbinbase = 0;
                if (lineindex.previous.ref != -1) {
                    state.uncovered[uncoveredIndex(static_cast<size_t>(ref), dir)] &= ~coverage;
                    while (((coverage >> rbin) & 0x1U) == 0U) {
                        ++rbin;
                    }
                    rbinbase = rbin;
                    while (rbin < radiussize) {
                        if (((coverage >> rbin) & 0x1U) == 1U) {
                            state.depths[depthIndex(static_cast<size_t>(ref), static_cast<size_t>(rbin), dir)] =
                                depthlevel;
                        }
                        ++rbin;
                    }
                } else {
                    state.uncovered[uncoveredIndex(static_cast<size_t>(ref), 0)] &= ~coverage;
                    state.uncovered[uncoveredIndex(static_cast<size_t>(ref), 1)] &= ~coverage;
                }

                const Connector &line = map.getConnections()[static_cast<size_t>(ref)];
                auto addConnections = [&](const std::map<SegmentRef, float> &connections, char previousDirection) {
                    for (const auto &segconn : connections) {
                        rbin = rbinbase;
                        const SegmentRef conn = segconn.first;
                        const int connDir = (conn.dir == 1) ? 0 : 1;
                        if ((state.uncovered[uncoveredIndex(static_cast<size_t>(conn.ref), connDir)] & coverage) == 0) {
                            continue;
                        }

                        const int extradepth = routeweight_col != -1
                                                   ? static_cast<int>(floor(segconn.second * tulip_bins * 0.5 *
                                                                            routeweights[static_cast<size_t>(conn.ref)]))
                                                   : static_cast<int>(floor(segconn.second * tulip_bins * 0.5));
                        const float seglength = lengths[static_cast<size_t>(conn.ref)];

                        switch (m_radius_type) {
                        case Options::RADIUS_ANGULAR:
                            while (rbin != radiussize && radius[static_cast<size_t>(rbin)] != -1 &&
                                   depthlevel + extradepth > static_cast<int>(radius[static_cast<size_t>(rbin)])) {
                                ++rbin;
                            }
                            break;
                        case Options::RADIUS_METRIC:
                            while (rbin != radiussize && radius[static_cast<size_t>(rbin)] != -1 &&
                                   lineindex.metricdepth + seglength * 0.5 > radius[static_cast<size_t>(rbin)]) {
                                ++rbin;
                            }
                            break;
                        case Options::RADIUS_STEPS:
                            if (rbin != radiussize && radius[static_cast<size_t>(rbin)] != -1 &&
                                lineindex.segdepth >= static_cast<int>(radius[static_cast<size_t>(rbin)])) {
                                ++rbin;
                            }
                            break;
                        }

                        if ((coverage >> rbin) != 0U) {
                            SegmentData next(conn, SegmentRef(previousDirection, lineindex.ref),
                                             lineindex.segdepth + 1, lineindex.metricdepth + seglength,
                                             (coverage >> rbin) << rbin);
                            const size_t bin =
                                (currentbin + static_cast<size_t>(tulip_bins) + static_cast<size_t>(extradepth)) %
                                static_cast<size_t>(tulip_bins);
                            depthmapX::insert_sorted(state.bins[bin], next);
                            ++opencount;
                        }
                    }
                };

                if (lineindex.dir != -1) {
                    addConnections(line.m_forward_segconns, 1);
                }
                if (lineindex.dir != 1) {
                    addConnections(line.m_back_segconns, -1);
                }
            }

            for (int radiusIndex = 0; radiusIndex < radiussize; ++radiusIndex) {
                TulipResult result;
                for (size_t ref = 0; ref < shapeCount; ++ref) {
                    const bool reached0 =
                        ((state.uncovered[uncoveredIndex(ref, 0)] >> radiusIndex) & 0x1U) == 0U;
                    const bool reached1 =
                        ((state.uncovered[uncoveredIndex(ref, 1)] >> radiusIndex) & 0x1U) == 0U;
                    if (!reached0 && !reached1) {
                        continue;
                    }

                    int reachedDir;
                    if (reached0 && reached1) {
                        const int depth0 = state.depths[depthIndex(ref, static_cast<size_t>(radiusIndex), 0)];
                        const int depth1 = state.depths[depthIndex(ref, static_cast<size_t>(radiusIndex), 1)];
                        reachedDir = depth0 < depth1 ? 0 : 1;
                    } else {
                        reachedDir = reached0 ? 0 : 1;
                    }

                    const int reachedDepth =
                        state.depths[depthIndex(ref, static_cast<size_t>(radiusIndex), reachedDir)];
                    result.nodeCount += 1.0;
                    result.totalDepth += reachedDepth;
                    result.totalWeight += weights[ref];
                    result.totalWeightedDepth += reachedDepth * weights[ref];
                }

                result.totalDepth /= ((tulip_bins - 1.0f) * 0.5f);
                result.totalWeightedDepth /= ((tulip_bins - 1.0f) * 0.5f);
                results[task * resultStride + static_cast<size_t>(radiusIndex)] = result;
            }
        });

        for (size_t task = 0; task < roots.size(); ++task) {
            AttributeRow &row = map.getAttributeRowFromShapeIndex(roots[task]);
            for (size_t radiusIndex = 0; radiusIndex < resultStride; ++radiusIndex) {
                const TulipResult &result = results[task * resultStride + radiusIndex];
                row.setValue(count_col[radiusIndex], static_cast<float>(result.nodeCount));
                if (result.nodeCount > 1.0) {
                    row.setValue(td_col[radiusIndex], static_cast<float>(result.totalDepth));
                    if (m_weighted_measure_col != -1) {
                        row.setValue(total_weight_col[radiusIndex], static_cast<float>(result.totalWeight));
                        row.setValue(w_td_col[radiusIndex], static_cast<float>(result.totalWeightedDepth));
                    }
                } else {
                    row.setValue(td_col[radiusIndex], -1.0f);
                    if (m_weighted_measure_col != -1) {
                        row.setValue(total_weight_col[radiusIndex], -1.0f);
                        row.setValue(w_td_col[radiusIndex], -1.0f);
                    }
                }

                if (result.totalDepth > 1e-9) {
                    row.setValue(integ_col[radiusIndex],
                                 static_cast<float>(result.nodeCount * result.nodeCount / result.totalDepth));
                    if (m_weighted_measure_col != -1) {
                        row.setValue(w_integ_col[radiusIndex],
                                     static_cast<float>(result.totalWeight * result.totalWeight /
                                                        result.totalWeightedDepth));
                    }
                } else {
                    row.setValue(integ_col[radiusIndex], -1.0f);
                    if (m_weighted_measure_col != -1) {
                        row.setValue(w_integ_col[radiusIndex], -1.0f);
                    }
                }
            }
        }

        map.setDisplayedAttribute(-2);
        map.setDisplayedAttribute(td_col.back());
        return !roots.empty();
    }

    for (size_t cursor = 0; cursor < map.getConnections().size(); cursor++) {
        AttributeRow &row =
            map.getAttributeRowFromShapeIndex(cursor);

        if (m_sel_only) {
            // could use m_selection_set.searchindex(rowid) to find
            // if this row is selected as m_selection_set is ordered for axial and segment maps, etc
            // BUT, actually quicker to check the tag in the attributes that shows it's selected
            if (!row.isSelected()) {
                continue;
            }
        }

        for (int k = 0; k < tulip_bins; k++) {
            bins[k].clear();
        }
        for (size_t j = 0; j < map.getConnections().size(); j++) {
            for (int dir = 0; dir < 2; dir++) {
                for (int k = 0; k < radiussize; k++) {
                    audittrail[j][k][dir].clearLine();
                }
                uncovered[j][dir] = radiusmask;
            }
        }

        double rootseglength = row.getValue(length_col);
        double rootweight = (m_weighted_measure_col != -1) ? weights[cursor] : 0.0;

        // setup: direction 0 (both ways), segment i, previous -1, segdepth (step depth) 0, metricdepth 0.5 *
        // rootseglength, bin 0
        SegmentData segmentData(0, cursor, SegmentRef(), 0, 0.5 * rootseglength, radiusmask);
        auto it = std::lower_bound(bins[0].begin(), bins[0].end(), segmentData);
        if (it == bins[0].end() || segmentData != *it) {
            bins[0].insert(it, segmentData);
        }
        // this version below is only designed to be used temporarily --
        // could be on an option?
        // bins[0].push_back(SegmentData(0,rowid,SegmentRef(),0,0.0,radiusmask));
        int depthlevel = 0;
        int opencount = 1;
        size_t currentbin = 0;
        while (opencount) {
            while (!bins[currentbin].size()) {
                depthlevel++;
                currentbin++;
                if (currentbin == static_cast<size_t>(tulip_bins)) {
                    currentbin = 0;
                }
            }
            SegmentData lineindex = bins[currentbin].back();
            bins[currentbin].pop_back();
            //
            opencount--;

            int ref = lineindex.ref;
            int dir = (lineindex.dir == 1) ? 0 : 1;
            int coverage = lineindex.coverage & uncovered[ref][dir];
            if (coverage != 0) {
                int rbin = 0;
                int rbinbase;
                if (lineindex.previous.ref != -1) {
                    uncovered[ref][dir] &= ~coverage;
                    while (((coverage >> rbin) & 0x1) == 0)
                        rbin++;
                    rbinbase = rbin;
                    while (rbin < radiussize) {
                        if (((coverage >> rbin) & 0x1) == 1) {
                            audittrail[ref][rbin][dir].depth = depthlevel;
                            audittrail[ref][rbin][dir].previous = lineindex.previous;
                            audittrail[lineindex.previous.ref][rbin][(lineindex.previous.dir == 1) ? 0 : 1].leaf =
                                false;
                        }
                        rbin++;
                    }
                } else {
                    rbinbase = 0;
                    uncovered[ref][0] &= ~coverage;
                    uncovered[ref][1] &= ~coverage;
                }
                Connector &line = map.getConnections()[ref];
                float seglength;
                int extradepth;
                if (lineindex.dir != -1) {
                    for (auto &segconn : line.m_forward_segconns) {
                        rbin = rbinbase;
                        SegmentRef conn = segconn.first;
                        if ((uncovered[conn.ref][(conn.dir == 1 ? 0 : 1)] & coverage) != 0) {
                            // EF routeweight*
                            if (routeweight_col != -1) { // EF here we do the weighting of the angular cost by the
                                                         // weight of the next segment
                                // note that the content of the routeweights array is scaled between 0 and 1 and is
                                // reversed
                                // such that: = 1.0-(attributes.getValue(i, routeweight_col)/max_value)
                                extradepth = (int)floor(segconn.second * tulip_bins * 0.5 * routeweights[conn.ref]);
                            }
                            //*EF routeweight
                            else {
                                extradepth = (int)floor(segconn.second * tulip_bins * 0.5);
                            }
                            seglength = lengths[conn.ref];
                            switch (m_radius_type) {
                            case Options::RADIUS_ANGULAR:
                                while (rbin != radiussize && radius[rbin] != -1 &&
                                       depthlevel + extradepth > (int)radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case Options::RADIUS_METRIC:
                                while (rbin != radiussize && radius[rbin] != -1 &&
                                       lineindex.metricdepth + seglength * 0.5 > radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case Options::RADIUS_STEPS:
                                if (rbin != radiussize && radius[rbin] != -1 &&
                                    lineindex.segdepth >= (int)radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            }
                            if ((coverage >> rbin) != 0) {
                                SegmentData sd(conn, SegmentRef(1, lineindex.ref), lineindex.segdepth + 1,
                                               lineindex.metricdepth + seglength, (coverage >> rbin) << rbin);
                                size_t bin = (currentbin + tulip_bins + extradepth) % tulip_bins;
                                depthmapX::insert_sorted(bins[bin], sd);
                                opencount++;
                            }
                        }
                    }
                }
                if (lineindex.dir != 1) {
                    for (auto &segconn : line.m_back_segconns) {
                        rbin = rbinbase;
                        SegmentRef conn = segconn.first;
                        if ((uncovered[conn.ref][(conn.dir == 1 ? 0 : 1)] & coverage) != 0) {
                            // EF routeweight*
                            if (routeweight_col != -1) { // EF here we do the weighting of the angular cost by the
                                                         // weight of the next segment
                                // note that the content of the routeweights array is scaled between 0 and 1 and is
                                // reversed
                                // such that: = 1.0-(attributes.getValue(i, routeweight_col)/max_value)
                                extradepth = (int)floor(segconn.second * tulip_bins * 0.5 * routeweights[conn.ref]);
                            }
                            //*EF routeweight
                            else {
                                extradepth = (int)floor(segconn.second * tulip_bins * 0.5);
                            }
                            seglength = lengths[conn.ref];
                            switch (m_radius_type) {
                            case Options::RADIUS_ANGULAR:
                                while (rbin != radiussize && radius[rbin] != -1 &&
                                       depthlevel + extradepth > (int)radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case Options::RADIUS_METRIC:
                                while (rbin != radiussize && radius[rbin] != -1 &&
                                       lineindex.metricdepth + seglength * 0.5 > radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case Options::RADIUS_STEPS:
                                if (rbin != radiussize && radius[rbin] != -1 &&
                                    lineindex.segdepth >= (int)radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            }
                            if ((coverage >> rbin) != 0) {
                                SegmentData sd(conn, SegmentRef(-1, lineindex.ref), lineindex.segdepth + 1,
                                               lineindex.metricdepth + seglength, (coverage >> rbin) << rbin);
                                size_t bin = (currentbin + tulip_bins + extradepth) % tulip_bins;
                                depthmapX::insert_sorted(bins[bin], sd);
                                opencount++;
                            }
                        }
                    }
                }
            }
        }
        // set the attributes for this node:
        for (int k = 0; k < radiussize; k++) {
            // note, curs_total_depth must use double as mantissa can get too long for int in large systems
            double curs_node_count = 0.0, curs_total_depth = 0.0;
            double curs_total_weight = 0.0, curs_total_weighted_depth = 0.0;
            size_t j;
            for (j = 0; j < map.getConnections().size(); j++) {
                // find dir according
                bool m0 = ((uncovered[j][0] >> k) & 0x1) == 0;
                bool m1 = ((uncovered[j][1] >> k) & 0x1) == 0;
                if ((m0 | m1) != 0) {
                    int dir;
                    if (m0 & m1) {
                        // dir is the one with the lowest depth:
                        if (audittrail[j][k][0].depth < audittrail[j][k][1].depth)
                            dir = 0;
                        else
                            dir = 1;
                    } else {
                        // dir is simply the one that's filled in:
                        dir = m0 ? 0 : 1;
                    }
                    curs_node_count++;
                    curs_total_depth += audittrail[j][k][dir].depth;
                    curs_total_weight += weights[j];
                    curs_total_weighted_depth += audittrail[j][k][dir].depth * weights[j];
                    //
                    if (m_choice && audittrail[j][k][dir].leaf) {
                        // note, graph may be directed (e.g., for one way streets), so both ways must be included from
                        // now on:
                        SegmentRef here = SegmentRef(dir == 0 ? 1 : -1, j);
                        if (here.ref != static_cast<int>(cursor)) {
                            int choicecount = 0;
                            double choiceweight = 0.0;
                            // EFEF*
                            double choiceweight2 = 0.0;
                            //*EFEF
                            while (here.ref != static_cast<int>(cursor)) { // not rowid means not the current root for the path
                                int heredir = (here.dir == 1) ? 0 : 1;
                                // each node has the existing choicecount and choiceweight from previously encountered
                                // nodes added to it
                                audittrail[here.ref][k][heredir].choice += choicecount;
                                // nb, weighted values calculated anyway to save time on 'if'
                                audittrail[here.ref][k][heredir].weighted_choice += choiceweight;
                                // EFEF*
                                audittrail[here.ref][k][heredir].weighted_choice2 += choiceweight2;
                                //*EFEF
                                // if the node hasn't been encountered before, the choicecount and choiceweight is
                                // incremented for all remaining nodes to be encountered on the backwards route from it
                                if (!audittrail[here.ref][k][heredir].choicecovered) {
                                    // this node has not been encountered before: this adds the choicecount and weight
                                    // for this node, and flags it as visited
                                    choicecount++;
                                    choiceweight += weights[here.ref] * rootweight;
                                    // EFEF*
                                    choiceweight2 += weights2[here.ref] * rootweight; // rootweight!
                                    //*EFEF

                                    audittrail[here.ref][k][heredir].choicecovered = true;
                                    // note, for weighted choice, the start and end points have choice added to them:
                                    if (m_weighted_measure_col != -1) {
                                        audittrail[here.ref][k][heredir].weighted_choice +=
                                            (weights[here.ref] * rootweight) / 2.0;
                                        // EFEF*
                                        if (weighting_col2 != -1) {
                                            audittrail[here.ref][k][heredir].weighted_choice2 +=
                                                (weights2[here.ref] * rootweight) / 2.0; // rootweight!
                                        }
                                        //*EFEF
                                    }
                                }
                                here = audittrail[here.ref][k][heredir].previous;
                            }
                            // note, for weighted choice, the start and end points have choice added to them:
                            // (this is the summed weight for all starting nodes encountered in this path)
                            if (m_weighted_measure_col != -1) {
                                audittrail[here.ref][k][(here.dir == 1) ? 0 : 1].weighted_choice += choiceweight / 2.0;
                                // EFEF*
                                if (weighting_col2 != -1) {
                                    audittrail[here.ref][k][(here.dir == 1) ? 0 : 1].weighted_choice2 +=
                                        choiceweight2 / 2.0;
                                }
                                //*EFEF
                            }
                        }
                    }
                }
            }
            double total_depth_conv = curs_total_depth / ((tulip_bins - 1.0f) * 0.5f);
            double total_weighted_depth_conv = curs_total_weighted_depth / ((tulip_bins - 1.0f) * 0.5f);
            //
            row.setValue(count_col[k], float(curs_node_count));
            if (curs_node_count > 1) {
                // for dmap 8 and above, mean depth simply isn't calculated as for radius measures it is meaningless
                row.setValue(td_col[k], total_depth_conv);
                if (m_weighted_measure_col != -1) {
                    row.setValue(total_weight_col[k], float(curs_total_weight));
                    row.setValue(w_td_col[k], float(total_weighted_depth_conv));
                }
            } else {
                row.setValue(td_col[k], -1);
                if (m_weighted_measure_col != -1) {
                    row.setValue(total_weight_col[k], -1.0f);
                    row.setValue(w_td_col[k], -1.0f);
                }
            }
            // for dmap 10 an above, integration is included!
            if (total_depth_conv > 1e-9) {
                row.setValue(integ_col[k], (float)(curs_node_count * curs_node_count / total_depth_conv));
                if (m_weighted_measure_col != -1) {
                    row.setValue(w_integ_col[k],
                                 (float)(curs_total_weight * curs_total_weight / total_weighted_depth_conv));
                }
            } else {
                row.setValue(integ_col[k], -1);
                if (m_weighted_measure_col != -1) {
                    row.setValue(w_integ_col[k], -1.0f);
                }
            }
        }
        //
        processed_rows++;
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    // interactive is usual Depthmap: throw an exception if cancelled
                    if (interactive) {
                        for (size_t i = 0; i < map.getConnections().size(); i++) {
                            for (size_t j = 0; j < size_t(radiussize); j++) {
                                delete[] audittrail[i][j];
                            }
                            delete[] audittrail[i];
                            delete[] uncovered[i];
                        }
                        delete[] audittrail;
                        delete[] uncovered;
                        throw Communicator::CancelledException();
                    } else {
                        // in non-interactive mode, retain what's been processed already
                        break;
                    }
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, cursor);
            }
        }
    }
    if (m_choice) {
        for (size_t cursor = 0; cursor < map.getConnections().size(); cursor++) {
            AttributeRow &row =
                attributes.getRow(AttributeKey(depthmapX::getMapAtIndex(map.getAllShapes(), cursor)->first));
            for (size_t r = 0; r < radius.size(); r++) {
                // according to Eva's correction, total choice and total weighted choice
                // should already have been accumulated by radius at this stage
                double total_choice = audittrail[cursor][r][0].choice + audittrail[cursor][r][1].choice;
                double total_weighted_choice =
                    audittrail[cursor][r][0].weighted_choice + audittrail[cursor][r][1].weighted_choice;
                // EFEF*
                double total_weighted_choice2 =
                    audittrail[cursor][r][0].weighted_choice2 + audittrail[cursor][r][1].weighted_choice2;
                //*EFEF

                // normalised choice now excluded for two reasons:
                // a) not useful measure, b) in parallel calculations, cannot be calculated at this stage
                // n.b., it is possible through the front end: the new choice takes into account bidirectional routes,
                // so it should be normalised according to (n-1)(n-2) (maximum possible through routes) not
                // (n-1)(n-2)/2 the relativised segment length weighted choice equation was
                // (total_seg_length*total_seg_length-seg_length*seg_length)/2 again, drop the divide by 2 for the new
                // implementation
                //
                //
                row.setValue(choice_col[r], float(total_choice));
                if (m_weighted_measure_col != -1) {
                    row.setValue(w_choice_col[r], float(total_weighted_choice));
                    // EFEF*
                    if (weighting_col2 != -1) {
                        row.setValue(w_choice_col2[r], float(total_weighted_choice2));
                    }
                    //*EFEF
                }
            }
        }
    }
    for (size_t i = 0; i < map.getConnections().size(); i++) {
        for (int j = 0; j < radiussize; j++) {
            delete[] audittrail[i][j];
        }
        delete[] audittrail[i];
        delete[] uncovered[i];
    }
    delete[] audittrail;
    delete[] uncovered;

    map.setDisplayedAttribute(-2); // <- override if it's already showing
    if (m_choice) {
        map.setDisplayedAttribute(choice_col.back());
    } else {
        map.setDisplayedAttribute(td_col.back());
    }
    return processed_rows > 0;
}
