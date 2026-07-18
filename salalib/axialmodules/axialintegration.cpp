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

#include "salalib/axialmodules/axialintegration.h"

#include "genlib/parallel.h"
#include "genlib/pflipper.h"
#include "genlib/stringutils.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>

bool AxialIntegration::run(Communicator *comm, ShapeGraph &map, bool simple_version) {
    // note, from 10.0, Depthmap no longer includes *self* connections on axial lines
    // self connections are stripped out on loading graph files, as well as no longer made

    if (comm) {
        comm->CommPostMessage(Communicator::NUM_RECORDS, static_cast<int>(map.getShapeCount()));
    }

    AttributeTable &attributes = map.getAttributeTable();

    // note: radius must be sorted lowest to highest, but if -1 occurs ("radius n") it needs to be last...
    // ...to ensure no mess ups, we'll re-sort here:
    bool radius_n = false;
    std::vector<int> radii;
    for (double radius : m_radius_set) {
        if (radius < 0) {
            radius_n = true;
        } else {
            radii.push_back(static_cast<int>(radius));
        }
    }
    if (radius_n) {
        radii.push_back(-1);
    }

    // retrieve weighted col data, as this may well be overwritten in the new analysis:
    std::vector<double> weights;
    std::string weighting_col_text;
    if (m_weighted_measure_col != -1) {
        weighting_col_text = attributes.getColumnName(m_weighted_measure_col);
        for (size_t i = 0; i < map.getShapeCount(); i++) {
            weights.push_back(map.getAttributeRowFromShapeIndex(i).getValue(m_weighted_measure_col));
        }
    }

    // first enter the required attribute columns:
    for (int radius : radii) {
        std::string radius_text;
        if (radius != -1) {
            radius_text = dXstring::formatString(radius, " R%d");
        }
        if (m_choice) {
            std::string choice_col_text = std::string("Choice") + radius_text;
            attributes.insertOrResetColumn(choice_col_text.c_str());
            std::string n_choice_col_text = std::string("Choice [Norm]") + radius_text;
            attributes.insertOrResetColumn(n_choice_col_text.c_str());
            if (m_weighted_measure_col != -1) {
                std::string w_choice_col_text = std::string("Choice [") + weighting_col_text + " Wgt]" + radius_text;
                attributes.insertOrResetColumn(w_choice_col_text.c_str());
                std::string nw_choice_col_text =
                    std::string("Choice [") + weighting_col_text + " Wgt][Norm]" + radius_text;
                attributes.insertOrResetColumn(nw_choice_col_text.c_str());
            }
        }

        if (!simple_version) {
            std::string entropy_col_text = std::string("Entropy") + radius_text;
            attributes.insertOrResetColumn(entropy_col_text.c_str());
        }

        std::string integ_dv_col_text = std::string("Integration [HH]") + radius_text;
        attributes.insertOrResetColumn(integ_dv_col_text.c_str());

        if (!simple_version) {
            std::string integ_pv_col_text = std::string("Integration [P-value]") + radius_text;
            attributes.insertOrResetColumn(integ_pv_col_text.c_str());
            std::string integ_tk_col_text = std::string("Integration [Tekl]") + radius_text;
            attributes.insertOrResetColumn(integ_tk_col_text.c_str());
            std::string intensity_col_text = std::string("Intensity") + radius_text;
            attributes.insertOrResetColumn(intensity_col_text.c_str());
            std::string harmonic_col_text = std::string("Harmonic Mean Depth") + radius_text;
            attributes.insertOrResetColumn(harmonic_col_text.c_str());
        }

        std::string depth_col_text = std::string("Mean Depth") + radius_text;
        attributes.insertOrResetColumn(depth_col_text.c_str());
        std::string count_col_text = std::string("Node Count") + radius_text;
        attributes.insertOrResetColumn(count_col_text.c_str());

        if (!simple_version) {
            std::string rel_entropy_col_text = std::string("Relativised Entropy") + radius_text;
            attributes.insertOrResetColumn(rel_entropy_col_text);
        }

        if (m_weighted_measure_col != -1) {
            std::string w_md_col_text = std::string("Mean Depth [") + weighting_col_text + " Wgt]" + radius_text;
            attributes.insertOrResetColumn(w_md_col_text.c_str());
            std::string total_weight_text = std::string("Total ") + weighting_col_text + radius_text;
            attributes.insertOrResetColumn(total_weight_text.c_str());
        }
        if (m_fulloutput) {
            if (!simple_version) {
                std::string penn_norm_text = std::string("RA [Penn]") + radius_text;
                attributes.insertOrResetColumn(penn_norm_text);
            }
            std::string ra_col_text = std::string("RA") + radius_text;
            attributes.insertOrResetColumn(ra_col_text.c_str());

            if (!simple_version) {
                std::string rra_col_text = std::string("RRA") + radius_text;
                attributes.insertOrResetColumn(rra_col_text.c_str());
            }

            std::string td_col_text = std::string("Total Depth") + radius_text;
            attributes.insertOrResetColumn(td_col_text.c_str());
        }
        //
    }
    if (m_local) {
        if (!simple_version) {
            attributes.insertOrResetColumn("Control");
            attributes.insertOrResetColumn("Controllability");
        }
    }
    // then look up all the columns... eek:
    std::vector<int> choice_col, n_choice_col, w_choice_col, nw_choice_col, entropy_col, integ_dv_col, integ_pv_col,
        integ_tk_col, intensity_col, depth_col, count_col, rel_entropy_col, penn_norm_col, w_depth_col,
        total_weight_col, ra_col, rra_col, td_col, harmonic_col;
    for (int radius : radii) {
        std::string radius_text;
        if (radius != -1) {
            radius_text = std::string(" R") + dXstring::formatString(int(radius), "%d");
        }
        if (m_choice) {
            std::string choice_col_text = std::string("Choice") + radius_text;
            choice_col.push_back(attributes.getColumnIndex(choice_col_text.c_str()));
            std::string n_choice_col_text = std::string("Choice [Norm]") + radius_text;
            n_choice_col.push_back(attributes.getColumnIndex(n_choice_col_text.c_str()));
            if (m_weighted_measure_col != -1) {
                std::string w_choice_col_text = std::string("Choice [") + weighting_col_text + " Wgt]" + radius_text;
                w_choice_col.push_back(attributes.getColumnIndex(w_choice_col_text.c_str()));
                std::string nw_choice_col_text =
                    std::string("Choice [") + weighting_col_text + " Wgt][Norm]" + radius_text;
                nw_choice_col.push_back(attributes.getColumnIndex(nw_choice_col_text.c_str()));
            }
        }
        if (!simple_version) {
            std::string entropy_col_text = std::string("Entropy") + radius_text;
            entropy_col.push_back(attributes.getColumnIndex(entropy_col_text.c_str()));
        }

        std::string integ_dv_col_text = std::string("Integration [HH]") + radius_text;
        integ_dv_col.push_back(attributes.getColumnIndex(integ_dv_col_text.c_str()));

        if (!simple_version) {
            std::string integ_pv_col_text = std::string("Integration [P-value]") + radius_text;
            integ_pv_col.push_back(attributes.getColumnIndex(integ_pv_col_text.c_str()));
            std::string integ_tk_col_text = std::string("Integration [Tekl]") + radius_text;
            integ_tk_col.push_back(attributes.getColumnIndex(integ_tk_col_text.c_str()));
            std::string intensity_col_text = std::string("Intensity") + radius_text;
            intensity_col.push_back(attributes.getColumnIndex(intensity_col_text.c_str()));
            std::string harmonic_col_text = std::string("Harmonic Mean Depth") + radius_text;
            harmonic_col.push_back(attributes.getColumnIndex(harmonic_col_text.c_str()));
        }

        std::string depth_col_text = std::string("Mean Depth") + radius_text;
        depth_col.push_back(attributes.getColumnIndex(depth_col_text.c_str()));
        std::string count_col_text = std::string("Node Count") + radius_text;
        count_col.push_back(attributes.getColumnIndex(count_col_text.c_str()));

        if (!simple_version) {
            std::string rel_entropy_col_text = std::string("Relativised Entropy") + radius_text;
            rel_entropy_col.push_back(attributes.getColumnIndex(rel_entropy_col_text.c_str()));
        }

        if (m_weighted_measure_col != -1) {
            std::string w_md_col_text = std::string("Mean Depth [") + weighting_col_text + " Wgt]" + radius_text;
            w_depth_col.push_back(attributes.getColumnIndex(w_md_col_text.c_str()));
            std::string total_weight_col_text = std::string("Total ") + weighting_col_text + radius_text;
            total_weight_col.push_back(attributes.getColumnIndex(total_weight_col_text.c_str()));
        }
        if (m_fulloutput) {
            std::string ra_col_text = std::string("RA") + radius_text;
            ra_col.push_back(attributes.getColumnIndex(ra_col_text.c_str()));

            if (!simple_version) {
                std::string penn_norm_text = std::string("RA [Penn]") + radius_text;
                penn_norm_col.push_back(attributes.getColumnIndex(penn_norm_text));
                std::string rra_col_text = std::string("RRA") + radius_text;
                rra_col.push_back(attributes.getColumnIndex(rra_col_text.c_str()));
            }

            std::string td_col_text = std::string("Total Depth") + radius_text;
            td_col.push_back(attributes.getColumnIndex(td_col_text.c_str()));
        }
    }
    int control_col = -1, controllability_col = -1;
    if (m_local) {
        if (!simple_version) {
            control_col = attributes.getColumnIndex("Control");
            controllability_col = attributes.getColumnIndex("Controllability");
        }
    }

    const size_t shapeCount = map.getShapeCount();
    const size_t radiusCount = radii.size();

    struct RadiusResult {
        float nodeCount = 0.0f;
        float totalWeight = -1.0f;
        float meanDepth = -1.0f;
        float weightedMeanDepth = -1.0f;
        float integrationDv = -1.0f;
        float integrationPv = -1.0f;
        float integrationTekl = -1.0f;
        float ra = -1.0f;
        float rra = -1.0f;
        float totalDepth = -1.0f;
        float penn = -1.0f;
        float entropy = -1.0f;
        float relativeEntropy = -1.0f;
        float intensity = -1.0f;
        float harmonic = -1.0f;
    };
    struct ChoiceValue {
        double choice = 0.0;
        double weightedChoice = 0.0;
    };
    struct WorkerState {
        std::vector<std::uint8_t> covered;
        std::vector<int> previous;
        std::vector<ChoiceValue> choice;

        WorkerState(size_t count, size_t radiiCount, bool calculateChoice)
            : covered(count), previous(count), choice(calculateChoice ? count * radiiCount : 0) {}
    };

    std::vector<RadiusResult> results(shapeCount * radiusCount);
    std::vector<float> controlResults(shapeCount, -1.0f);
    std::vector<float> controllabilityResults(shapeCount, -1.0f);

    const size_t estimatedBytesPerWorker =
        shapeCount * (sizeof(std::uint8_t) + sizeof(int) + (m_choice ? radiusCount * sizeof(ChoiceValue) : 0));
    const size_t workerCount = depthmapX::configuredAnalysisThreadCount(shapeCount, estimatedBytesPerWorker);
    std::vector<WorkerState> workerStates;
    workerStates.reserve(workerCount);
    for (size_t worker = 0; worker < workerCount; ++worker) {
        workerStates.emplace_back(shapeCount, radiusCount, m_choice);
    }

    depthmapX::parallelFor(shapeCount, workerCount, comm, [&](size_t root, size_t workerIndex) {
        WorkerState &state = workerStates[workerIndex];
        std::fill(state.covered.begin(), state.covered.end(), std::uint8_t(0));
        if (m_choice) {
            std::fill(state.previous.begin(), state.previous.end(), -1);
        }

        if (m_local) {
            double control = 0.0;
            const std::vector<int> &connections = map.getConnections()[root].m_connections;
            std::vector<int> totalneighbourhood;
            for (int connection : connections) {
                if (connection < 0 || static_cast<size_t>(connection) >= shapeCount) {
                    continue;
                }
                depthmapX::addIfNotExists(totalneighbourhood, connection);
                const auto &retconnectors = map.getConnections()[size_t(connection)].m_connections;
                for (int retconnector : retconnectors) {
                    depthmapX::addIfNotExists(totalneighbourhood, retconnector);
                }
                if (!retconnectors.empty()) {
                    control += 1.0 / double(retconnectors.size());
                }
            }
            if (!simple_version && !connections.empty()) {
                controlResults[root] = float(control);
                if (totalneighbourhood.size() > 1) {
                    controllabilityResults[root] =
                        float(double(connections.size()) / double(totalneighbourhood.size() - 1));
                }
            }
        }

        std::vector<int> depthcounts(1, 0);
        pflipper<std::vector<std::pair<int, int>>> foundlist;
        foundlist.a().push_back(std::make_pair(static_cast<int>(root), -1));
        state.covered[root] = 1;

        int totalDepth = 0;
        int depth = 1;
        int nodeCount = 1;
        double rootweight = 0.0;
        double totalWeight = 0.0;
        double weightedTotalDepth = 0.0;
        if (m_weighted_measure_col != -1) {
            rootweight = weights[root];
            totalWeight = rootweight;
        }

        std::minstd_rand random(static_cast<unsigned int>(0x9e3779b9U ^ static_cast<unsigned int>(root)));
        size_t processed = 0;

        for (size_t radiusIndex = 0; radiusIndex < radiusCount; ++radiusIndex) {
            const int radius = radii[radiusIndex];
            while (!foundlist.a().empty()) {
                if (comm && ((processed++ & 0x3ff) == 0) && comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }

                size_t position = foundlist.a().size() - 1;
                if (m_choice) {
                    position = static_cast<size_t>(random()) % foundlist.a().size();
                }
                const int index = foundlist.a()[position].first;
                const int previous = foundlist.a()[position].second;
                if (m_choice) {
                    state.previous[index] = previous;
                }

                const Connector &line = map.getConnections()[size_t(index)];
                for (int connection : line.m_connections) {
                    if (connection < 0 || static_cast<size_t>(connection) >= shapeCount || state.covered[connection]) {
                        continue;
                    }
                    state.covered[connection] = 1;
                    foundlist.b().push_back(std::make_pair(connection, index));

                    double weight = 0.0;
                    if (m_weighted_measure_col != -1) {
                        weight = weights[connection];
                        totalWeight += weight;
                        weightedTotalDepth += depth * weight;
                    }

                    if (m_choice && previous != -1) {
                        int here = index;
                        while (here != static_cast<int>(root) && here >= 0 && static_cast<size_t>(here) < shapeCount) {
                            ChoiceValue &choice = state.choice[size_t(here) * radiusCount + radiusIndex];
                            choice.choice += 1.0;
                            choice.weightedChoice += weight * rootweight;
                            here = state.previous[here];
                        }
                        if (m_weighted_measure_col != -1) {
                            state.choice[root * radiusCount + radiusIndex].weightedChoice +=
                                (weight * rootweight) * 0.5;
                            state.choice[size_t(connection) * radiusCount + radiusIndex].weightedChoice +=
                                (weight * rootweight) * 0.5;
                        }
                    }
                    totalDepth += depth;
                    ++nodeCount;
                    ++depthcounts.back();
                }

                if (m_choice) {
                    foundlist.a().erase(foundlist.a().begin() + static_cast<std::ptrdiff_t>(position));
                } else {
                    foundlist.a().pop_back();
                }
                if (foundlist.a().empty()) {
                    foundlist.flip();
                    ++depth;
                    depthcounts.push_back(0);
                    if (radius != -1 && depth > radius) {
                        break;
                    }
                }
            }

            RadiusResult &result = results[root * radiusCount + radiusIndex];
            result.nodeCount = float(nodeCount);
            if (m_weighted_measure_col != -1) {
                result.totalWeight = float(totalWeight);
            }

            if (nodeCount > 1) {
                const double meanDepth = double(totalDepth) / double(nodeCount - 1);
                result.meanDepth = float(meanDepth);
                if (m_weighted_measure_col != -1 && totalWeight != 0.0) {
                    result.weightedMeanDepth = float(weightedTotalDepth / totalWeight);
                }

                if (nodeCount > 2 && meanDepth > 1.0) {
                    const double ra = 2.0 * (meanDepth - 1.0) / double(nodeCount - 2);
                    const double rraD = ra / dvalue(nodeCount);
                    const double rraP = ra / dvalue(nodeCount);
                    result.integrationDv = float(1.0 / rraD);
                    if (!simple_version) {
                        result.integrationPv = float(1.0 / rraP);
                        result.integrationTekl =
                            totalDepth - nodeCount + 1 > 1 ? float(teklinteg(nodeCount, totalDepth)) : -1.0f;
                    }
                    if (m_fulloutput) {
                        result.ra = float(ra);
                        result.totalDepth = float(totalDepth);
                        if (!simple_version) {
                            result.rra = float(rraD);
                            const double dmin = nodeCount - 1;
                            const double dmax = palmtree(nodeCount, depth - 1);
                            if (dmax != dmin) {
                                result.penn = float((dmax - totalDepth) / (dmax - dmin));
                            }
                        }
                    }
                }

                if (!simple_version) {
                    double entropy = 0.0;
                    double intensity = 0.0;
                    double relativeEntropy = 0.0;
                    double factorial = 1.0;
                    double harmonicDenominator = 0.0;
                    for (size_t k = 0; k < depthcounts.size(); ++k) {
                        if (depthcounts[k] == 0) {
                            continue;
                        }
                        const double probability = double(depthcounts[k]) / double(nodeCount);
                        entropy -= probability * std::log2(probability);
                        factorial *= double(k + 1);
                        const double q =
                            (std::pow(meanDepth, double(k)) / factorial) * std::exp(-meanDepth);
                        if (q > 0.0) {
                            relativeEntropy += probability * std::log2(probability / q);
                        }
                        harmonicDenominator += 1.0 / double(depthcounts[k]);
                    }
                    if (harmonicDenominator > 0.0) {
                        result.harmonic = float(double(depthcounts.size()) / harmonicDenominator);
                    }
                    intensity = totalDepth > nodeCount ? nodeCount * entropy / (totalDepth - nodeCount) : -1.0;
                    result.entropy = float(entropy);
                    result.relativeEntropy = float(relativeEntropy);
                    result.intensity = float(intensity);
                }
            }
        }
    });

    size_t root = 0;
    for (auto &attributeIter : attributes) {
        if (comm && ((root & 0x3ff) == 0) && comm->IsCancelled()) {
            throw Communicator::CancelledException();
        }
        AttributeRow &row = attributeIter.getRow();
        if (m_local && !simple_version) {
            row.setValue(control_col, controlResults[root]);
            row.setValue(controllability_col, controllabilityResults[root]);
        }
        for (size_t radiusIndex = 0; radiusIndex < radiusCount; ++radiusIndex) {
            const RadiusResult &result = results[root * radiusCount + radiusIndex];
            row.setValue(count_col[radiusIndex], result.nodeCount);
            row.setValue(depth_col[radiusIndex], result.meanDepth);
            row.setValue(integ_dv_col[radiusIndex], result.integrationDv);
            if (m_weighted_measure_col != -1) {
                row.setValue(total_weight_col[radiusIndex], result.totalWeight);
                row.setValue(w_depth_col[radiusIndex], result.weightedMeanDepth);
            }
            if (!simple_version) {
                row.setValue(integ_pv_col[radiusIndex], result.integrationPv);
                row.setValue(integ_tk_col[radiusIndex], result.integrationTekl);
                row.setValue(entropy_col[radiusIndex], result.entropy);
                row.setValue(rel_entropy_col[radiusIndex], result.relativeEntropy);
                row.setValue(intensity_col[radiusIndex], result.intensity);
                row.setValue(harmonic_col[radiusIndex], result.harmonic);
            }
            if (m_fulloutput) {
                row.setValue(ra_col[radiusIndex], result.ra);
                row.setValue(td_col[radiusIndex], result.totalDepth);
                if (!simple_version) {
                    row.setValue(rra_col[radiusIndex], result.rra);
                    row.setValue(penn_norm_col[radiusIndex], result.penn);
                }
            }
        }
        ++root;
    }

    if (m_choice) {
        root = 0;
        for (auto &attributeIter : attributes) {
            if (comm && ((root & 0x3ff) == 0) && comm->IsCancelled()) {
                throw Communicator::CancelledException();
            }
            AttributeRow &row = attributeIter.getRow();
            double cumulativeChoice = 0.0;
            double cumulativeWeightedChoice = 0.0;
            for (size_t radiusIndex = 0; radiusIndex < radiusCount; ++radiusIndex) {
                for (size_t worker = 0; worker < workerCount; ++worker) {
                    const ChoiceValue &choice =
                        workerStates[worker].choice[root * radiusCount + radiusIndex];
                    cumulativeChoice += choice.choice;
                    cumulativeWeightedChoice += choice.weightedChoice;
                }
                const RadiusResult &result = results[root * radiusCount + radiusIndex];
                const double nodeCount = result.nodeCount;
                const double totalWeight = result.totalWeight;
                if (nodeCount > 2.0) {
                    row.setValue(choice_col[radiusIndex], float(cumulativeChoice));
                    row.setValue(n_choice_col[radiusIndex],
                                 float(2.0 * cumulativeChoice / ((nodeCount - 1.0) * (nodeCount - 2.0))));
                    if (m_weighted_measure_col != -1) {
                        row.setValue(w_choice_col[radiusIndex], float(cumulativeWeightedChoice));
                        row.setValue(nw_choice_col[radiusIndex],
                                     totalWeight != 0.0
                                         ? float(2.0 * cumulativeWeightedChoice / (totalWeight * totalWeight))
                                         : -1.0f);
                    }
                } else {
                    row.setValue(choice_col[radiusIndex], -1.0f);
                    row.setValue(n_choice_col[radiusIndex], -1.0f);
                    if (m_weighted_measure_col != -1) {
                        row.setValue(w_choice_col[radiusIndex], -1.0f);
                        row.setValue(nw_choice_col[radiusIndex], -1.0f);
                    }
                }
            }
            ++root;
        }
    }

    map.setDisplayedAttribute(-1);
    map.setDisplayedAttribute(integ_dv_col.back());

    return true;
}
