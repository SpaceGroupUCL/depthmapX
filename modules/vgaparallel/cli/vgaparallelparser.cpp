// Copyright (C) 2017 Christian Sailer
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

#include "vgaparallelparser.h"
#include "depthmapXcli/exceptions.h"
#include "depthmapXcli/parsingutils.h"
#include "depthmapXcli/runmethods.h"
#include "depthmapXcli/simpletimer.h"
#include "modules/vgaparallel/core/vgaangularopenmp.h"
#include "modules/vgaparallel/core/vgametricopenmp.h"
#include "modules/vgaparallel/core/vgavisualglobalopenmp.h"
#include "modules/vgaparallel/core/vgavisuallocaladjmatrix.h"
#include "modules/vgaparallel/core/vgavisuallocalopenmp.h"
#include "salalib/options.h"
#include <cstring>
#include <iostream>

using namespace depthmapX;

VgaParallelParser::VgaParallelParser() : m_vgaMode(VgaMode::NONE) {}

void VgaParallelParser::parse(int argc, char *argv[]) {
    for (int i = 1; i < argc;) {

        if (std::strcmp("-vm", argv[i]) == 0) {
            if (m_vgaMode != VgaMode::NONE) {
                throw CommandLineException("-vm can only be used once, modes are mutually exclusive");
            }
            ENFORCE_ARGUMENT("-vm", i)
            if (std::strcmp(argv[i], "visiblity-global") == 0) {
                m_vgaMode = VgaMode::VISBILITY_GLOBAL;
            } else if (std::strcmp(argv[i], "visibility-local") == 0) {
                m_vgaMode = VgaMode::VISBILITY_LOCAL;
            } else if (std::strcmp(argv[i], "visibility-local-adjmatrix") == 0) {
                m_vgaMode = VgaMode::VISBILITY_LOCAL_ADJMATRIX;
            } else if (std::strcmp(argv[i], "metric") == 0) {
                m_vgaMode = VgaMode::METRIC;
            } else if (std::strcmp(argv[i], "angular") == 0) {
                m_vgaMode = VgaMode::ANGULAR;
            } else {
                throw CommandLineException(std::string("Invalid VGAPARALLEL mode: ") + argv[i]);
            }
        } else if (std::strcmp(argv[i], "-vr") == 0) {
            ENFORCE_ARGUMENT("-vr", i)
            m_radius = argv[i];
        }
        ++i;
    }

    if (m_vgaMode == VgaMode::VISBILITY_GLOBAL) {
        if (m_radius.empty()) {
            throw CommandLineException(
                "Global measures in VGA/visibility analysis require a radius, use -vr <radius>");
        }
        if (m_radius != "n" && !has_only_digits(m_radius)) {
            throw CommandLineException(std::string("Radius must be a positive integer number or n, got ") + m_radius);
        }

    } else if (m_vgaMode == VgaMode::METRIC) {
        if (m_radius.empty()) {
            throw CommandLineException("Metric vga requires a radius, use -vr <radius>");
        }
        if (m_radius != "n" && !has_only_digits(m_radius)) {
            throw CommandLineException(std::string("Radius must be a positive integer number or n, got ") + m_radius);
        }
    }
}

void VgaParallelParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const {

    auto mgraph = dm_runmethods::loadGraph(clp.getFileName().c_str(), perfWriter);

    std::unique_ptr<IAnalysis> analysis = nullptr;

    RadiusConverter radiusConverter;
    switch (getVgaMode()) {
    case VgaMode::VISBILITY_GLOBAL:
        analysis = std::unique_ptr<IAnalysis>(new VGAVisualGlobalOpenMP(
            mgraph->getDisplayedPointMap(), radiusConverter.ConvertForVisibility(getRadius()), false));
        break;
    case VgaMode::VISBILITY_LOCAL:
        analysis = std::unique_ptr<IAnalysis>(new VGAVisualLocalOpenMP(mgraph->getDisplayedPointMap()));
        break;
    case VgaMode::VISBILITY_LOCAL_ADJMATRIX:
        analysis = std::unique_ptr<IAnalysis>(new VGAVisualLocalAdjMatrix(mgraph->getDisplayedPointMap(), false));
        break;
    case VgaMode::METRIC:
        analysis = std::unique_ptr<IAnalysis>(
            new VGAMetricOpenMP(mgraph->getDisplayedPointMap(), radiusConverter.ConvertForMetric(getRadius()), false));
        break;
    case VgaMode::ANGULAR:
        analysis = std::unique_ptr<IAnalysis>(new VGAAngularOpenMP(
            mgraph->getDisplayedPointMap(), radiusConverter.ConvertForMetric(getRadius()), false));
        break;
    default:
        throw depthmapX::SetupCheckException("Unsupported VGA mode");
    }

    std::cout << " ok\nAnalysing graph..." << std::flush;

    DO_TIMED("Run VGA", analysis->run(dm_runmethods::getCommunicator(clp).get()))
    std::cout << " ok\nWriting out result..." << std::flush;
    DO_TIMED("Writing graph", mgraph->write(clp.getOuputFile().c_str(), METAGRAPH_VERSION, false))
    std::cout << " ok" << std::endl;
}
