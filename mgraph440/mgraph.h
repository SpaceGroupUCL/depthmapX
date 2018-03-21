#pragma once

#include "mgraph440/ngraph.h"  // for conversion
#include "mgraph440/pointmap.h"
#include "mgraph440/axialmap.h"
#include "mgraph440/shapemap.h"
#include "mgraph440/spacepix.h"
#include "mgraph440/datalayer.h"
#include "mgraph440/fileproperties.h"
#include "mgraph440/bspnode.h"
#include "mgraph440/attr.h"

namespace mgraph440 {

class MetaGraph : public SuperSpacePixel, public PointMaps, public FileProperties
{
public:
    MetaGraph();
    ~MetaGraph();

    enum { ANGULARGRAPH = 0x0010, LINEDATA = 0x0004, POINTMAPS = 0x0002, DATAMAPS = 0x0020, SHAPEGRAPHS = 0x0100, BUGGY = 0x8000 };
    enum { OK, WARN_BUGGY_VERSION, WARN_CONVERTED, NOT_A_GRAPH, DAMAGED_FILE, DISK_ERROR, NEWER_VERSION, DEPRECATED_VERSION };
    enum { VIEWNONE = 0x00, VIEWVGA = 0x01, VIEWAXIAL = 0x04, VIEWDATA = 0x20 };

    int m_state;
    int m_file_version;
    int m_view_class;
    bool m_showgrid;
    bool m_showtext;

    ShapeMaps<ShapeMap> m_data_maps;
    ShapeGraphs m_shape_graphs;

    // Standard layers no longer added: the gates layer will be initialised with the first push to layer,
    // or when made from axial lines.

    pqvector<AttrBody> *m_attr_conv_table = NULL;

    streampos skipVirtualMem(ifstream& stream, int version);
    int convertDataLayersToShapeMap(DataLayers& datalayers, PointMap& pointmap);
    int convertAttributes(ifstream &stream, int version);
    int convertVirtualMem(ifstream &stream, int version);
    int read(const std::string &filename);
    int writeToFile( const std::string& filename, int version, bool currentlayer = false);
    int writeToStream(ostream &stream, int version, bool currentlayer = false);
};

}
