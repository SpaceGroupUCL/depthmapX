// Copyright (C) 2017 Christian Sailer

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

#include "vgaparser.h"
#include "exceptions.h"
#include <cstring>
#include "radiusconverter.h"
#include "runmethods.h"
#include "parsingutils.h"

using namespace depthmapX;


VgaParser::VgaParser() : _vgaMode(VgaMode::NONE), _globalMeasures(false), _localMeasures(false)
{}

void VgaParser::parse(int argc, char *argv[])
{
    for ( int i = 1; i < argc;  )
    {

        if ( std::strcmp ("-vm", argv[i]) == 0)
        {
            if (_vgaMode != VgaMode::NONE)
            {
                throw CommandLineException("-vm can only be used once, modes are mutually exclusive");
            }
            ENFORCE_ARGUMENT("-vm", i)
            if ( std::strcmp(argv[i], "isovist") == 0 )
            {
                _vgaMode = VgaMode::ISOVIST;
            }
            else if ( std::strcmp(argv[i], "visibility") == 0 )
            {
                _vgaMode = VgaMode::VISBILITY;
            }
            else if ( std::strcmp(argv[i], "metric") == 0 )
            {
                _vgaMode = VgaMode::METRIC;
            }
            else if ( std::strcmp(argv[i], "angular") == 0 )
            {
                _vgaMode = VgaMode::ANGULAR;
            }
            else if ( std::strcmp(argv[i], "thruvision") == 0)
            {
                _vgaMode = VgaMode::THRU_VISION;
            }
            else
            {
                throw CommandLineException(std::string("Invalid VGA mode: ") + argv[i]);
            }
        }
        else if ( std::strcmp(argv[i], "-vg") == 0 )
        {
            _globalMeasures = true;
        }
        else if ( std::strcmp(argv[i], "-vl") == 0 )
        {
            _localMeasures = true;
        }
        else if (std::strcmp(argv[i], "-vr") == 0)
        {
            ENFORCE_ARGUMENT("-vr", i)
            _radius = argv[i];
        }
        ++i;
    }

    if(_vgaMode == VgaMode::NONE)
    {
        _vgaMode = VgaMode::ISOVIST;
    }

    if (_vgaMode == VgaMode::VISBILITY && _globalMeasures)
    {
        if (_radius.empty())
        {
            throw CommandLineException("Global measures in VGA/visibility analysis require a radius, use -vr <radius>");
        }
        if (_radius != "n" && !has_only_digits(_radius))
        {
            throw CommandLineException(std::string("Radius must be a positive integer number or n, got ") + _radius);
        }

    }
    else if (_vgaMode == VgaMode::METRIC)
    {
        if (_radius.empty())
        {
            throw CommandLineException("Metric vga requires a radius, use -vr <radius>");
        }
    }
}

void VgaParser::run(const CommandLineParser &clp, IPerformanceSink &perfWriter) const
{
    RadiusConverter radiusConverter;
    dm_runmethods::runVga(clp, *this, radiusConverter, perfWriter);
}
