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

#include "genlib/pflipper.h"
#include "genlib/stringutils.h"

bool AxialIntegration::run(Communicator *comm, ShapeGraph &map, bool simple_version) {
    // note, from 10.0, Depthmap no longer includes *self* connections on axial lines
    // self connections are stripped out on loading graph files, as well as no longer made

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getShapeCount());
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

    // for choice
    AnalysisInfo **audittrail;
    if (m_choice) {
        audittrail = new AnalysisInfo *[map.getShapeCount()];
        for (size_t i = 0; i < map.getShapeCount(); i++) {
            audittrail[i] = new AnalysisInfo[radii.size()];
        }
    }

    // n.b., for this operation we assume continuous line referencing from zero (this is silly?)
    // has already failed due to this!  when intro hand drawn fewest line (where user may have deleted)
    // it's going to get worse...

    bool *covered = new bool[map.getShapeCount()];

    size_t i = -1;
    for (auto & iter : attributes) {
        i++;
        AttributeRow &row = iter.getRow();
        for (size_t j = 0; j < map.getShapeCount(); j++) {
            covered[j] = false;
        }
        if (m_choice) {
            for (size_t k = 0; k < map.getShapeCount(); k++) {
                audittrail[k][0].previous.ref = -1; // note, 0th member used as radius doesn't matter
                // note, choice columns are not cleared, but cummulative over all shortest path pairs
            }
        }

        if (m_local) {
            double control = 0.0;
            const std::vector<int> &connections = map.getConnections()[i].m_connections;
            std::vector<int> totalneighbourhood;
            for (int connection : connections) {
                // n.b., as of Depthmap 10.0, connections[j] and i cannot coexist
                // if (connections[j] != i) {
                depthmapX::addIfNotExists(totalneighbourhood, connection);
                int retro_size = 0;
                auto &retconnectors = map.getConnections()[size_t(connection)].m_connections;
                for (auto retconnector : retconnectors) {
                    retro_size++;
                    depthmapX::addIfNotExists(totalneighbourhood, retconnector);
                }
                control += 1.0 / double(retro_size);
                //}
            }

            if (!simple_version) {
                if (connections.size() > 0) {
                    row.setValue(control_col, float(control));
                    row.setValue(controllability_col,
                                 float(double(connections.size()) / double(totalneighbourhood.size() - 1)));
                } else {
                    row.setValue(control_col, -1);
                    row.setValue(controllability_col, -1);
                }
            }
        }

        std::vector<int> depthcounts;
        depthcounts.push_back(0);

        pflipper<std::vector<std::pair<int, int>>> foundlist;
        foundlist.a().push_back(std::pair<int, int>(i, -1));
        covered[i] = true;
        int total_depth = 0, depth = 1, node_count = 1, pos = -1, previous = -1; // node_count includes this 1
        double weight = 0.0, rootweight = 0.0, total_weight = 0.0, w_total_depth = 0.0;
        if (m_weighted_measure_col != -1) {
            rootweight = weights[i];
            // include this line in total weights (as per nodecount)
            total_weight += rootweight;
        }
        int index = -1;
        int r = 0;
        for (int radius : radii) {
            while (foundlist.a().size()) {
                if (!m_choice) {
                    index = foundlist.a().back().first;
                } else {
                    pos = pafrand() % foundlist.a().size();
                    index = foundlist.a().at(pos).first;
                    previous = foundlist.a().at(pos).second;
                    audittrail[index][0].previous.ref =
                        previous; // note 0th member used here: can be used individually different radius previous
                }
                Connector &line = map.getConnections()[index];
                for (size_t k = 0; k < line.m_connections.size(); k++) {
                    if (!covered[line.m_connections[k]]) {
                        covered[line.m_connections[k]] = true;
                        foundlist.b().push_back(std::pair<int, int>(line.m_connections[k], index));
                        if (m_weighted_measure_col != -1) {
                            // the weight is taken from the discovered node:
                            weight = weights[line.m_connections[k]];
                            total_weight += weight;
                            w_total_depth += depth * weight;
                        }
                        if (m_choice && previous != -1) {
                            // both directional paths are now recorded for choice
                            // (coincidentally fixes choice problem which was completely wrong)
                            size_t here = index;   // note: start counting from index as actually looking ahead here
                            while (here != i) { // not i means not the current root for the path
                                audittrail[here][r].choice += 1;
                                audittrail[here][r].weighted_choice += weight * rootweight;
                                here =
                                    audittrail[here][0].previous.ref; // <- note, just using 0th position: radius for
                                                                      // the previous doesn't matter in this analysis
                            }
                            if (m_weighted_measure_col != -1) {
                                // in weighted choice, root node and current node receive values:
                                audittrail[i][r].weighted_choice += (weight * rootweight) * 0.5;
                                audittrail[line.m_connections[k]][r].weighted_choice += (weight * rootweight) * 0.5;
                            }
                        }
                        total_depth += depth;
                        node_count++;
                        depthcounts.back() += 1;
                    }
                }
                if (!m_choice)
                    foundlist.a().pop_back();
                else
                    foundlist.a().erase(foundlist.a().begin() + pos);
                if (!foundlist.a().size()) {
                    foundlist.flip();
                    depth++;
                    depthcounts.push_back(0);
                    if (radius != -1 && depth > radius) {
                        break;
                    }
                }
            }
            // set the attributes for this node:
            row.setValue(count_col[r], float(node_count));
            if (m_weighted_measure_col != -1) {
                row.setValue(total_weight_col[r], float(total_weight));
            }
            // node count > 1 to avoid divide by zero (was > 2)
            if (node_count > 1) {
                // note -- node_count includes this one -- mean depth as per p.108 Social Logic of Space
                double mean_depth = double(total_depth) / double(node_count - 1);
                row.setValue(depth_col[r], float(mean_depth));
                if (m_weighted_measure_col != -1) {
                    // weighted mean depth:
                    row.setValue(w_depth_col[r], float(w_total_depth / total_weight));
                }
                // total nodes > 2 to avoid divide by 0 (was > 3)
                if (node_count > 2 && mean_depth > 1.0) {
                    double ra = 2.0 * (mean_depth - 1.0) / double(node_count - 2);
                    // d-value / p-value from Depthmap 4 manual, note: node_count includes this one
                    double rra_d = ra / dvalue(node_count);
                    double rra_p = ra / dvalue(node_count);
                    double integ_tk = teklinteg(node_count, total_depth);
                    row.setValue(integ_dv_col[r], float(1.0 / rra_d));

                    if (!simple_version) {
                        row.setValue(integ_pv_col[r], float(1.0 / rra_p));
                        if (total_depth - node_count + 1 > 1) {
                            row.setValue(integ_tk_col[r], float(integ_tk));
                        } else {
                            row.setValue(integ_tk_col[r], -1.0f);
                        }
                    }

                    if (m_fulloutput) {
                        row.setValue(ra_col[r], float(ra));

                        if (!simple_version) {
                            row.setValue(rra_col[r], float(rra_d));
                        }
                        row.setValue(td_col[r], float(total_depth));

                        if (!simple_version) {
                            // alan's palm-tree normalisation: palmtree
                            double dmin = node_count - 1;
                            double dmax = palmtree(node_count, depth - 1);
                            if (dmax != dmin) {
                                row.setValue(penn_norm_col[r], float((dmax - total_depth) / (dmax - dmin)));
                            }
                        }
                    }
                } else {
                    row.setValue(integ_dv_col[r], -1.0f);

                    if (!simple_version) {
                        row.setValue(integ_pv_col[r], -1.0f);
                        row.setValue(integ_tk_col[r], -1.0f);
                    }
                    if (m_fulloutput) {
                        row.setValue(ra_col[r], -1.0f);

                        if (!simple_version) {
                            row.setValue(rra_col[r], -1.0f);
                        }

                        row.setValue(td_col[r], -1.0f);

                        if (!simple_version) {
                            row.setValue(penn_norm_col[r], -1.0f);
                        }
                    }
                }

                if (!simple_version) {
                    double entropy = 0.0, intensity = 0.0, rel_entropy = 0.0, factorial = 1.0, harmonic = 0.0;
                    for (size_t k = 0; k < depthcounts.size(); k++) {
                        if (depthcounts[k] != 0) {
                            // some debate over whether or not this should be node count - 1
                            // (i.e., including or not including the node itself)
                            double prob = double(depthcounts[k]) / double(node_count);
                            entropy -= prob * log2(prob);
                            // Formula from Turner 2001, "Depthmap"
                            factorial *= double(k + 1);
                            double q = (pow(mean_depth, double(k)) / double(factorial)) * exp(-mean_depth);
                            rel_entropy += (double)prob * log2(prob / q);
                            //
                            harmonic += 1.0 / double(depthcounts[k]);
                        }
                    }
                    harmonic = double(depthcounts.size()) / harmonic;
                    if (total_depth > node_count) {
                        intensity = node_count * entropy / (total_depth - node_count);
                    } else {
                        intensity = -1;
                    }
                    row.setValue(entropy_col[r], float(entropy));
                    row.setValue(rel_entropy_col[r], float(rel_entropy));
                    row.setValue(intensity_col[r], float(intensity));
                    row.setValue(harmonic_col[r], float(harmonic));
                }
            } else {
                row.setValue(depth_col[r], -1.0f);
                row.setValue(integ_dv_col[r], -1.0f);

                if (!simple_version) {
                    row.setValue(integ_pv_col[r], -1.0f);
                    row.setValue(integ_tk_col[r], -1.0f);
                    row.setValue(entropy_col[r], -1.0f);
                    row.setValue(rel_entropy_col[r], -1.0f);
                    row.setValue(harmonic_col[r], -1.0f);
                }
            }
            ++r;
        }
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    delete[] covered;
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, i);
            }
        }
    }
    delete[] covered;
    if (m_choice) {
        i = -1;
        for (auto & iter: attributes) {
            i++;
            AttributeRow &row = iter.getRow();
            double total_choice = 0.0, w_total_choice = 0.0;
            for (size_t r = 0; r < radii.size(); r++) {
                total_choice += audittrail[i][r].choice;
                w_total_choice += audittrail[i][r].weighted_choice;
                // n.b., normalise choice according to (n-1)(n-2)/2 (maximum possible through routes)
                double node_count = row.getValue(count_col[r]);
                double total_weight = 0;
                if (m_weighted_measure_col != -1) {
                    total_weight = row.getValue(total_weight_col[r]);
                }
                if (node_count > 2) {
                    row.setValue(choice_col[r], float(total_choice));
                    row.setValue(n_choice_col[r], float(2.0 * total_choice / ((node_count - 1) * (node_count - 2))));
                    if (m_weighted_measure_col != -1) {
                        row.setValue(w_choice_col[r], float(w_total_choice));
                        row.setValue(nw_choice_col[r], float(2.0 * w_total_choice / (total_weight * total_weight)));
                    }
                } else {
                    row.setValue(choice_col[r], -1);
                    row.setValue(n_choice_col[r], -1);
                    if (m_weighted_measure_col != -1) {
                        row.setValue(w_choice_col[r], -1);
                        row.setValue(nw_choice_col[r], -1);
                    }
                }
            }
        }
        for (size_t i = 0; i < map.getShapeCount(); i++) {
            delete[] audittrail[i];
        }
        delete[] audittrail;
    }

    map.setDisplayedAttribute(-1); // <- override if it's already showing
    map.setDisplayedAttribute(integ_dv_col.back());

    return true;
}
