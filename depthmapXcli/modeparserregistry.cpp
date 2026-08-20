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

#include "modeparserregistry.h"
#include "importparser.h"
#include "linkparser.h"
#include "vgaparser.h"
#include "visprepparser.h"
#include "axialparser.h"
#include "segmentparser.h"
#include "agentparser.h"
#include "isovistparser.h"
#include "exportparser.h"
#include "stepdepthparser.h"
#include "mapconvertparser.h"
#include "modules/segmentshortestpaths/cli/segmentshortestpathparser.h"
#include "modules/vgapaths/cli/shortestpathparser.h"
#include "modules/vgapaths/cli/isovistzoneparser.h"


void ModeParserRegistry::populateParsers()
{
    // Register any mode parsers here
    REGISTER_PARSER(VgaParser);
    REGISTER_PARSER(LinkParser);
    REGISTER_PARSER(VisPrepParser);
    REGISTER_PARSER(AxialParser);
    REGISTER_PARSER(SegmentParser);
    REGISTER_PARSER(AgentParser);
    REGISTER_PARSER(IsovistParser);
    REGISTER_PARSER(ExportParser);
    REGISTER_PARSER(ImportParser);
    REGISTER_PARSER(StepDepthParser);
    REGISTER_PARSER(MapConvertParser);
    REGISTER_PARSER(SegmentShortestPathParser);
    REGISTER_PARSER(ShortestPathParser);
    REGISTER_PARSER(IsovistZoneParser);
    // *********
}
