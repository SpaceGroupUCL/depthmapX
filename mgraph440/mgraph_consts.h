#pragma once

namespace mgraph440 {

const int METAGRAPH_VERSION = 440;

// Human readable(ish) metagraph version changes

const int VERSION_ALWAYS_RECORD_BINDISTANCES    = 440;

// 17-Aug-2010 Version stamp for Depthmap 10.08.00
const int VERSION_100800                        = 430;

// 12-Jul-2010 Version stamp for Depthmap 10.07.00
const int VERSION_100700                        = 430;

// 12-Jul-2010 Occlusion distances for agent control test
const int VERSION_OCCDISTANCES                  = 430;

// 07-Feb-2010 Axial lines no longer self-connect
const int VERSION_NO_SELF_CONNECTION            = 420;

// 31-Jan-2009 Maps have sublayers
const int VERSION_MAP_LAYERS                    = 410;
//
// 390 and 400 used only for testing
//
// 31-Jan-2009 this is simply a version stamp for Depthmap 8.14.00 and 8.15.00
// The idea is that I should start writing code that saves in version 380 format
// as a benchmark "save for older version"
const int VERSION_81500                         = 380;
const int VERSION_81400                         = 380;
//
// 20-Apr-2008 The shape map name lookup could have been corrupted.  In addition it's not that useful (never much to sort through, unique layer names not necessary)
const int VERSION_NO_SHAPEMAP_NAME_LOOKUP       = 380;
// 15-Mar-2008
const int VERSION_SHAPE_AREA_PERIMETER          = 370;
const int VERSION_FORGET_COLUMN_CREATOR         = 370;
// 04-Oct-2007
const int VERSION_MAP_TYPES                     = 360;
// 20-Sep-2007
const int VERSION_STORE_COLUMN_CREATOR          = 350;
const int VERSION_ATTRIBUTE_LOCKING             = 350;
// version 340 unused
// 08-Jun-2007: Store all drawing layers as shape maps rather than spacepixels (continued)
const int VERSION_DRAWING_SHAPES_B              = 330;
// 27-Nov-2006: Store all drawing layers as shape maps rather than spacepixels
const int VERSION_DRAWING_SHAPES                = 320;
// 27-Nov-2006: Store axial maps as children of shape maps rather than spacepixels
const int VERSION_AXIAL_SHAPES                  = 310;
// 01-Sep-2006: Store formulae for columns
const int VERSION_STORE_FORMULA                 = 300;
// 14-May-2006: Occlusions with node
const int VERSION_OCCLUSIONS                    = 290;
// 28-Dec-2005: Polygon shapes have centroids
const int VERSION_SHAPE_CENTROIDS               = 280;
// 18-Dec-2005: Mapinfo data read into shape maps instead of axial maps
const int VERSION_MAPINFO_SHAPES                = 270;
// 14-Sep-2005: QtRegion bug with segment maps from imported axial maps fixed:
const int VERSION_AXIAL_REGION_FIX              = 263;
// 01-Sep-2005: Now Pointmaps really ought to store their names:
const int VERSION_POINT_MAP_NAMES               = 262;
// 25-Aug-2005: And an extension to shape maps to make them easier to use as lines or points
const int VERSION_EXTENDED_SHAPE_MAPS           = 261;
// 23-Aug-2005: Although in test from 11-Aug-2005, useful to read the testing graphs created:
const int VERSION_SHAPE_MAPS                    = 260;
// 11-Aug-2005: Also, a set of PointMaps now replace a single instance of PointData
const int VERSION_POINT_MAPS                    = 251;
// 11-Aug-2005: Location stored with points, not depixelated on the fly
const int VERSION_POINT_LOCATIONS               = 250;
// 01-Mar-2005: Quick grid connections
const int VERSION_GRID_CONNECTIONS              = 240;
// 02-Dec-2004: Axial map gates
const int VERSION_GATE_MAPS                     = 230;
// 29-Oct-2004: Store the colour display settings with the graph data
const int VERSION_STORE_GRIDTEXTINFO            = 220;
// 29-Oct-2004: Store the colour display settings with the graph data
const int VERSION_STORE_COLOR                   = 210;
// 16-Jun-2004: New boundary graph (now much simpler: nodes at edge of main graph)
const int VERSION_NEWBOUNDARYGRAPH              = 200;
// 20-May-2004: Each segment must have forward and backward connections listed separately!
const int VERSION_SEGMENT_MAPS_FIX              = 191;
// 17-May-2004: Axial maps can be either segment or axial maps.  Affects ShapeGraph and AxialLine classes
const int VERSION_SEGMENT_MAPS                  = 190;
// 12-May-2004: Extra Mapinfo table data
const int VERSION_MAPINFO_DATA                  = 180;
// 06-May-2004: Explicit links and unlinks for axial lines
const int VERSION_AXIAL_LINKS                   = 170;
// 29-Feb-2004: Attributes table (already used for AxialLines) now used for PointData as well
const int VERSION_ATTRIBUTES_TABLE              = 160;
// File compression introduced
const int VERSION_FILE_COMPRESSION              = 150;
// Some minor modifications to the axial line format... won't load v.130 files
const int VERSION_REVISED_AXIAL                 = 140;
// View class specifies whether axial or vga currently viewed
const int VERSION_VIEW_CLASS_ADDED              = 130;
// A distance stored in the bin
const int VERSION_BINDISTANCES                  = 120;
// A set of nodes on the boundaries of the isovist
const int VERSION_BOUNDARYGRAPH                 = 110;
// Dynamic lines (addable and removable) in the visibility graph
const int VERSION_DYNAMICLINES                  = 100;
// Line layers are coloured...
const int VERSION_LAYERCOLORS                   = 91;
// Blocked locations split into 4, replaces m_noderef
const int VERSION_BLOCKEDQUAD                   = 90;
// Space pixel groups have different space pixels for different layers (at their own resolution!)
const int VERSION_SPACEPIXELGROUPS              = 80;
// The graph state is just recorded
const int VERSION_STATE_RECORDED                = 72;
// The binsizes weren't included in the metagraph 70
const int VERSION_NGRAPH_BINCOUNT               = 71;
// Major, major changes to the graph format (from now on it will now be held in memory only)
const int VERSION_NGRAPH_INTROD                 = 70;
// Slight changes to PointData required for the actual implementation of the quick graph
const int VERSION_SPARK_GRAPH_INTROD            = 61;
// Quick graph... add underlying info about lines into the pointdata structure
const int VERSION_QUICK_GRAPH_INTROD            = 60;
// Layers
const int VERSION_LAYERS_CENTROID_INTROD        = 51;
const int VERSION_LAYERS_INTROD                 = 50;
// version 41 repairs VERSION_EXTRA_POINT_DATA_INTROD bug
const int VERSION_EXTRA_POINT_DATA_INTROD       = 40;
const int VERSION_BINS_INTROD                   = 30;

const unsigned int SALA_SELECTED_COLOR = 0x00FFFF77;
const unsigned int SALA_HIGHLIGHTED_COLOR = 0x0077FFFF;

}
