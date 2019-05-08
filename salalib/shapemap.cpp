// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Tasos Varoudis

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

// This is my code to make a set of axial lines from a set of boundary lines

#include "salalib/shapemap.h"
#include "salalib/attributetable.h"
#include "salalib/attributetablehelpers.h"
#include "salalib/mgraph.h"              // purely for the version info --- as phased out should replace
#include "salalib/parsers/mapinfodata.h" // for mapinfo interface

#include "genlib/comm.h" // for communicator
#include "genlib/containerutils.h"
#include "genlib/exceptions.h"
#include "genlib/stringutils.h"

#include <float.h>
#include <math.h>
#include <numeric>
#include <stdexcept>
#include <time.h>
#include <unordered_map>
#include <unordered_set>

#ifndef _WIN32
#define _finite finite
#endif

static const double TOLERANCE_A = 1e-9;

// import TOLERANCE_B from axial map...
static const double TOLERANCE_B = 1e-12;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool SalaShape::read(std::istream &stream) {
    // defaults
    m_draworder = -1;
    m_selected = false;

    stream.read((char *)&m_type, sizeof(m_type));

    stream.read((char *)&m_region, sizeof(m_region));

    stream.read((char *)&m_centroid, sizeof(m_centroid));

    stream.read((char *)&m_area, sizeof(m_area));
    stream.read((char *)&m_perimeter, sizeof(m_perimeter));

    dXreadwrite::readIntoVector(stream, m_points);

    return true;
}

bool SalaShape::write(std::ofstream &stream) {
    stream.write((char *)&m_type, sizeof(m_type));
    stream.write((char *)&m_region, sizeof(m_region));
    stream.write((char *)&m_centroid, sizeof(m_centroid));
    stream.write((char *)&m_area, sizeof(m_area));
    stream.write((char *)&m_perimeter, sizeof(m_perimeter));
    dXreadwrite::writeVector(stream, m_points);
    return true;
}

void SalaShape::setCentroidAreaPerim() {
    m_area = 0.0;
    m_perimeter = 0.0;
    m_centroid = Point2f(0, 0);
    for (size_t i = 0; i < m_points.size(); i++) {
        Point2f &p1 = m_points[i];
        Point2f &p2 = m_points[(i + 1) % m_points.size()];
        double a_i = (p1.x * p2.y - p2.x * p1.y) / 2.0;
        m_area += a_i;
        a_i /= 6.0;
        m_centroid.x += (p1.x + p2.x) * a_i;
        m_centroid.y += (p1.y + p2.y) * a_i;
        Point2f side = p2 - p1;
        m_perimeter += side.length();
    }
    m_type &= ~SHAPE_CCW;
    if (sgn(m_area) == 1) {
        m_type |= SHAPE_CCW;
    }
    m_centroid.scale(2.0 / m_area); // note, *not* fabs(m_area) as it is then confused by clockwise ordered shapes
    m_area = fabs(m_area);
    if (isOpen()) {
        // take off the automatically collected final side
        Point2f side = m_points.back() - m_points.front();
        m_perimeter -= side.length();
    }
}

// allows override of the above (used for isovists)
void SalaShape::setCentroid(const Point2f &p) { m_centroid = p; }

// get the angular deviation along the length of a poly line:
double SalaShape::getAngDev() const {
    double dev = 0.0;
    for (size_t i = 1; i < m_points.size() - 1; i++) {
        double ang = angle(m_points[i - 1], m_points[i], m_points[i + 1]);

        // Quick mod - TV
#if defined(_MSC_VER)
        dev += abs(M_PI - ang);
#else
        (M_PI - ang) < 0.0 ? dev += (ang - M_PI) : dev += (M_PI - ang);
#endif
    }
    // convert to Iida Hillier units (0 to 2):
    dev /= M_PI * 0.5;
    return dev;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// the replacement for datalayers

ShapeMap::ShapeMap(const std::string &name, int type)
    : m_pixel_shapes(0, 0), m_attributes(new AttributeTable()),
      m_attribHandle(new AttributeTableHandle(*m_attributes)) {
    m_name = name;
    m_map_type = type;
    m_hasgraph = false;

    // shape and object counters
    m_obj_ref = -1;
    // -1 is the shape ref column (which will be shown by default)
    m_displayed_attribute = -1;
    m_invalidate = false;
    // for polygons:
    m_show_lines = true;
    m_show_fill = true;
    m_show_centroids = false;

    // data (MUST be set before use)
    m_tolerance = 0.0;

    m_selection = false;

    // note show is
    m_show = true;
    m_editable = false;

    m_bsp_tree = false;
    m_bsp_root = NULL;
    //
    m_hasMapInfoData = false;
}

ShapeMap::~ShapeMap() {
    if (m_bsp_root) {
        delete m_bsp_root;
        m_bsp_root = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////

// this can be reinit as well

void ShapeMap::init(int size, const QtRegion &r) {
    m_display_shapes.clear();
    m_rows = __min(__max(20, (int)sqrt((double)size)), 32768);
    m_cols = __min(__max(20, (int)sqrt((double)size)), 32768);
    if (m_region.atZero()) {
        m_region = r;
    } else {
        m_region = runion(m_region, r);
    }
    // calculate geom data:
    m_tolerance = __max(m_region.width(), m_region.height()) * TOLERANCE_A;
    //
    m_pixel_shapes = depthmapX::ColumnMatrix<std::vector<ShapeRef>>(m_rows, m_cols);
}

// this makes an exact copy, keep the reference numbers and so on:

void ShapeMap::copy(const ShapeMap &sourcemap, int copyflags) {
    if ((copyflags & ShapeMap::COPY_GEOMETRY) == ShapeMap::COPY_GEOMETRY) {
        m_shapes.clear();
        init(sourcemap.m_shapes.size(), sourcemap.m_region);
        for (auto shape : sourcemap.m_shapes) {
            // using makeShape is actually easier than thinking about a total copy:
            makeShape(shape.second, shape.first);
            // note that addShape automatically adds the attribute row
        }
    }

    if ((copyflags & ShapeMap::COPY_ATTRIBUTES) == ShapeMap::COPY_ATTRIBUTES) {
        // assumes attribute rows are filled in already

        // TODO: Compatibility. The columns are sorted in the old implementation so
        // they are also passed sorted in the conversion:

        std::vector<size_t> indices(sourcemap.m_attributes->getNumColumns());
        std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            return sourcemap.m_attributes->getColumnName(a) < sourcemap.m_attributes->getColumnName(b);
        });

        for (int idx : indices) {
            int outcol = m_attributes->insertOrResetColumn(sourcemap.m_attributes->getColumnName(idx));
            // n.b. outcol not necessarily the same as incol, although row position in table (j) should match

            auto targetIter = m_attributes->begin();
            for (auto sourceIter = sourcemap.m_attributes->begin(); sourceIter != sourcemap.m_attributes->end();
                 sourceIter++) {
                targetIter->getRow().setValue(outcol, sourceIter->getRow().getValue(idx));
                targetIter++;
            }
        }
    }

    if ((copyflags & ShapeMap::COPY_ATTRIBUTES) == ShapeMap::COPY_GRAPH) {
        if (sourcemap.m_hasgraph) {
            m_hasgraph = true;
            // straight copy:
            m_connectors = sourcemap.m_connectors;
            m_links = sourcemap.m_links;
            m_unlinks = sourcemap.m_unlinks;
        }
    }

    // copies mapinfodata (projection data) regardless of copy flags
    if (sourcemap.hasMapInfoData()) {
        m_mapinfodata = MapInfoData();
        m_mapinfodata.m_coordsys = sourcemap.getMapInfoData().m_coordsys;
        m_mapinfodata.m_bounds = sourcemap.getMapInfoData().m_bounds;
        m_hasMapInfoData = true;
    }
}

// Zaps all memory structures, apart from mapinfodata
void ShapeMap::clearAll() {
    if (m_bsp_root) {
        delete m_bsp_root;
        m_bsp_root = NULL;
    }
    m_display_shapes.clear();

    m_shapes.clear();
    m_undobuffer.clear();
    m_connectors.clear();
    m_attributes->clear();
    m_links.clear();
    m_unlinks.clear();
    m_region = QtRegion();

    m_obj_ref = -1;
    m_displayed_attribute = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////

int ShapeMap::makePointShapeWithRef(const Point2f &point, int shape_ref, bool tempshape,
                                    const std::map<int, float> &extraAttributes) {
    bool bounds_good = true;

    if (!m_region.contains_touch(point)) {
        bounds_good = false;
        init(m_shapes.size(), QtRegion(point, point));
    }

    m_shapes.insert(std::make_pair(shape_ref, SalaShape(point)));

    if (bounds_good) {
        // note: also sets polygon bounding box:
        makePolyPixels(shape_ref);
    } else {
        // pixelate all polys in the pixel new structure:
        for (auto shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    if (!tempshape) {
        auto &row = m_attributes->addRow(AttributeKey(shape_ref));
        for (auto &attr : extraAttributes) {
            row.setValue(attr.first, attr.second);
        }
        m_newshape = true;
    }

    return shape_ref;
}

int ShapeMap::makePointShape(const Point2f &point, bool tempshape, const std::map<int, float> &extraAttributes) {
    return makePointShapeWithRef(point, getNextShapeKey(), tempshape, extraAttributes);
}

int ShapeMap::makeLineShapeWithRef(const Line &line, int shape_ref, bool through_ui, bool tempshape,
                                   const std::map<int, float> &extraAttributes) {
    // note, map must have editable flag on if we are to make a shape through the user interface:
    if (through_ui && !m_editable) {
        return -1;
    }

    bool bounds_good = true;

    if (!(m_region.contains_touch(line.start()) && m_region.contains_touch(line.end()))) {
        bounds_good = false;
        init(m_shapes.size(), line);
    }

    // note, shape constructor sets centroid, length etc
    m_shapes.insert(std::make_pair(shape_ref, SalaShape(line)));

    if (bounds_good) {
        // note: also sets polygon bounding box:
        makePolyPixels(shape_ref);
    } else {
        // pixelate all polys in the pixel new structure:
        for (auto shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    if (!tempshape) {
        auto &row = m_attributes->addRow(AttributeKey(shape_ref));
        for (auto &attr : extraAttributes) {
            row.setValue(attr.first, attr.second);
        }
        m_newshape = true;
    }

    if (through_ui) {
        // manually add connections:
        if (m_hasgraph) {
            int rowid = depthmapX::findIndexFromKey(m_shapes, shape_ref);
            if (isAxialMap()) {
                connectIntersected(rowid, true); // "true" means line-line intersections only will be applied
            } else {
                connectIntersected(rowid, false);
            }
        }
        // if through ui, set undo counter:
        m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_CREATED, shape_ref));
        // update displayed attribute if through ui:
        invalidateDisplayedAttribute();
        setDisplayedAttribute(m_displayed_attribute);
    }

    return shape_ref;
}

int ShapeMap::getNextShapeKey() {
    if (m_shapes.size() == 0)
        return 0;
    return m_shapes.rbegin()->first + 1;
}

int ShapeMap::makeLineShape(const Line &line, bool through_ui, bool tempshape,
                            const std::map<int, float> &extraAttributes) {
    return makeLineShapeWithRef(line, getNextShapeKey(), through_ui, tempshape, extraAttributes);
}

int ShapeMap::makePolyShapeWithRef(const std::vector<Point2f> &points, bool open, int shape_ref, bool tempshape,
                                   const std::map<int, float> &extraAttributes) {
    bool bounds_good = true;

    switch (points.size()) {
    case 0:
        return -1;
    case 1:
        return makePointShapeWithRef(points[0], shape_ref, tempshape);
    case 2:
        return makeLineShapeWithRef(Line(points[0], points[1]), shape_ref, false,
                                    tempshape); // false is not through ui: there really should be a through ui here?
    }

    QtRegion region(points[0], points[0]);
    size_t i;
    for (i = 1; i < points.size(); i++) {
        region.encompass(points[i]);
    }
    if (!m_region.contains_touch(region.bottom_left) || !m_region.contains_touch(region.top_right)) {
        bounds_good = false;
        init(m_shapes.size(), region);
    }

    size_t len = points.size();
    // NOTE: This is commented out deliberately
    // Sometimes you really do want a polyline that forms a loop
    /*
    if (points.head() == points.tail()) {
       len--;
       open = false;
    }
    */

    // not sure if it matters if the polygon is clockwise or anticlockwise... we'll soon tell!

    if (open) {
        m_shapes.insert(std::make_pair(shape_ref, SalaShape(SalaShape::SHAPE_POLY)));
    } else {
        m_shapes.insert(std::make_pair(shape_ref, SalaShape(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED)));
    }
    for (i = 0; i < len; i++) {
        m_shapes.rbegin()->second.m_points.push_back(points[i]);
    }

    if (bounds_good) {
        // note: also sets polygon bounding box:
        makePolyPixels(shape_ref);
    } else {
        // pixelate all polys in the pixel new structure:
        for (auto shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    if (!tempshape) {
        // set centroid now also adds a few other things: as well as area, perimeter
        m_shapes.rbegin()->second.setCentroidAreaPerim();

        auto &row = m_attributes->addRow(AttributeKey(shape_ref));
        for (auto &attr : extraAttributes) {
            row.setValue(attr.first, attr.second);
        }
        m_newshape = true;
    }

    return shape_ref;
}

int ShapeMap::makePolyShape(const std::vector<Point2f> &points, bool open, bool tempshape,
                            const std::map<int, float> &extraAttributes) {
    return makePolyShapeWithRef(points, open, getNextShapeKey(), tempshape, extraAttributes);
}

int ShapeMap::makeShape(const SalaShape &poly, int override_shape_ref, const std::map<int, float> &extraAttributes) {
    // overridden shape cannot exist:
    if (override_shape_ref != -1 && m_shapes.find(override_shape_ref) != m_shapes.end()) {
        return -1; // failure!
    }

    bool bounds_good = true;

    if (!m_region.contains_touch(poly.m_region.bottom_left) || !m_region.contains_touch(poly.m_region.top_right)) {
        bounds_good = false;
        init(m_shapes.size(), poly.m_region);
    }

    int shape_ref;
    if (override_shape_ref == -1) {
        shape_ref = getNextShapeKey();
    } else {
        shape_ref = override_shape_ref;
    }

    m_shapes.insert(std::make_pair(shape_ref, poly));

    if (bounds_good) {
        // note: also sets polygon bounding box:
        makePolyPixels(shape_ref);
    } else {
        // pixelate all polys in the pixel new structure:
        for (auto shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    auto &row = m_attributes->addRow(AttributeKey(shape_ref));
    for (auto &attr : extraAttributes) {
        row.setValue(attr.first, attr.second);
    }

    m_newshape = true;

    return shape_ref;
}

//////////////////////////////////////////////////////////////////////////////////////////

// n.b., only works from current selection (and uses point selected attribute)

int ShapeMap::makeShapeFromPointSet(const PointMap &pointmap) {
    bool bounds_good = true;
    PixelRefVector selset;
    Point2f offset = Point2f(pointmap.getSpacing() / 2, pointmap.getSpacing() / 2);
    for (auto &sel : pointmap.getSelSet()) {
        selset.push_back(sel);
        if (!m_region.contains_touch(pointmap.depixelate(sel) - offset) ||
            !m_region.contains_touch(pointmap.depixelate(sel) + offset)) {
            bounds_good = false;
        }
    }
    if (!bounds_good) {
        QtRegion r(pointmap.getRegion().bottom_left - offset, pointmap.getRegion().top_right + offset);
        init(m_shapes.size(), r);
    }
    std::map<int, int> relations;
    for (size_t j = 0; j < selset.size(); j++) {
        PixelRef pix = selset[j];
        auto relation = relations.insert(std::make_pair(pix, ShapeRef::SHAPE_EDGE));
        if (pointmap.includes(pix.right()) && pointmap.getPoint(pix.right()).selected()) {
            relation.first->second &= ~ShapeRef::SHAPE_R;
        }
        if (pointmap.includes(pix.up()) && pointmap.getPoint(pix.up()).selected()) {
            relation.first->second &= ~ShapeRef::SHAPE_T;
        }
        if (pointmap.includes(pix.down()) && pointmap.getPoint(pix.down()).selected()) {
            relation.first->second &= ~ShapeRef::SHAPE_B;
        }
        if (pointmap.includes(pix.left()) && pointmap.getPoint(pix.left()).selected()) {
            relation.first->second &= ~ShapeRef::SHAPE_L;
        }
    }
    // now find pixel with SHAPE_B | SHAPE_L
    PixelRef minpix = NoPixel;

    for (auto &relation : relations) {
        if ((relation.second & (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) == (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) {
            if ((minpix == NoPixel) || (relation.first < (int)minpix)) {
                minpix = relation.first;
            }
        }
    }
    // now follow round anticlockwise...
    SalaShape poly(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED);
    pointPixelBorder(pointmap, relations, poly, ShapeRef::SHAPE_L, minpix, minpix, true);

    for (auto relation : relations) {
        if (relation.second != 0) {
            // more than one shape!
            return -1;
        }
    }
    poly.setCentroidAreaPerim();

    int new_shape_ref = getNextShapeKey();
    m_shapes.insert(std::make_pair(new_shape_ref, poly));

    if (bounds_good) {
        // note: also sets polygon bounding box:
        makePolyPixels(new_shape_ref);
    } else {
        // pixelate all polys in the pixel new structure:
        for (auto shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    m_attributes->addRow(AttributeKey(new_shape_ref));
    m_newshape = true;

    return new_shape_ref;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::convertPointsToPolys(double poly_radius, bool selected_only) {
    // I'm not sure quite how easy this will be...
    QtRegion region;

    bool done_something = false;

    // replace the points with polys
    for (auto shape : m_shapes) {
        if (selected_only && !m_attributes->getRow(AttributeKey(shape.first)).isSelected()) {
            continue;
        }
        if (shape.second.isPoint()) {
            done_something = true;
            // remove old spatial index
            removePolyPixels(shape.first);
            // construct a poly from the point:
            Point2f p = shape.second.getCentroid();
            //
            if (region.atZero()) {
                region = QtRegion(p, p);
            }
            // replace with a polygon:
            shape.second = SalaShape(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED);
            for (int k = 0; k < 8; k++) {
                Point2f poly_p;
                if (k == 0) {
                    poly_p.x = p.x + poly_radius;
                    poly_p.y = p.y;
                } else if (k == 1) {
                    poly_p.x = p.x + poly_radius * M_ROOT_1_2;
                    poly_p.y = p.y + poly_radius * M_ROOT_1_2;
                } else if (k == 2) {
                    poly_p.x = p.x;
                    poly_p.y = p.y + poly_radius;
                } else if (k == 3) {
                    poly_p.x = p.x - poly_radius * M_ROOT_1_2;
                    poly_p.y = p.y + poly_radius * M_ROOT_1_2;
                } else if (k == 4) {
                    poly_p.x = p.x - poly_radius;
                    poly_p.y = p.y;
                } else if (k == 5) {
                    poly_p.x = p.x - poly_radius * M_ROOT_1_2;
                    poly_p.y = p.y - poly_radius * M_ROOT_1_2;
                } else if (k == 6) {
                    poly_p.x = p.x;
                    poly_p.y = p.y - poly_radius;
                } else if (k == 7) {
                    poly_p.x = p.x + poly_radius * M_ROOT_1_2;
                    poly_p.y = p.y - poly_radius * M_ROOT_1_2;
                }
                region.encompass(poly_p);
                shape.second.m_points.push_back(poly_p);
            }
            shape.second.setCentroidAreaPerim();
        }
    }

    if (done_something) {
        // spatially reindex (simplest just to redo everything)
        init(m_shapes.size(), region);

        for (auto shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::moveShape(int shaperef, const Line &line, bool undoing) {
    bool bounds_good = true;

    auto shapeIter = m_shapes.find(shaperef);
    if (shapeIter == m_shapes.end()) {
        return false;
    }

    // remove shape from the pixel grid
    removePolyPixels(shaperef); // done first, as all interface references use this list

    if (!undoing) {
        // set undo counter, but only if this is not an undo itself:
        m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_MOVED, shaperef));
        m_undobuffer.back().m_geometry = shapeIter->second;
        m_undobuffer.back().m_geometry.m_selected =
            false; // <- this m_selected really shouldn't be used -- should use attributes, but for some reason it is!
    }

    if (!(m_region.contains_touch(line.start()) && m_region.contains_touch(line.end()))) {
        bounds_good = false;
        init(m_shapes.size(), line);
    }

    shapeIter->second = SalaShape(line);

    if (bounds_good) {
        // note: also sets polygon bounding box:
        makePolyPixels(shaperef);
    } else {
        // pixelate all polys in the pixel new structure:
        for (auto shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    int rowid = std::distance(m_shapes.begin(), shapeIter);
    AttributeRow &row = m_attributes->getRow(AttributeKey(shapeIter->first));
    // change connections:
    if (m_hasgraph) {
        //
        const std::vector<int> oldconnections = m_connectors[size_t(rowid)].m_connections;
        //
        int conn_col = m_attributes->getOrInsertLockedColumn("Connectivity");
        int leng_col = -1;
        //
        if (isAxialMap()) {
            // line connections optimised for line-line intersection
            m_connectors[size_t(rowid)].m_connections =
                getLineConnections(shaperef, TOLERANCE_B * __max(m_region.height(), m_region.width()));
        } else {
            m_connectors[size_t(rowid)].m_connections =
                getShapeConnections(shaperef, TOLERANCE_B * __max(m_region.height(), m_region.width()));
        }

        std::vector<int> &newconnections = m_connectors[size_t(rowid)].m_connections;
        row.setValue(conn_col, float(newconnections.size()));
        if (isAxialMap()) {
            leng_col = m_attributes->getOrInsertLockedColumn("Line Length");
            row.setValue(leng_col, (float)depthmapX::getMapAtIndex(m_shapes, rowid)->second.getLength());
        }
        //
        // now go through our old connections, and remove ourself:
        for (int oldconnection : oldconnections) {
            if (oldconnection != rowid) { // <- exclude self!
                auto &connections = m_connectors[size_t(oldconnection)].m_connections;
                depthmapX::findAndErase(connections, rowid);
                auto &oldConnectionRow = getAttributeRowFromShapeIndex(oldconnection);
                oldConnectionRow.incrValue(conn_col, -1.0f);
            }
        }
        // now go through our new connections, and add ourself:
        for (int newconnection : m_connectors[size_t(rowid)].m_connections) {
            if (newconnection != rowid) { // <- exclude self!
                depthmapX::insert_sorted(m_connectors[size_t(newconnection)].m_connections, rowid);
                auto &newConnectionRow = getAttributeRowFromShapeIndex(newconnection);
                newConnectionRow.incrValue(conn_col);
            }
        }
        // now check any unlinks still exist in our newconnections are unlinked again (argh...)
        for (auto revIter = m_unlinks.rbegin(); revIter != m_unlinks.rend(); ++revIter) {
            int connb = -1;
            if (revIter->a == rowid)
                connb = revIter->b;
            else if (revIter->b == rowid)
                connb = revIter->a;
            if (connb != -1) {
                if (std::find(newconnections.begin(), newconnections.end(), connb) == newconnections.end()) {
                    // no longer required:
                    m_unlinks.erase(std::next(revIter).base());
                } else {
                    // enforce:
                    depthmapX::findAndErase(newconnections, connb);
                    depthmapX::findAndErase(m_connectors[size_t(connb)].m_connections, rowid);
                    auto &connbRow = getAttributeRowFromShapeIndex(connb);
                    connbRow.incrValue(conn_col, -1.0f);
                    row.incrValue(conn_col, -1.0f);
                }
            }
        }
        // now check any links are actually required (argh...)
        for (auto revIter = m_links.rbegin(); revIter != m_links.rend(); ++revIter) {
            int connb = -1;
            if (revIter->a == rowid)
                connb = revIter->b;
            else if (revIter->b == rowid)
                connb = revIter->a;
            if (connb != -1) {
                if (std::find(newconnections.begin(), newconnections.end(), connb) != newconnections.end()) {
                    // no longer required:
                    m_links.erase(std::next(revIter).base());
                } else {
                    // enforce:
                    depthmapX::insert_sorted(newconnections, connb);
                    depthmapX::insert_sorted(m_connectors[size_t(connb)].m_connections, rowid);
                    auto &connbRow = getAttributeRowFromShapeIndex(connb);
                    connbRow.incrValue(conn_col);
                    row.incrValue(conn_col);
                }
            }
        }
        // update displayed attribute for any changes:
        invalidateDisplayedAttribute();
        setDisplayedAttribute(m_displayed_attribute);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// some functions to make a polygon from the UI

int ShapeMap::polyBegin(const Line &line) {
    // add geometry
    bool bounds_good = true;
    if (!(m_region.contains_touch(line.start()) && m_region.contains_touch(line.end()))) {
        bounds_good = false;
        init(m_shapes.size(), line);
    }

    int new_shape_ref = getNextShapeKey();
    m_shapes.insert(std::make_pair(new_shape_ref, SalaShape(line)));
    m_shapes.rbegin()->second.m_centroid = line.getCentre();

    if (bounds_good) {
        // note: also sets polygon bounding box:
        makePolyPixels(new_shape_ref);
    } else {
        // pixelate all polys in the pixel new structure:
        for (auto shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    // insert into attributes
    m_attributes->addRow(AttributeKey(new_shape_ref));
    // would usually set attributes here, but actually, really want
    // to set the attributes only when the user completes the drawing

    // change connections:
    if (m_hasgraph) {
        // dummy for now to ensure there is a row in the connector table
        // so all indices match...
        m_connectors.push_back(Connector());
    }

    // flag new shape
    m_newshape = true;

    // set undo counter:
    m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_CREATED, new_shape_ref));

    // update displayed attribute
    invalidateDisplayedAttribute();
    setDisplayedAttribute(m_displayed_attribute);

    return new_shape_ref;
}

bool ShapeMap::polyAppend(int shape_ref, const Point2f &point) {
    // don't do anything too complex:
    SalaShape &shape = m_shapes.rbegin()->second;

    // check you can actually do this first
    if (!(shape.isLine() || shape.isPolyLine())) {
        return false;
    }

    // junk the old shape pixels:
    removePolyPixels(shape_ref);

    bool bounds_good = true;
    if (!m_region.contains_touch(point)) {
        bounds_good = false;
        init(m_shapes.size(), QtRegion(point, point));
    }

    if (shape.m_type == SalaShape::SHAPE_LINE) {
        // convert it to a poly line:
        shape.m_type = SalaShape::SHAPE_POLY;
        shape.m_points.push_back(shape.m_region.t_start());
        shape.m_points.push_back(shape.m_region.t_end());
    }
    // add new point:
    shape.m_points.push_back(point);

    if (bounds_good) {
        // note: also sets polygon bounding box:
        makePolyPixels(shape_ref);
    } else {
        // pixelate all polys in the pixel new structure:
        for (auto shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    shape.setCentroidAreaPerim();

    return true;
}

bool ShapeMap::polyClose(int shape_ref) {
    // don't do anything too complex:
    SalaShape &shape = m_shapes.rbegin()->second;

    // check you can actually do this first
    if (!shape.isPolyLine()) {
        return false;
    }

    // junk the old shape pixels:
    removePolyPixels(shape_ref);

    shape.m_type |= SalaShape::SHAPE_CLOSED;

    makePolyPixels(shape_ref);

    return true;
}

bool ShapeMap::polyCancel(int shape_ref) {
    // don't do anything too complex:
    SalaShape &shape = m_shapes.rbegin()->second;

    // check you can actually do this first
    if (!(shape.isLine() || shape.isPolyLine())) {
        return false;
    }

    m_undobuffer.pop_back();
    removeShape(shape_ref, true);

    // update displayed attribute
    invalidateDisplayedAttribute();
    setDisplayedAttribute(m_displayed_attribute);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::removeSelected() {
    // note, map must have editable flag on if we are to remove shape:
    if (!m_editable) {
        return false;
    }

    // pray that the selection set is in order!
    // (it should be: code currently uses add() throughout)
    for (auto &shapeRef : m_selection_set) {
        removeShape(shapeRef);
    }
    m_selection_set.clear();
    m_selection = false;

    invalidateDisplayedAttribute();
    setDisplayedAttribute(m_displayed_attribute);
    return true;
}

void ShapeMap::removeShape(int shaperef, bool undoing) {
    // remove shape from four keys: the pixel grid, the poly list, the attributes and the connections
    removePolyPixels(shaperef); // done first, as all interface references use this list

    auto shapeIter = m_shapes.find(shaperef);
    size_t rowid = std::distance(m_shapes.begin(), shapeIter);

    if (!undoing) { // <- if not currently undoing another event, then add to the undo buffer:
        m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_DELETED, shaperef));
        m_undobuffer.back().m_geometry = shapeIter->second;
        m_undobuffer.back().m_geometry.m_selected =
            false; // <- this m_selected really shouldn't be used -- should use attributes, but for some reason it is!
    }

    if (shapeIter != m_shapes.end()) {
        shapeIter = m_shapes.erase(shapeIter);
    }

    if (m_hasgraph) {
        // note that the connections have no key for speed when processing,
        // we rely on the index order matching the index order of the shapes
        // and the attributes, and simply change all references (ick!)
        int conn_col = m_attributes->getColumnIndex("Connectivity");

        // TODO: Replace with iterators
        for (size_t i = m_connectors.size() - 1; static_cast<int>(i) != -1; i--) {
            if (i == rowid) {
                continue; // it's going to be removed anyway
            }
            for (size_t j = m_connectors[i].m_connections.size() - 1; static_cast<int>(j) != -1; j--) {
                if (m_connectors[i].m_connections[j] == int(rowid)) {
                    m_connectors[i].m_connections.erase(m_connectors[i].m_connections.begin() + int(j));
                    if (conn_col != -1) {
                        auto &row = getAttributeRowFromShapeIndex(i);
                        row.incrValue(conn_col, -1.0f);
                    }
                } else if (m_connectors[i].m_connections[j] > int(rowid)) {
                    m_connectors[i].m_connections[j] -= 1;
                }
            }
            // note, you cannot delete from a segment map, it's just too messy!
        }

        m_connectors.erase(m_connectors.begin() + int(rowid));

        // take out explicit links and unlinks (note, undo won't restore these):
        for (auto revIter = m_links.rbegin(); revIter != m_links.rend(); ++revIter) {
            if (revIter->a == static_cast<int>(rowid) || revIter->b == static_cast<int>(rowid)) {
                m_links.erase(std::next(revIter).base());
            } else {
                if (revIter->a > int(rowid))
                    revIter->a -= 1;
                if (revIter->b > int(rowid))
                    revIter->b -= 1;
            }
        }
        for (auto revIter = m_unlinks.rbegin(); revIter != m_unlinks.rend(); ++revIter) {
            if (revIter->a == static_cast<int>(rowid) || revIter->b == static_cast<int>(rowid)) {
                m_unlinks.erase(std::next(revIter).base());
            } else {
                if (revIter->a > int(rowid))
                    revIter->a -= 1;
                if (revIter->b > int(rowid))
                    revIter->b -= 1;
            }
        }
    }

    // n.b., shaperef should have been used to create the row in the first place:
    const AttributeKey shapeRefKey(shaperef);
    m_attributes->removeRow(shapeRefKey);

    m_newshape = true;
    m_invalidate = true;
}

void ShapeMap::undo() {
    if (m_undobuffer.size() == 0) {
        return;
    }

    SalaEvent &event = m_undobuffer.back();

    if (event.m_action == SalaEvent::SALA_CREATED) {

        removeShape(
            event.m_shape_ref,
            true); // <- note, must tell remove shape it's an undo, or it will add this remove to the undo stack!

    } else if (event.m_action == SalaEvent::SALA_DELETED) {

        makeShape(event.m_geometry, event.m_shape_ref);
        int rowid = std::distance(m_shapes.begin(), m_shapes.find(event.m_shape_ref));
        auto &row = m_attributes->getRow(AttributeKey(event.m_shape_ref));

        if (rowid != -1 && m_hasgraph) {
            // redo connections... n.b. TO DO this is intended to use the slower "any connection" method, so it can
            // handle any sort of graph
            // ...but that doesn't exist yet, so for the moment do lines:
            //
            // insert new connector at the row:
            m_connectors[rowid] = Connector();
            //
            // now go through all connectors, ensuring they're reindexed above this one:
            // Argh!  ...but, remember the reason we're doing this is for fast processing elsewhere
            // -- this is a user triggered *undo*, they'll just have to wait:
            for (size_t i = 0; i < m_connectors.size(); i++) {
                for (size_t j = 0; j < m_connectors[i].m_connections.size(); j++) {
                    if (m_connectors[i].m_connections[j] >= rowid) {
                        m_connectors[i].m_connections[j] += 1;
                    }
                }
            }
            // it gets worse, the links and unlinks will also be all over the shop due to the inserted row:
            size_t j;
            for (j = 0; j < m_links.size(); j++) {
                if (m_links[j].a >= rowid)
                    m_links[j].a += 1;
                if (m_links[j].b >= rowid)
                    m_links[j].b += 1;
            }
            for (j = 0; j < m_unlinks.size(); j++) {
                if (m_unlinks[j].a >= rowid)
                    m_unlinks[j].a += 1;
                if (m_unlinks[j].b >= rowid)
                    m_unlinks[j].b += 1;
            }
            //
            // calculate this line's connections
            m_connectors[size_t(rowid)].m_connections =
                getLineConnections(event.m_shape_ref, TOLERANCE_B * __max(m_region.height(), m_region.width()));
            // update:
            int conn_col = m_attributes->getOrInsertLockedColumn("Connectivity");
            row.setValue(conn_col, float(m_connectors[rowid].m_connections.size()));
            //
            if (event.m_geometry.isLine()) {
                int leng_col = m_attributes->getOrInsertLockedColumn("Line Length");
                row.setValue(leng_col, (float)depthmapX::getMapAtIndex(m_shapes, rowid)->second.getLength());
            }
            //
            // now go through our connections, and add ourself:
            const std::vector<int> &connections = m_connectors[size_t(rowid)].m_connections;
            for (int connection : connections) {
                if (connection != rowid) { // <- exclude self!
                    depthmapX::insert_sorted(m_connectors[size_t(connection)].m_connections, rowid);
                    auto &row = getAttributeRowFromShapeIndex(connection);
                    row.incrValue(conn_col);
                }
            }
        }
    } else if (event.m_action == SalaEvent::SALA_MOVED) {

        moveShape(event.m_shape_ref, event.m_geometry.getLine(),
                  true); // <- note, must tell remove shape it's an undo, or it will add this remove to the undo stack!
    }

    m_undobuffer.pop_back();

    m_newshape = true;

    invalidateDisplayedAttribute();
    setDisplayedAttribute(m_displayed_attribute);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeMap::makePolyPixels(int polyref) {
    ShapeRef shapeRef = ShapeRef(polyref);
    // first add into pixels, and ensure you have a bl, tr for the set (useful for testing later)
    SalaShape &poly = m_shapes.find(polyref)->second;
    if (poly.isClosed()) {
        std::map<int, int> relations;
        for (size_t k = 0; k < poly.m_points.size(); k++) {
            int nextk = int((k + 1) % poly.m_points.size());
            Line li(poly.m_points[k], poly.m_points[nextk]);
            if (k == 0) {
                poly.m_region = li;
            } else {
                poly.m_region = runion(poly.m_region, li);
            }
            PixelRefVector pixels = pixelateLine(li);
            // debug
            // int duplicate_shaperefs = 0;
            // end debug
            for (size_t i = 0; i < pixels.size(); i++) {
                PixelRef pix = pixels[i];
                std::vector<ShapeRef> &pixShapes =
                    m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
                auto it = depthmapX::findBinary(pixShapes, shapeRef);
                if (it == pixShapes.end()) {
                    pixShapes.push_back(shapeRef);
                    it = pixShapes.end() - 1;
                }
                it->m_polyrefs.push_back(k);
                relations.insert(std::make_pair(pixels[i], ShapeRef::SHAPE_EDGE));
            }
        }
        // erase joined sides, and look for min:
        PixelRef minpix = NoPixel;
        for (auto &relation : relations) {
            PixelRef pix = relation.first;
            PixelRef nextpix;
            nextpix = pix.right();
            if (includes(nextpix)) {
                auto &pixShapes = m_pixel_shapes(static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                if (depthmapX::findBinary(pixShapes, shapeRef) != pixShapes.end()) {
                    relation.second &= ~ShapeRef::SHAPE_R;
                }
            }
            nextpix = pix.up();
            if (includes(nextpix)) {
                auto &pixShapes = m_pixel_shapes(static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                if (depthmapX::findBinary(pixShapes, shapeRef) != pixShapes.end()) {
                    relation.second &= ~ShapeRef::SHAPE_T;
                }
            }
            nextpix = pix.down();
            if (includes(nextpix)) {
                auto &pixShapes = m_pixel_shapes(static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                if (depthmapX::findBinary(pixShapes, shapeRef) != pixShapes.end()) {
                    relation.second &= ~ShapeRef::SHAPE_B;
                }
            }
            nextpix = pix.left();
            if (includes(nextpix)) {
                auto &pixShapes = m_pixel_shapes(static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                if (depthmapX::findBinary(pixShapes, shapeRef) != pixShapes.end()) {
                    relation.second &= ~ShapeRef::SHAPE_L;
                }
            }
            if ((relation.second & (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) ==
                (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) {
                if ((minpix == NoPixel) || (relation.first < int(minpix))) {
                    minpix = relation.first;
                }
            }
        }
        shapePixelBorder(relations, polyref, ShapeRef::SHAPE_L, minpix, minpix, true);
        // go through any that aren't on the outer border: this will be internal edges, and will cause problems
        // for point in polygon algorithms!

        for (auto &relation : relations) {
            PixelRef pix = relation.first;
            std::vector<ShapeRef> &pixShapes = m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
            const auto iter = depthmapX::findBinary(pixShapes, shapeRef);
            if (iter == pixShapes.end())
                throw new depthmapX::RuntimeException("Poly reference not found");
            unsigned char &tags = iter->m_tags;
            if (tags == 0x00) {
                tags |= ShapeRef::SHAPE_INTERNAL_EDGE;
            }
        }
        // now, any remaining tags are internal sides, and need to be cleared through fill
        // we could go either direction, but we just go left to right:
        for (auto &relation : relations) {
            PixelRef pix = relation.first;
            if (relation.second & ShapeRef::SHAPE_R) {
                bool lastWasNotFound = true;
                while (lastWasNotFound) {
                    PixelRef nextpix = pix.right();
                    if (!includes(nextpix)) {
                        // this shouldn't happen
                        break;
                    }
                    // returns -1 if cannot add due to already existing:
                    lastWasNotFound = false;
                    std::vector<ShapeRef> &pixelShapes =
                        m_pixel_shapes(static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                    const auto it = depthmapX::findBinary(pixelShapes, shapeRef);
                    if (it == pixelShapes.end()) {
                        lastWasNotFound = true;
                        pixelShapes.push_back(ShapeRef(polyref, ShapeRef::SHAPE_CENTRE));
                    }
                    pix = nextpix;
                }
            }
        }
        // Done...! This polygon is registered in the pixel polygon structure
    } else {
        // Open shapes much easier!
        switch (poly.m_type & SalaShape::SHAPE_TYPE) {
        case SalaShape::SHAPE_POINT: {
            PixelRef pix = pixelate(poly.m_centroid);
            std::vector<ShapeRef> &pixShapes = m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
            const auto it = depthmapX::findBinary(pixShapes, shapeRef);
            if (it == pixShapes.end()) {
                pixShapes.push_back(ShapeRef(polyref, ShapeRef::SHAPE_OPEN));
            }
        } break;
        case SalaShape::SHAPE_LINE: {
            PixelRefVector pixels = pixelateLine(poly.m_region);
            for (size_t i = 0; i < pixels.size(); i++) {
                PixelRef pix = pixels[i];
                std::vector<ShapeRef> &pixShapes =
                    m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
                const auto it = depthmapX::findBinary(pixShapes, shapeRef);
                if (it == pixShapes.end()) {
                    pixShapes.push_back(ShapeRef(polyref, ShapeRef::SHAPE_OPEN));
                }
            }
        } break;
        case SalaShape::SHAPE_POLY:
            for (size_t k = 0; k < poly.m_points.size() - 1; k++) {
                int nextk = (k + 1);
                Line li(poly.m_points[k], poly.m_points[nextk]);
                if (k == 0) {
                    poly.m_region = li;
                } else {
                    poly.m_region = runion(poly.m_region, li);
                }
                PixelRefVector pixels = pixelateLine(li);
                for (size_t i = 0; i < pixels.size(); i++) {
                    PixelRef pix = pixels[i];
                    std::vector<ShapeRef> &pixShapes =
                        m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
                    auto it = depthmapX::findBinary(pixShapes, shapeRef);
                    if (it == pixShapes.end()) {
                        pixShapes.push_back(ShapeRef(polyref, ShapeRef::SHAPE_OPEN));
                        it = pixShapes.end() - 1;
                    }
                    it->m_polyrefs.push_back(k);
                }
            }
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeMap::shapePixelBorder(std::map<int, int> &relations, int polyref, int side, PixelRef currpix,
                                PixelRef minpix, bool first) {
    if (!first && currpix == minpix && side == ShapeRef::SHAPE_L) {
        // looped:
        return;
    }
    auto relation = relations.find(currpix);
    if (relation->second & side) {
        std::vector<ShapeRef> &pixShapes =
            m_pixel_shapes(static_cast<size_t>(currpix.y), static_cast<size_t>(currpix.x));
        const auto iter = depthmapX::findBinary(pixShapes, ShapeRef(polyref));
        if (iter == pixShapes.end())
            throw new depthmapX::RuntimeException("Poly reference not found");
        iter->m_tags |= side;
        relation->second &= ~side; // <- clear to check all have been done later
        side <<= 1;
        if (side > ShapeRef::SHAPE_T) {
            side = ShapeRef::SHAPE_L;
        }
        shapePixelBorder(relations, polyref, side, currpix, minpix, false);
    } else {
        currpix.move(moveDir(side));
        side >>= 1;
        if (side < ShapeRef::SHAPE_L) {
            side = ShapeRef::SHAPE_T;
        }
        shapePixelBorder(relations, polyref, side, currpix, minpix, false);
    }
}

// note that this is almost exactly the same as shapePixelBorder
void ShapeMap::pointPixelBorder(const PointMap &pointmap, std::map<int, int> &relations, SalaShape &poly, int side,
                                PixelRef currpix, PixelRef minpix, bool first) {
    if (!first && currpix == minpix && side == ShapeRef::SHAPE_L) {
        // looped:
        return;
    }
    auto relation = relations.find(currpix);
    if (relation->second & side) {
        poly.m_points.push_back(pointmap.depixelate(currpix) + pointOffset(pointmap, side));
        relation->second &= ~side; // <- clear to check all have been done later
        side <<= 1;
        if (side > ShapeRef::SHAPE_T) {
            side = ShapeRef::SHAPE_L;
        }
        pointPixelBorder(pointmap, relations, poly, side, currpix, minpix, false);
    } else {
        currpix.move(moveDir(side));
        side >>= 1;
        if (side < ShapeRef::SHAPE_L) {
            side = ShapeRef::SHAPE_T;
        }
        pointPixelBorder(pointmap, relations, poly, side, currpix, minpix, false);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeMap::removePolyPixels(int polyref) {
    auto shapeIter = m_shapes.find(polyref);
    if (shapeIter == m_shapes.end()) {
        return;
    }
    SalaShape &poly = shapeIter->second;
    if (poly.isClosed()) {
        // easiest just to use scan lines to find internal pixels rather than trace a complex border:
        PixelRef minpix = pixelate(poly.m_region.bottom_left);
        PixelRef maxpix = pixelate(poly.m_region.top_right);
        for (int x = minpix.x; x <= maxpix.x; x++) {
            for (int y = minpix.y; y <= maxpix.y; y++) {
                std::vector<ShapeRef> &pixShapes = m_pixel_shapes(static_cast<size_t>(y), static_cast<size_t>(x));
                const auto it = depthmapX::findBinary(pixShapes, ShapeRef(polyref));
                if (it != pixShapes.end())
                    pixShapes.erase(it);
            }
        }
    } else {
        // open shapes easier still, as no need to find internal pixels:
        switch (poly.m_type & SalaShape::SHAPE_TYPE) {
        case SalaShape::SHAPE_POINT: {
            PixelRef pix = pixelate(poly.m_centroid);
            std::vector<ShapeRef> &pixShapes = m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
            const auto it = depthmapX::findBinary(pixShapes, ShapeRef(polyref));
            if (it != pixShapes.end())
                pixShapes.erase(it);
        } break;
        case SalaShape::SHAPE_LINE: {
            PixelRefVector list = pixelateLine(poly.m_region);
            for (size_t i = 0; i < list.size(); i++) {
                std::vector<ShapeRef> &pixShapes =
                    m_pixel_shapes(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
                const auto it = depthmapX::findBinary(pixShapes, ShapeRef(polyref));
                if (it != pixShapes.end())
                    pixShapes.erase(it);
            }
        } break;
        case SalaShape::SHAPE_POLY:
            for (size_t k = 0; k < poly.m_points.size() - 1; k++) {
                size_t nextk = (k + 1);
                Line li(poly.m_points[k], poly.m_points[nextk]);
                PixelRefVector list = pixelateLine(li);
                for (size_t i = 0; i < list.size(); i++) {
                    std::vector<ShapeRef> &pixShapes =
                        m_pixel_shapes(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
                    const auto it = depthmapX::findBinary(pixShapes, ShapeRef(polyref));
                    if (it != pixShapes.end())
                        pixShapes.erase(it);
                }
            }
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int ShapeMap::moveDir(int side) {
    int dir = PixelRef::NODIR;
    switch (side) {
    case ShapeRef::SHAPE_L:
        dir = PixelRef::NEGHORIZONTAL;
        break;
    case ShapeRef::SHAPE_B:
        dir = PixelRef::NEGVERTICAL;
        break;
    case ShapeRef::SHAPE_R:
        dir = PixelRef::HORIZONTAL;
        break;
    case ShapeRef::SHAPE_T:
        dir = PixelRef::VERTICAL;
        break;
    }
    return dir;
}

Point2f ShapeMap::pointOffset(const PointMap &pointmap, int side) {
    Point2f p;
    switch (side) {
    case ShapeRef::SHAPE_L:
        p = Point2f(-pointmap.getSpacing() / 2, 0.0);
        break;
    case ShapeRef::SHAPE_B:
        p = Point2f(0.0, -pointmap.getSpacing() / 2);
        break;
    case ShapeRef::SHAPE_R:
        p = Point2f(pointmap.getSpacing() / 2, 0.0);
        break;
    case ShapeRef::SHAPE_T:
        p = Point2f(0.0, pointmap.getSpacing() / 2);
        break;
    }
    return p;
}

// Point in poly testing (returns topmost displayed poly)

int ShapeMap::pointInPoly(const Point2f &p) const {
    if (!m_region.contains(p)) {
        return -1;
    }
    std::vector<size_t> testedshapes;
    PixelRef pix = pixelate(p);
    const std::vector<ShapeRef> &shapes = m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    int drawlast = -1;
    int draworder = -1;

    for (const ShapeRef &shape : shapes) {
        auto iter = depthmapX::findBinary(testedshapes, shape.m_shape_ref);
        if (iter != testedshapes.end()) {
            continue;
        }
        testedshapes.insert(iter, int(shape.m_shape_ref));

        int shapeindex = testPointInPoly(p, shape);

        // if there's a shapeindex, then add:
        int currentDrawOrder = m_attribHandle->findInIndex(AttributeKey(shape.m_shape_ref));
        if (shapeindex != -1 && currentDrawOrder > draworder) {
            drawlast = shapeindex;
            draworder = currentDrawOrder;
        }
    }
    return drawlast;
}

// Point in specific poly (by reference)

bool ShapeMap::pointInPoly(const Point2f &p, int polyref) const {
    PixelRef pix = pixelate(p);
    const std::vector<ShapeRef> &shapes = m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    const auto iter = depthmapX::findBinary(shapes, ShapeRef(polyref));
    if (iter != shapes.end()) {
        return (testPointInPoly(p, *iter) != -1);
    }
    return false;
}

// similar to above, but builds a list

std::vector<int> ShapeMap::pointInPolyList(const Point2f &p) const {
    std::vector<int> shapeindexlist;
    if (!m_region.contains(p)) {
        return shapeindexlist;
    }
    std::vector<size_t> testedshapes;
    PixelRef pix = pixelate(p);
    const std::vector<ShapeRef> &shapes = m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    for (const ShapeRef &shape : shapes) {
        auto iter = depthmapX::findBinary(testedshapes, shape.m_shape_ref);
        if (iter != testedshapes.end()) {
            continue;
        }
        testedshapes.insert(iter, int(shape.m_shape_ref));

        int shapeindex = testPointInPoly(p, shape);

        // if there's a shapeindex, then add (note it is an add -- you may be passed a list again to expand)
        if (shapeindex != -1) {
            shapeindexlist.push_back(shapeindex);
        }
    }
    std::sort(shapeindexlist.begin(), shapeindexlist.end());
    return shapeindexlist;
}

// note, lineref is only used as an "exclude self" test when called from getShapeConnections
std::vector<int> ShapeMap::lineInPolyList(const Line &li_orig, size_t lineref, double tolerance) const {
    std::vector<int> shapeindexlist;
    if (!intersect_region(m_region, li_orig)) {
        return shapeindexlist;
    }
    Line li = li_orig;
    if (!m_region.contains(li.start()) || !m_region.contains(li.end())) {
        li.crop(m_region);
    }

    shapeindexlist = pointInPolyList(li.start());
    std::vector<int> endShapeIndexList = pointInPolyList(li.end());
    shapeindexlist.insert(shapeindexlist.end(), endShapeIndexList.begin(), endShapeIndexList.end());

    // only now pixelate and test for any other shapes:
    PixelRefVector list = pixelateLine(li);
    for (size_t i = 0; i < list.size(); i++) {
        PixelRef pix = list[i];
        if (includes(pix)) {
            const std::vector<ShapeRef> &shapes =
                m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
            for (const ShapeRef &shape : shapes) {
                // slow to do this as it can repeat -- really need to use a linetest like structure to avoid retest of
                // polygon lines
                if (shape.m_shape_ref != lineref &&
                    shape.m_tags & (ShapeRef::SHAPE_EDGE | ShapeRef::SHAPE_INTERNAL_EDGE | ShapeRef::SHAPE_OPEN)) {
                    auto shapeIter = m_shapes.find(shape.m_shape_ref);
                    const SalaShape &poly = shapeIter->second;
                    switch (poly.m_type & (SalaShape::SHAPE_LINE | SalaShape::SHAPE_POLY)) {
                    case SalaShape::SHAPE_LINE:
                        if (intersect_region(li, poly.m_region)) {
                            // note: in this case m_region is stored as a line:
                            if (intersect_line(li, poly.m_region, tolerance)) {
                                shapeindexlist.push_back(int(std::distance(m_shapes.begin(), shapeIter)));
                            }
                        }
                        break;
                    case SalaShape::SHAPE_POLY: {
                        for (size_t k = 0; k < shape.m_polyrefs.size(); k++) {
                            Line lineb = Line(poly.m_points[shape.m_polyrefs[k]],
                                              poly.m_points[((shape.m_polyrefs[k] + 1) % poly.m_points.size())]);
                            if (intersect_region(li, lineb)) {
                                if (intersect_line(li, lineb, tolerance)) {
                                    shapeindexlist.push_back(int(std::distance(m_shapes.begin(), shapeIter)));
                                }
                            }
                        }
                    } break;
                    default:
                        break;
                    }
                }
            }
        }
    }
    std::sort(shapeindexlist.begin(), shapeindexlist.end());
    return shapeindexlist;
}

std::vector<int> ShapeMap::polyInPolyList(int polyref, double tolerance) const {
    std::vector<int> shapeindexlist;
    auto shapeIter = m_shapes.find(polyref);
    if (shapeIter == m_shapes.end()) {
        return shapeindexlist;
    }
    const SalaShape &poly = shapeIter->second;
    if (poly.isClosed()) { // <- it ought to be, you shouldn't be using this function if not!
        std::vector<size_t> testedlist;
        // easiest just to use scan lines to find internal pixels rather than trace a complex border:
        PixelRef minpix = pixelate(poly.m_region.bottom_left);
        PixelRef maxpix = pixelate(poly.m_region.top_right);
        // pass one: shape centre of either object coincident automatically adds
        int x;
        for (x = minpix.x; x <= maxpix.x; x++) {
            for (int y = minpix.y; y <= maxpix.y; y++) {
                const std::vector<ShapeRef> &pixShapes =
                    m_pixel_shapes(static_cast<size_t>(y), static_cast<size_t>(x));
                const auto iter = depthmapX::findBinary(pixShapes, ShapeRef(polyref));
                if (iter != pixShapes.end()) {
                    // this has us in it, now looked through everything else:
                    for (const ShapeRef &shapeRef : pixShapes) {
                        if (*iter != shapeRef &&
                            ((iter->m_tags & ShapeRef::SHAPE_CENTRE) || (shapeRef.m_tags & ShapeRef::SHAPE_CENTRE))) {
                            auto iter = depthmapX::findBinary(testedlist, shapeRef.m_shape_ref);
                            if (iter == testedlist.end()) {
                                testedlist.insert(iter, shapeRef.m_shape_ref);
                                shapeindexlist.push_back(
                                    int(depthmapX::findIndexFromKey(m_shapes, int(shapeRef.m_shape_ref))));
                            }
                        }
                    }
                }
            }
        }
        // that was the easy bit... now, pass 2, for non centre things:
        for (x = minpix.x; x <= maxpix.x; x++) {
            for (int y = minpix.y; y <= maxpix.y; y++) {
                const std::vector<ShapeRef> &pixShapes =
                    m_pixel_shapes(static_cast<size_t>(y), static_cast<size_t>(x));
                const auto iter = depthmapX::findBinary(pixShapes, ShapeRef(polyref));
                if (iter != pixShapes.end()) {
                    const ShapeRef &shaperef = *iter;
                    if ((shaperef.m_tags & ShapeRef::SHAPE_CENTRE) == 0) {
                        // this has us in it, now looked through everything else:
                        for (auto &shaperefb : pixShapes) {
                            auto iter = depthmapX::findBinary(testedlist, shaperefb.m_shape_ref);
                            if (shaperef != shaperefb && iter == testedlist.end()) {
                                auto shapeIter = m_shapes.find(shaperefb.m_shape_ref);
                                size_t indexb = std::distance(m_shapes.begin(), shapeIter);
                                const SalaShape &polyb = shapeIter->second;
                                if (polyb.isPoint()) {
                                    if (testPointInPoly(polyb.getPoint(), shaperef) != -1) {
                                        shapeindexlist.push_back(int(indexb));
                                    }
                                } else if (polyb.isLine()) {
                                    if (testPointInPoly(polyb.getLine().start(), shaperef) != -1 ||
                                        testPointInPoly(polyb.getLine().end(), shaperef) != -1) {
                                        testedlist.insert(iter, shaperefb.m_shape_ref);
                                        shapeindexlist.push_back(int(indexb));
                                    } else {
                                        for (size_t k = 0; k < shaperef.m_polyrefs.size(); k++) {
                                            Line line = Line(
                                                poly.m_points[shaperef.m_polyrefs[k]],
                                                poly.m_points[((shaperef.m_polyrefs[k] + 1) % poly.m_points.size())]);
                                            if (intersect_region(line, polyb.getLine())) {
                                                if (intersect_line(line, polyb.getLine(), tolerance)) {
                                                    testedlist.insert(iter, shaperefb.m_shape_ref);
                                                    shapeindexlist.push_back(int(indexb));
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                } else if (polyb.isPolyLine()) {
                                    if (testPointInPoly(polyb.m_points[shaperefb.m_polyrefs[0]], shaperef) != -1) {
                                        testedlist.insert(iter, shaperefb.m_shape_ref);
                                        shapeindexlist.push_back(int(indexb));
                                    } else {
                                        for (size_t k = 0; k < shaperef.m_polyrefs.size(); k++) {
                                            for (size_t kk = 0; kk < shaperefb.m_polyrefs.size(); kk++) {
                                                Line line = Line(poly.m_points[shaperef.m_polyrefs[k]],
                                                                 poly.m_points[((shaperef.m_polyrefs[k] + 1) %
                                                                                poly.m_points.size())]);
                                                Line lineb = Line(polyb.m_points[shaperefb.m_polyrefs[kk]],
                                                                  polyb.m_points[((shaperefb.m_polyrefs[kk] + 1) %
                                                                                  polyb.m_points.size())]);
                                                if (intersect_region(line, lineb)) {
                                                    if (intersect_line(line, lineb, tolerance)) {
                                                        auto iterInternal =
                                                            depthmapX::findBinary(testedlist, shaperefb.m_shape_ref);
                                                        if (iterInternal == testedlist.end()) {
                                                            testedlist.insert(iterInternal, shaperefb.m_shape_ref);
                                                            shapeindexlist.push_back(int(indexb));
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    // poly to poly, ick!
                                    // first test one entirely inside the other
                                    // any point at all will suffice to check this: however, we need to check that the
                                    // polyref point *itself* is within the pixel, not just part of the line associated
                                    // with it...
                                    if ((pixelate(polyb.m_points[shaperefb.m_polyrefs[0]]) == PixelRef(x, y) &&
                                         testPointInPoly(polyb.m_points[shaperefb.m_polyrefs[0]], shaperef) != -1) ||
                                        (pixelate(poly.m_points[shaperef.m_polyrefs[0]]) == PixelRef(x, y) &&
                                         testPointInPoly(poly.m_points[shaperef.m_polyrefs[0]], shaperefb) != -1)) {
                                        testedlist.insert(iter, shaperefb.m_shape_ref);
                                        shapeindexlist.push_back(int(indexb));
                                    } else {
                                        // now check crossing
                                        bool breakit = false;
                                        for (size_t k = 0; k < shaperef.m_polyrefs.size() && !breakit; k++) {
                                            for (size_t kk = 0; kk < shaperefb.m_polyrefs.size(); kk++) {
                                                Line line = Line(poly.m_points[shaperef.m_polyrefs[k]],
                                                                 poly.m_points[((shaperef.m_polyrefs[k] + 1) %
                                                                                poly.m_points.size())]);
                                                Line lineb = Line(polyb.m_points[shaperefb.m_polyrefs[kk]],
                                                                  polyb.m_points[((shaperefb.m_polyrefs[kk] + 1) %
                                                                                  polyb.m_points.size())]);
                                                if (intersect_region(line, lineb)) {
                                                    if (intersect_line(line, lineb, tolerance)) {
                                                        testedlist.insert(iter, shaperefb.m_shape_ref);
                                                        shapeindexlist.push_back(int(indexb));
                                                        breakit = true;
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    } else {
        throw depthmapX::RuntimeException("this function is to be used for polygons only");
    }
    std::sort(shapeindexlist.begin(), shapeindexlist.end());
    return shapeindexlist;
}

std::vector<int> ShapeMap::shapeInPolyList(const SalaShape &shape) // note: no const due to poly in poly testing
{
    std::vector<int> shapeindexlist;
    if (!intersect_region(m_region, shape.m_region)) {
        // quick test that actually coincident
        return shapeindexlist;
    }
    if (shape.isPoint()) {
        shapeindexlist = pointInPolyList(shape.getPoint());
    } else if (shape.isLine()) {
        shapeindexlist = lineInPolyList(shape.getLine());
    } else if (shape.isPolyLine()) {
        for (size_t i = 1; i < shape.m_points.size() - 1; i++) {
            Line li(shape.m_points[i], shape.m_points[i - 1]);
            shapeindexlist = lineInPolyList(li);
        }
    } else {
        // first *add* the poly temporarily (note this may grow pixel set):
        int ref = makePolyShape(shape.m_points, false, true); // false is closed poly, true is temporary shape
        // do test:
        shapeindexlist = polyInPolyList(ref);
        // clean up:
        removePolyPixels(ref);
        m_shapes.erase(m_shapes.find(ref));
    }
    return shapeindexlist;
}

// helper for point in poly --
// currently needs slight rewrite to avoid problem if point is in line with a vertex
// (counter incremented twice on touching implies not in poly when is)

int ShapeMap::testPointInPoly(const Point2f &p, const ShapeRef &shape) const {
    auto shapeIter = m_shapes.end();
    // simplist: in shape centre
    if (shape.m_tags & ShapeRef::SHAPE_CENTRE) {
        shapeIter = m_shapes.find(shape.m_shape_ref);
    }
    // check not an open shape (cannot be inside)
    else if ((shape.m_tags & ShapeRef::SHAPE_OPEN) == 0) {
        shapeIter = m_shapes.find(shape.m_shape_ref);
        const SalaShape &poly = shapeIter->second;
        if (poly.m_region.contains_touch(p)) {
            // next simplest, on the outside border:
            int alpha = 0;
            int counter = 0;
            int parity = 0;
            if (shape.m_tags & ShapeRef::SHAPE_EDGE) {
                // run a test line to the edge:
                if (shape.m_tags & (ShapeRef::SHAPE_L | ShapeRef::SHAPE_R)) {
                    if (shape.m_tags & ShapeRef::SHAPE_L) {
                        parity = -1;
                    } else if (shape.m_tags & ShapeRef::SHAPE_R) {
                        parity = +1;
                    }
                    for (size_t j = 0; j < shape.m_polyrefs.size(); j++) {
                        Line lineb = Line(poly.m_points[shape.m_polyrefs[j]],
                                          poly.m_points[((shape.m_polyrefs[j] + 1) % poly.m_points.size())]);
                        if (lineb.bottom_left.y <= p.y && lineb.top_right.y >= p.y) {
                            // crosses or touches... but we need to check
                            // touching exception:
                            if (lineb.t_start().y == p.y) {
                                if (parity * lineb.t_start().x >= parity * p.x) {
                                    alpha -= 1;
                                    counter++;
                                }
                            }
                            // the other touching exception
                            else if (lineb.t_end().y == p.y) {
                                if (parity * lineb.t_end().x >= parity * p.x) {
                                    alpha += 1;
                                    // n.b., no counter here
                                }
                            }
                            // at this stage we know the line isn't horizontal, so we can find the intersection point:
                            else if (parity * (lineb.grad(XAXIS) * (p.y - lineb.ay()) + lineb.ax()) >= parity * p.x) {
                                counter++;
                            }
                        }
                    }
                } else {
                    if (shape.m_tags & ShapeRef::SHAPE_B) {
                        parity = -1;
                    } else if (shape.m_tags & ShapeRef::SHAPE_T) {
                        parity = +1;
                    }
                    for (size_t j = 0; j < shape.m_polyrefs.size(); j++) {
                        Line lineb = Line(poly.m_points[shape.m_polyrefs[j]],
                                          poly.m_points[((shape.m_polyrefs[j] + 1) % poly.m_points.size())]);
                        if (lineb.bottom_left.x <= p.x && lineb.top_right.x >= p.x) {
                            // crosses or touches... but we need to check
                            // touching exception:
                            if (lineb.top_right.x == p.x) {
                                if (parity * lineb.by() >= parity * p.y) {
                                    alpha -= 1;
                                    counter++;
                                }
                            }
                            // the other touching exception
                            else if (lineb.bottom_left.x == p.x) {
                                if (parity * lineb.ay() >= parity * p.y) {
                                    alpha += 1;
                                    // n.b., no counter here
                                }
                            }
                            // at this stage we know the line isn't vertical, so we can find the intersection point:
                            else if (parity * (lineb.grad(YAXIS) * (p.x - lineb.ax()) + lineb.ay()) >= parity * p.y) {
                                counter++;
                            }
                        }
                    }
                }
                if (counter % 2 != 0 && alpha == 0) {
                    shapeIter = m_shapes.find(shape.m_shape_ref);
                }
            }
            // and now the pig -- it's somewhere in the middle of the poly:
            else if (shape.m_tags & ShapeRef::SHAPE_INTERNAL_EDGE) {
                std::vector<int> testnodes;
                size_t j;
                for (j = 0; j < size_t(shape.m_polyrefs.size()); j++) {
                    depthmapX::addIfNotExists(testnodes, int(shape.m_polyrefs[j]));
                }
                PixelRef pix2 = pixelate(p);
                const std::vector<ShapeRef> &pixelShapes =
                    m_pixel_shapes(static_cast<size_t>(pix2.y), static_cast<size_t>(pix2.x));
                // bit of code duplication like this, but easier on params to this function:
                pix2.move(PixelRef::NEGVERTICAL); // move pix2 down, search for this shape...
                auto iter = std::find(pixelShapes.begin(), pixelShapes.end(), shape.m_shape_ref);
                while (iter != pixelShapes.end()) {
                    for (size_t k = 0; k < iter->m_polyrefs.size(); k++) {
                        depthmapX::addIfNotExists(testnodes, int(iter->m_polyrefs[k]));
                    }
                    pix2.move(PixelRef::NEGVERTICAL); // move pix2 down, search for this shape...
                    if (includes(pix2)) {
                        iter = std::find(pixelShapes.begin(), pixelShapes.end(), shape.m_shape_ref);
                    } else {
                        iter = pixelShapes.end();
                    }
                }
                int alpha = 0;
                int counter = 0;
                int parity = -1;
                for (j = 0; j < testnodes.size(); j++) {
                    Line lineb =
                        Line(poly.m_points[testnodes[j]], poly.m_points[((testnodes[j] + 1) % poly.m_points.size())]);
                    if (lineb.bottom_left.x <= p.x && lineb.top_right.x >= p.x) {
                        // crosses or touches... but we need to check
                        // touching exception:
                        if (lineb.top_right.x == p.x) {
                            if (parity * lineb.by() >= parity * p.y) {
                                alpha -= 1;
                                counter++;
                            }
                        }
                        // the other touching exception
                        else if (lineb.bottom_left.x == p.x) {
                            if (parity * lineb.ay() >= parity * p.y) {
                                alpha += 1;
                                // n.b., no counter here
                            }
                        }
                        // at this stage we know the line isn't vertical, so we can find the intersection point:
                        else if (parity * (lineb.grad(YAXIS) * (p.x - lineb.ax()) + lineb.ay()) >= parity * p.y) {
                            counter++;
                        }
                    }
                }
                if (counter % 2 != 0 && alpha == 0) {
                    shapeIter = m_shapes.find(shape.m_shape_ref);
                }
            }
        }
    }
    return (shapeIter == m_shapes.end()) ? -1 : std::distance(m_shapes.begin(), shapeIter); // note convert to -1
}

// also note that you may want to find a close poly line or point
// if you can't find a point in poly (or even if you can)

// (see also getClosestVertex below)

// returns a rowid *not* a shape key

int ShapeMap::getClosestOpenGeom(const Point2f &p) const {
    if (!m_region.contains(p)) {
        return -1;
    }

    PixelRef pix = pixelate(p);

    auto shapeIter = m_shapes.end();
    double mindist = -1;
    const std::vector<ShapeRef> &shapeRefs = m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    for (const ShapeRef &ref : shapeRefs) {
        if (ref.m_tags & ShapeRef::SHAPE_OPEN) {
            double thisdist = -1.0;
            auto tempShapeIter = m_shapes.find(ref.m_shape_ref);
            const SalaShape &poly = tempShapeIter->second;
            switch (poly.m_type) {
            case SalaShape::SHAPE_POINT:
                thisdist = dist(p, poly.m_centroid);
            case SalaShape::SHAPE_LINE:
                thisdist = dist(p, poly.m_region); // note, in this case m_region is a line
                break;
            case SalaShape::SHAPE_POLY:
            case SalaShape::SHAPE_POLY | SalaShape::SHAPE_CCW: // note CCW should never have happened, but it has
                for (size_t j = 0; j < ref.m_polyrefs.size(); j++) {
                    Line line(poly.m_points[ref.m_polyrefs[j]], poly.m_points[ref.m_polyrefs[j] + 1]);
                    double tempthisdist = dist(p, line);
                    if (tempthisdist != -1 && (thisdist == -1 || tempthisdist < thisdist)) {
                        thisdist = tempthisdist;
                    }
                }
                break;
            }
            if (thisdist != -1.0 && (mindist == -1 || thisdist < mindist)) {
                mindist = thisdist;
                shapeIter = tempShapeIter;
            }
        }
    }

    return (shapeIter == m_shapes.end()) ? -1 : std::distance(m_shapes.begin(), shapeIter); // note conversion to -1
}

Point2f ShapeMap::getClosestVertex(const Point2f &p) const {
    Point2f vertex; // null by default

    if (!m_region.contains(p)) {
        return vertex; // will be null in this case
    }

    PixelRef pix = pixelate(p);

    double mindist = -1.0;
    const std::vector<ShapeRef> &shapeRefs = m_pixel_shapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    for (const ShapeRef &ref : shapeRefs) {
        double thisdist = -1.0;
        Point2f thisvertex;
        const SalaShape &poly = m_shapes.find(ref.m_shape_ref)->second;
        switch (poly.m_type) {
        case SalaShape::SHAPE_POINT:
            thisvertex = poly.m_centroid;
            thisdist = dist(p, thisvertex);
            break;
        case SalaShape::SHAPE_LINE: {
            double d1 = dist(p, poly.m_region.start());
            double d2 = dist(p, poly.m_region.end());
            if (d1 < d2) {
                thisvertex = poly.m_region.start();
                thisdist = d1;
            } else {
                thisvertex = poly.m_region.end();
                thisdist = d2;
            }
        } break;
        default: // either a poly line or a polygon
            for (size_t j = 0; j < ref.m_polyrefs.size(); j++) {
                double d1 = dist(p, poly.m_points[ref.m_polyrefs[j]]);
                // note this can be used for both open / closed with the % poly.size()
                double d2 = dist(p, poly.m_points[(ref.m_polyrefs[j] + 1) % poly.m_points.size()]);
                if (thisdist == -1 || d1 < thisdist) {
                    thisvertex = poly.m_points[ref.m_polyrefs[j]];
                    thisdist = d1;
                }
                if (d2 < thisdist) {
                    thisvertex = poly.m_points[(ref.m_polyrefs[j] + 1) % poly.m_points.size()];
                    thisdist = d2;
                }
            }
            break;
        }
        if (thisdist != -1.0 && (mindist == -1.0 || thisdist < mindist)) {
            mindist = thisdist;
            vertex = thisvertex;
        }
    }

    return vertex;
}

// nb, uses full BSP tree test:

#include "isovist.h"

int ShapeMap::getClosestLine(const Point2f &p) const {
    // not the best place to check this, but we must all the same:
    if (m_newshape) {
        m_bsp_tree = false;
    }

    if (!m_bsp_tree) {
        makeBSPtree();
    }

    int index = -1;

    if (m_bsp_tree) { // <- check there is actually something in the tree!
        Isovist iso;
        index = iso.getClosestLine(m_bsp_root, p);
    }

    return index;
}

// code to add intersections when shapes are added to the graph one by one:
int ShapeMap::connectIntersected(int rowid, bool linegraph) {
    auto shaperefIter = depthmapX::getMapAtIndex(m_shapes, rowid);
    int conn_col = m_attributes->getOrInsertLockedColumn("Connectivity");
    int leng_col = -1;
    if (linegraph) {
        // historically line length has always been added at this point
        leng_col = m_attributes->getOrInsertLockedColumn("Line Length");
    }
    // all indices should match... this grows connectors if necessary to same length as shapes
    while (m_connectors.size() < m_shapes.size()) {
        m_connectors.push_back(Connector());
    }
    m_connectors[size_t(rowid)].m_connections =
        linegraph ? getLineConnections(shaperefIter->first, TOLERANCE_B * __max(m_region.height(), m_region.width()))
                  : getShapeConnections(shaperefIter->first, TOLERANCE_B * __max(m_region.height(), m_region.width()));

    auto &row = getAttributeRowFromShapeIndex(rowid);
    row.setValue(conn_col, float(m_connectors[size_t(rowid)].m_connections.size()));
    if (linegraph) {
        row.setValue(leng_col, (float)shaperefIter->second.getLength());
    }
    // now go through our connections, and add ourself:
    const std::vector<int> &connections = m_connectors[size_t(rowid)].m_connections;
    for (int connection : connections) {
        if (connection != rowid) { // <- exclude self!
            depthmapX::insert_sorted(m_connectors[size_t(connection)].m_connections, rowid);
            auto &connectionRow = getAttributeRowFromShapeIndex(connection);
            connectionRow.incrValue(conn_col);
        }
    }
    return m_connectors[size_t(rowid)].m_connections.size();
}

// this assumes this is a line map (to speed up axial map creation)
// use the other version, getShapeConnections for arbitrary shape-shape connections
// note, connections are listed by rowid in list, *not* reference number
// (so they may vary: must be checked carefully when shapes are removed / added)
std::vector<int> ShapeMap::getLineConnections(int lineref, double tolerance) {
    std::vector<int> connections;

    SalaShape &poly = m_shapes.find(lineref)->second;
    if (!poly.isLine()) {
        return std::vector<int>();
    }
    const Line &l = poly.getLine();

    std::unordered_set<ShapeRef, ShapeRefHash> shapesToTest;

    // As of version 10, self-connections are *not* added
    // In the past:
    // <exclude> it's useful to have yourself in your connections list
    // (apparently! -- this needs checking, as most of the time it is then checked to exclude self again!) </exclude>
    // <exclude> connections.add(m_shapes.searchindex(lineref)); </exclude>

    shapesToTest.insert(lineref);

    PixelRefVector list = pixelateLine(l);

    for (size_t i = 0; i < list.size(); i++) {
        const std::vector<ShapeRef> &shapeRefs =
            m_pixel_shapes(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
        for (const ShapeRef &shape : shapeRefs) {
            shapesToTest.insert(shape);
        }
    }
    for (ShapeRef shape : shapesToTest) {
        if ((shape.m_tags & ShapeRef::SHAPE_OPEN) == ShapeRef::SHAPE_OPEN) {
            const Line &line = m_shapes.find(int(shape.m_shape_ref))->second.getLine();
            if (intersect_region(line, l, line.length() * tolerance)) {
                // n.b. originally this followed the logic that we must normalise intersect_line properly: tolerance *
                // line length one * line length two in fact, works better if it's just line.length() * tolerance...
                if (intersect_line(line, l, line.length() * tolerance)) {
                    depthmapX::insert_sorted(connections,
                                             depthmapX::findIndexFromKey(m_shapes, int(shape.m_shape_ref)));
                }
            }
        }
    }

    return connections;
}

// this is only problematic as there is lots of legacy code with shape-in-shape testing,
std::vector<int> ShapeMap::getShapeConnections(int shaperef, double tolerance) {
    // In versions prior to 10, note that unlike getLineConnections, self-connection is excluded by all of the
    // following functions As of version 10, both getShapeConnections and getLineConnections exclude self-connection

    std::vector<int> connections;

    auto shapeIter = m_shapes.find(shaperef);
    if (shapeIter != m_shapes.end()) {
        SalaShape &shape = shapeIter->second;
        if (shape.isPoint()) {
            // a point is simple, it never intersects itself:
            connections = pointInPolyList(shape.getPoint());
        } else if (shape.isPolygon()) {
            // a closed poly is actually quite simple too as we already have code using a polyref:
            connections = polyInPolyList(shaperef, tolerance);
        } else if (shape.isLine()) {
            // line is a bit slow because there's no tested shape as in getLineConnections, but similar:
            connections = lineInPolyList(shape.getLine(), shaperef, tolerance);
        } else if (shape.isPolyLine()) {
            // this is the worst for efficiency: potential for many possible retries of the same shape:
            for (size_t i = 1; i < shape.m_points.size() - 1; i++) {
                Line li(shape.m_points[i - 1], shape.m_points[i]);
                connections = lineInPolyList(li, shaperef, tolerance);
            }
        }
    }

    return connections;
}

// for any geometry, not just line to lines
void ShapeMap::makeShapeConnections() {
    if (m_hasgraph) {
        m_connectors.clear();
        m_attributes->clear();
        m_links.clear();
        m_unlinks.clear();

        // note, expects these to be numbered 0, 1...
        int conn_col = m_attributes->insertOrResetLockedColumn("Connectivity");

        int i = -1;
        for (auto shape : m_shapes) {
            i++;
            int key = shape.first;
            auto &row = m_attributes->addRow(AttributeKey(key));
            // all indices should match...
            m_connectors.push_back(Connector());
            m_connectors[i].m_connections =
                getShapeConnections(key, TOLERANCE_B * __max(m_region.height(), m_region.width()));
            row.setValue(conn_col, float(m_connectors[i].m_connections.size()));
        }

        m_displayed_attribute = -1; // <- override if it's already showing
        setDisplayedAttribute(conn_col);
    }
}

/////////////////////////////////////////////////////////////////////////////////

// note: uses rowid not key
double ShapeMap::getLocationValue(const Point2f &point) const {
    double val = -1.0;
    int x = pointInPoly(point);
    if (x == -1) {
        // try looking for a polyline instead
        x = getClosestOpenGeom(point);
    }
    if (x != -1) {
        int key = getShapeRefFromIndex(x)->first;
        if (m_displayed_attribute == -1) {
            val = static_cast<float>(key);
        } else {
            auto &row = m_attributes->getRow(AttributeKey(key));
            val = row.getValue(m_displayed_attribute);
        }
    }
    return (x == -1) ? -2.0 : val; // -2.0 is returned when point cannot be associated with a poly
}

bool ShapeMap::setCurSel(QtRegion &r, bool add) {
    if (add == false) {
        clearSel();
    }

    if (r.bottom_left == r.top_right) {
        // note: uses index not key
        int index = pointInPoly(r.bottom_left);
        if (index == -1) {
            // try looking for a polyline instead
            index = getClosestOpenGeom(r.bottom_left);
        }
        if (index != -1) {
            auto shapeIter = getShapeRefFromIndex(index);
            if (m_selection_set.insert(shapeIter->first).second) {
                auto &row = m_attributes->getRow(AttributeKey(shapeIter->first));
                row.setSelection(true);
            }
            shapeIter->second.m_selected = true;
            m_selection = true;
        }
    } else {
        PixelRef bl = pixelate(r.bottom_left);
        PixelRef tr = pixelate(r.top_right);
        for (int i = bl.x; i <= tr.x; i++) {
            for (int j = bl.y; j <= tr.y; j++) {
                const std::vector<ShapeRef> &shapeRefs =
                    m_pixel_shapes(static_cast<size_t>(j), static_cast<size_t>(i));
                for (const ShapeRef &shape : shapeRefs) {
                    // relies on indices of shapes and attributes being aligned
                    auto &shapeIter = m_shapes.at(shape.m_shape_ref);
                    if (intersect_region(r, shapeIter.m_region)) {
                        shapeIter.m_selected = true;
                        m_selection = true;
                    }
                }
            }
        }
        // actually probably often faster to set flag and later record list:
        for (auto shape : m_shapes) {
            if (shape.second.m_selected) {
                if (m_selection_set.insert(shape.first).second) {
                    auto &row = m_attributes->getRow(AttributeKey(shape.first));
                    row.setSelection(true);
                }
            }
        }
    }

    return m_selection;
}

// this version is used by setSelSet in MetaGraph, ultimately called from CTableView and PlotView
bool ShapeMap::setCurSel(const std::vector<int> &selset, bool add) {
    // note: override cursel, can only be used with analysed pointdata:
    if (!add) {
        clearSel();
    }
    for (int shapeRef : selset) {
        if (m_selection_set.insert(shapeRef).second) {
            auto &row = m_attributes->getRow(AttributeKey(shapeRef));
            row.setSelection(true);
        }
        m_shapes.at(shapeRef).m_selected = true;
        m_selection = true;
    }
    return m_selection;
}

// this version is used when setting a selection set via the scripting language
bool ShapeMap::setCurSelDirect(const std::vector<int> &selset, bool add) {
    // note: override cursel, can only be used with analysed pointdata:
    if (!add) {
        clearSel();
    }
    for (int shapeRef : selset) {
        if (m_selection_set.insert(shapeRef).second) {
            auto &row = m_attributes->getRow(AttributeKey(shapeRef));
            row.setSelection(true);
        }
        m_shapes.at(shapeRef).m_selected = true;
        m_selection = true;
    }
    return m_selection;
}

float ShapeMap::getDisplayedSelectedAvg() { return (m_attributes->getSelAvg(m_displayed_attribute)); }

bool ShapeMap::clearSel() {
    // note, only clear if need be, as m_attributes->deselectAll is slow
    if (m_selection_set.size()) {
        m_attributes->deselectAllRows();
        m_selection = false;
        for (auto &shapeRef : m_selection_set) {
            m_shapes.at(shapeRef).m_selected = false;
        }
        m_selection_set.clear();
    }
    return true;
}

QtRegion ShapeMap::getSelBounds() {
    QtRegion r;
    if (m_selection_set.size()) {
        for (auto &shapeRef : m_selection_set) {
            r = runion(r, m_shapes.at(shapeRef).getBoundingBox());
        }
    }
    return r;
}

bool ShapeMap::selectionToLayer(const std::string &name) {
    bool retvar = false;
    if (m_selection_set.size()) {
        dXreimpl::pushSelectionToLayer(*m_attributes, m_layers, name);
        retvar = m_layers.isLayerVisible(m_layers.getLayerIndex(name));
        if (retvar) {
            clearSel();
        }
    }
    return retvar;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::read(std::istream &stream) {
    // turn off selection / editable etc
    m_selection = false;
    m_editable = false;
    m_show = true; // <- by default show
    m_map_type = ShapeMap::EMPTYMAP;

    // clear old BSP tree (if exists)
    m_bsp_tree = false;
    m_bsp_root = NULL;

    // clear old:
    m_display_shapes.clear();
    m_shapes.clear();
    m_attributes->clear();
    m_connectors.clear();
    m_links.clear();
    m_unlinks.clear();
    m_undobuffer.clear();

    // name
    m_name = dXstring::readString(stream);

    stream.read((char *)&m_map_type, sizeof(m_map_type));

    stream.read((char *)&m_show, sizeof(m_show));
    stream.read((char *)&m_editable, sizeof(m_editable));

    // PixelBase read
    // read extents:
    stream.read((char *)&m_region, sizeof(m_region));
    // read rows / cols
    int rows, cols;
    stream.read(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.read(reinterpret_cast<char *>(&cols), sizeof(cols));
    m_rows = static_cast<size_t>(rows);
    m_cols = static_cast<size_t>(cols);
    // calculate geom data:
    m_tolerance = __max(m_region.width(), m_region.height()) * TOLERANCE_A;

    // read next object ref to be used:
    stream.read((char *)&m_obj_ref, sizeof(m_obj_ref));
    int depr_int;
    stream.read((char *)&depr_int, sizeof(depr_int));

    // read shape data
    int count = 0;
    stream.read((char *)&count, sizeof(count));
    for (int j = 0; j < count; j++) {
        int key;
        stream.read((char *)&key, sizeof(key));
        auto iter = m_shapes.insert(std::make_pair(key, SalaShape())).first;
        iter->second.read(stream);
    }

    // read object data (currently unused)
    // PK: As the above comment (and others regarding the m_objects
    // functionality) suggest, these are no longer used so they can
    // just be skipped if ever found
    stream.read((char *)&count, sizeof(count));
    for (int k = 0; k < count; k++) {
        int key;
        stream.read((char *)&key, sizeof(key));
        unsigned int size;
        stream.read((char *)&size, sizeof(size));
        stream.ignore(sizeof(int) * std::streamsize(size));
    }
    // read attribute data
    m_attributes->read(stream, m_layers);
    stream.read((char *)&m_displayed_attribute, sizeof(m_displayed_attribute));

    if (int(m_displayed_attribute) >= 0) {
        std::vector<size_t> indices(m_attributes->getNumColumns());
        std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

        std::sort(indices.begin(), indices.end(),
                  [&](size_t a, size_t b) { return m_attributes->getColumnName(a) < m_attributes->getColumnName(b); });
        m_displayed_attribute = indices[m_displayed_attribute];
    }

    // prepare pixel map:
    m_pixel_shapes = depthmapX::ColumnMatrix<std::vector<ShapeRef>>(m_rows, m_cols);
    // Now add the pixel shapes pixel map:
    // pixelate all polys in the pixel structure:
    for (auto shape : m_shapes) {
        makePolyPixels(shape.first);
    }

    // shape connections:
    count = 0;
    stream.read((char *)&count, sizeof(count));
    for (int i = 0; i < count; i++) {
        m_connectors.push_back(Connector());
        m_connectors[size_t(i)].read(stream);
    }
    dXreadwrite::readIntoVector(stream, m_links);
    dXreadwrite::readIntoVector(stream, m_unlinks);

    // some miscellaneous extra data for mapinfo files
    m_hasMapInfoData = false;
    char x = stream.get();
    if (x == 'm') {
        m_mapinfodata = MapInfoData();
        m_mapinfodata.read(stream);
        m_hasMapInfoData = true;
    }

    invalidateDisplayedAttribute();
    setDisplayedAttribute(m_displayed_attribute);

    return true;
}

bool ShapeMap::write(std::ofstream &stream) {
    // name
    dXstring::writeString(stream, m_name);

    stream.write((char *)&m_map_type, sizeof(m_map_type));
    stream.write((char *)&m_show, sizeof(m_show));
    stream.write((char *)&m_editable, sizeof(m_editable));

    // PixelBase write
    // write extents:
    stream.write((char *)&m_region, sizeof(m_region));
    // write rows / cols
    int rows = static_cast<int>(m_rows);
    int cols = static_cast<int>(m_cols);
    stream.write(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.write(reinterpret_cast<char *>(&cols), sizeof(cols));

    // write next object ref to be used:
    stream.write((char *)&m_obj_ref, sizeof(m_obj_ref));

    // left here for backwards-compatibility
    // TODO: Remove at next iteration of the .graph file format
    int largest_shape_ref = m_shapes.empty() ? -1 : m_shapes.rbegin()->first;
    stream.write((char *)&largest_shape_ref, sizeof(largest_shape_ref));

    // write shape data
    int count = m_shapes.size();
    stream.write((char *)&count, sizeof(count));
    for (auto shape : m_shapes) {
        int key = shape.first;
        stream.write((char *)&key, sizeof(key));
        shape.second.write(stream);
    }
    // write object data (currently unused)
    count = 0;
    stream.write((char *)&count, sizeof(count));

    // write attribute data
    m_attributes->write(stream, m_layers);

    // TODO: Compatibility. The attribute columns will be stored sorted alphabetically
    // so the displayed attribute needs to match that
    int sortedDisplayedAttribute = m_attributes->getColumnSortedIndex(m_displayed_attribute);
    stream.write((char *)&sortedDisplayedAttribute, sizeof(sortedDisplayedAttribute));

    // write connections data
    count = m_connectors.size();
    stream.write((char *)&count, sizeof(count));

    for (int i = 0; i < count; i++) {
        m_connectors[i].write(stream);
    }
    dXreadwrite::writeVector(stream, m_links);
    dXreadwrite::writeVector(stream, m_unlinks);

    // some miscellaneous extra data for mapinfo files
    if (m_hasMapInfoData) {
        stream.put('m');
        m_mapinfodata.write(stream);
    } else {
        stream.put('x');
    }

    return true;
}

bool ShapeMap::output(std::ofstream &stream, char delimiter) {
    stream << "Ref";
    if ((m_map_type & LINEMAP) == 0) {
        stream << delimiter << "cx" << delimiter << "cy";
    } else {
        stream << delimiter << "x1" << delimiter << "y1" << delimiter << "x2" << delimiter << "y2";
    }

    // TODO: For compatibility write the columns in alphabetical order
    // but the physical columns in the order inserted

    std::vector<size_t> indices(m_attributes->getNumColumns());
    std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

    std::sort(indices.begin(), indices.end(),
              [&](size_t a, size_t b) { return m_attributes->getColumnName(a) < m_attributes->getColumnName(b); });
    for (int idx : indices) {
        stream << delimiter << m_attributes->getColumnName(idx);
    }

    stream << std::endl;

    for (auto iter = m_attributes->begin(); iter != m_attributes->end(); iter++) {
        int key = iter->getKey().value;
        if (isObjectVisible(m_layers, iter->getRow())) {
            stream << key;
            auto shape = m_shapes[key];
            if ((m_map_type & LINEMAP) == 0) {
                stream << delimiter << shape.m_centroid.x << delimiter << shape.m_centroid.y;
            } else {
                stream.precision(12); // TODO: Here for compatibility with old version
                const Line &li = shape.getLine();
                stream << delimiter << li.start().x << delimiter << li.start().y << delimiter << li.end().x
                       << delimiter << li.end().y;
            }
            stream.precision(8); // TODO: Here for compatibility with old version
            for (int idx : indices) {
                stream << delimiter << iter->getRow().getValue(idx);
            }
            stream << std::endl;
        }
    }
    return true;
}

bool ShapeMap::importPoints(const std::vector<Point2f> &points, const depthmapX::Table &data) {
    // assumes that points and data come in the same order

    std::vector<int> shape_refs;

    for (auto &point : points) {
        shape_refs.push_back(makePointShape(point));
    }

    bool dataImported = importData(data, shape_refs);

    invalidateDisplayedAttribute();
    setDisplayedAttribute(-1);

    return dataImported;
}

bool ShapeMap::importPointsWithRefs(const std::map<int, Point2f> &points, const depthmapX::Table &data) {
    // assumes that points and data come in the same order

    std::vector<int> shape_refs;

    for (auto &point : points) {
        shape_refs.push_back(makePointShapeWithRef(point.second, point.first));
    }

    bool dataImported = importData(data, shape_refs);

    invalidateDisplayedAttribute();
    setDisplayedAttribute(-1);

    return dataImported;
}

bool ShapeMap::importLines(const std::vector<Line> &lines, const depthmapX::Table &data) {
    // assumes that lines and data come in the same order

    std::vector<int> shape_refs;

    for (auto &line : lines) {
        shape_refs.push_back(makeLineShape(line));
    }

    bool dataImported = importData(data, shape_refs);

    invalidateDisplayedAttribute();
    setDisplayedAttribute(-1);

    return dataImported;
}

bool ShapeMap::importLinesWithRefs(const std::map<int, Line> &lines, const depthmapX::Table &data) {
    // assumes that lines and data come in the same order

    std::vector<int> shape_refs;

    for (auto &line : lines) {
        shape_refs.push_back(makeLineShapeWithRef(line.second, line.first));
    }

    bool dataImported = importData(data, shape_refs);

    invalidateDisplayedAttribute();
    setDisplayedAttribute(-1);

    return dataImported;
}

bool ShapeMap::importPolylines(const std::vector<depthmapX::Polyline> &polylines, const depthmapX::Table &data) {
    // assumes that lines and data come in the same order

    std::vector<int> shape_refs;

    for (auto &polyline : polylines) {
        shape_refs.push_back(makePolyShape(polyline.m_vertices, !polyline.m_closed));
    }

    bool dataImported = importData(data, shape_refs);

    invalidateDisplayedAttribute();
    setDisplayedAttribute(-1);

    return dataImported;
}

bool ShapeMap::importPolylinesWithRefs(const std::map<int, depthmapX::Polyline> &polylines,
                                       const depthmapX::Table &data) {
    // assumes that lines and data come in the same order

    std::vector<int> shape_refs;

    for (auto &polyline : polylines) {
        shape_refs.push_back(
            makePolyShapeWithRef(polyline.second.m_vertices, !polyline.second.m_closed, polyline.first));
    }

    bool dataImported = importData(data, shape_refs);

    invalidateDisplayedAttribute();
    setDisplayedAttribute(-1);

    return dataImported;
}

bool ShapeMap::importData(const depthmapX::Table &data, std::vector<int> shape_refs) {
    for (auto &column : data) {
        std::string colName = column.first;
        std::replace(colName.begin(), colName.end(), '_', ' ');
        dXstring::makeInitCaps(colName);

        if (colName.empty())
            continue;

        int colIndex = m_attributes->insertOrResetColumn(colName);

        if (colIndex == -1) {
            // error adding column (e.g., duplicate column names)
            continue;
        }

        std::unordered_map<std::string, size_t> colcodes;

        for (size_t i = 0; i < column.second.size(); i++) {
            std::string cellValue = column.second[i];
            double value = 0;
            if (dXstring::isDouble(cellValue)) {
                value = stod(cellValue);
            } else {
                std::unordered_map<std::string, size_t>::iterator cellAt = colcodes.find(cellValue);
                if (cellAt == colcodes.end()) {

                    // TODO:
                    // It seems that the original intention here was that if we are past 32 unique
                    // values, we should stop trying to make the column categorical and fill the rest
                    // of the values with -1.0f. It's not possible to test the original implementation
                    // because the app crashes if we load a file with more than 32 unique values. When
                    // and if we have a robust implementation of an attribute table that allows for
                    // both categorical and plain string attributes this should be re-examined for a
                    // better way to classify the column as either. Meanwhile after this threshold (32)
                    // we set the whole column to -1 so that it does not give the impression it worked
                    // when it's actually half-baked

                    if (colcodes.size() >= 32) {
                        for (size_t j = 0; j < column.second.size(); j++) {
                            m_attributes->getRow(AttributeKey(shape_refs[j])).setValue(colIndex, -1.0f);
                        }
                        continue;
                    } else {
                        value = colcodes.size();
                        colcodes.insert(std::make_pair(cellValue, colcodes.size()));
                    }
                } else {
                    value = cellAt->second;
                }
            }
            m_attributes->getRow(AttributeKey(shape_refs[i])).setValue(colIndex, value);
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////////

void ShapeMap::setDisplayedAttribute(int col) {
    if (!m_invalidate && m_displayed_attribute == col) {
        return;
    }
    m_displayed_attribute = col;
    m_invalidate = true;

    // always override at this stage:
    m_attribHandle->setDisplayColIndex(m_displayed_attribute);

    m_invalidate = false;
}

//////////////////////////////////////////////////////////////////////////////////

// this is all very similar to spacepixel, apart from a few minor details

void ShapeMap::makeViewportShapes(const QtRegion &viewport) const {
    if (m_invalidate) {
        return;
    }

    if (m_display_shapes.empty() || m_newshape) {
        m_display_shapes.assign(m_shapes.size(), -1);
        m_newshape = false;
    }

    m_current = -1; // note: findNext expects first to be labelled -1

    PixelRef bl = pixelate(viewport.bottom_left);
    PixelRef tr = pixelate(viewport.top_right);

    for (int i = bl.x; i <= tr.x; i++) {
        for (int j = bl.y; j <= tr.y; j++) {
            const std::vector<ShapeRef> &shapeRefs = m_pixel_shapes(static_cast<size_t>(j), static_cast<size_t>(i));
            for (const ShapeRef &shape : shapeRefs) {
                // copy the index to the correct draworder position (draworder is formatted on display attribute)
                int x = std::distance(m_shapes.begin(), m_shapes.find(shape.m_shape_ref));
                AttributeKey shapeRefKey(shape.m_shape_ref);
                if (isObjectVisible(m_layers, m_attributes->getRow(shapeRefKey))) {
                    m_display_shapes[m_attribHandle->findInIndex(shapeRefKey)] = x;
                }
            }
        }
    }

    m_curlinkline = -1;
    m_curunlinkpoint = -1;
}

bool ShapeMap::findNextShape(bool &nextlayer) const {
    // note: will not work immediately after a new poly has been added: makeViewportShapes first
    if (m_invalidate || m_newshape) {
        return false;
    }

    while (++m_current < (int)m_shapes.size() && m_display_shapes[m_current] == -1)
        ;

    if (m_current < (int)m_shapes.size()) {
        return true;
    } else {
        m_current = (int)m_shapes.size();
        nextlayer = true;
        return false;
    }
}

const SalaShape &ShapeMap::getNextShape() const {
    int x = m_display_shapes[m_current]; // x has display order in it
    m_display_shapes[m_current] = -1;    // you've drawn it
    return depthmapX::getMapAtIndex(m_shapes, x)->second;
}

///////////////////////////////////////////////////////////////////////////////////

std::vector<SalaEdgeU> SalaShape::getClippingSet(QtRegion &clipframe) const {
    std::vector<SalaEdgeU> edgeset;
    bool last_inside = (clipframe.contains_touch(m_points[0])) ? true : false;
    bool found_inside = last_inside;
    for (size_t i = 1; i < m_points.size(); i++) {
        bool next_inside = (clipframe.contains_touch(m_points[i])) ? true : false;
        found_inside |= next_inside;
        if (last_inside != next_inside) {
            if (last_inside) {
                EdgeU eu = clipframe.getCutEdgeU(m_points[i - 1], m_points[i]);
                edgeset.push_back(SalaEdgeU(i, false, eu));
            } else {
                EdgeU eu = clipframe.getCutEdgeU(m_points[i], m_points[i - 1]);
                edgeset.push_back(SalaEdgeU(i - 1, true, eu));
            }
        }
        last_inside = next_inside;
    }
    if (!found_inside) {
        // note: deliberately add a single empty SalaEdgeU if this polygon is never inside the frame
        edgeset.push_back(SalaEdgeU());
    }
    return edgeset;
}

///////////////////////////////////////////////////////////////////////////////////

// copied from SpacePixel

PixelRef ShapeMap::pixelate(const Point2f &p, bool constrain, int) const {
    PixelRef r;

    Point2f p1 = p;
    p1.normalScale(m_region);

    if (constrain) {
        if (p1.x <= 0.0) {
            r.x = 0;
        } else if (p1.x >= 1.0) {
            r.x = m_cols - 1;
        } else {
            r.x = short(floor(p1.x * m_cols));
        }
    } else {
        r.x = short(floor(p1.x * m_cols));
    }

    if (constrain) {
        if (p1.y <= 0.0) {
            r.y = 0;
        } else if (p1.y >= 1.0) {
            r.y = m_rows - 1;
        } else {
            r.y = short(floor(p1.y * m_rows));
        }
    } else {
        r.y = short(floor(p1.y * m_rows));
    }

    return r;
}

void ShapeMap::copyMapInfoBaseData(const ShapeMap &sourceMap) {
    m_mapinfodata = MapInfoData();
    m_mapinfodata.m_coordsys = sourceMap.getMapInfoData().m_coordsys;
    m_mapinfodata.m_bounds = sourceMap.getMapInfoData().m_bounds;
    m_hasMapInfoData = true;
}

///////////////////////////////////////////////////////////////////////////////////

int ShapeMap::loadMifMap(std::istream &miffile, std::istream &midfile) {
    m_mapinfodata = MapInfoData();
    int retvar = m_mapinfodata.import(miffile, midfile, *this);
    if (retvar == MINFO_OK)
        m_hasMapInfoData = true;
    return retvar;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::outputMifMap(std::ostream &miffile, std::ostream &midfile) {
    if (!m_hasMapInfoData) {
        MapInfoData mapinfodata;
        mapinfodata.exportFile(miffile, midfile, *this);
    } else {
        m_mapinfodata.exportFile(miffile, midfile, *this);
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////

// Code for explicit linking / unlinking

bool ShapeMap::linkShapes(const Point2f &p) {
    if (m_selection_set.size() != 1) {
        return false;
    }
    int index1 = std::distance(m_shapes.begin(), m_shapes.find(*m_selection_set.begin()));
    // note: uses rowid not key
    int index2 = pointInPoly(p);
    if (index2 == -1) {
        // try looking for a polyline instead
        index2 = getClosestOpenGeom(p);
    }
    if (index2 == -1) {
        return false;
    }
    clearSel();

    linkShapes(index1, index2);

    return true;
}

bool ShapeMap::linkShapesFromRefs(int ref1, int ref2, bool refresh) {
    int index1 = depthmapX::findIndexFromKey(m_shapes, ref1);
    int index2 = depthmapX::findIndexFromKey(m_shapes, ref2);
    return linkShapes(index1, index2, refresh);
}

bool ShapeMap::linkShapes(int index1, int index2, bool refresh) {
    int conn_col = m_attributes->getOrInsertLockedColumn("Connectivity");
    bool update = false;

    if (index1 != index2) {
        // link these lines...
        // first look for explicit unlinks and clear
        OrderedIntPair link(index1, index2);
        auto unlinkiter = std::find(m_unlinks.begin(), m_unlinks.end(), link);
        if (unlinkiter != m_unlinks.end()) {
            m_unlinks.erase(unlinkiter);
            update = true;
        } else {
            // then check not linked already
            auto &connections1 = m_connectors[size_t(index1)].m_connections;
            auto &connections2 = m_connectors[size_t(index2)].m_connections;
            auto linkIter1 = std::find(connections1.begin(), connections1.end(), index2);
            auto linkIter2 = std::find(connections2.begin(), connections2.end(), index1);
            if (linkIter1 == connections1.end() && linkIter2 == connections2.end()) {
                // finally, link the two lines
                depthmapX::addIfNotExists(m_links, link);
                update = true;
            }
        }
    }

    if (update) {
        depthmapX::insert_sorted(m_connectors[size_t(index1)].m_connections, index2);
        depthmapX::insert_sorted(m_connectors[size_t(index2)].m_connections, index1);
        auto &row1 = getAttributeRowFromShapeIndex(index1);
        auto &row2 = getAttributeRowFromShapeIndex(index2);
        row1.incrValue(conn_col);
        row2.incrValue(conn_col);
        if (refresh && getDisplayedAttribute() == conn_col) {
            invalidateDisplayedAttribute();
            setDisplayedAttribute(conn_col); // <- reflect changes to connectivity counts
        }
    }

    return update;
}

// this version is used to link segments in segment analysis
// note it only links one way!
bool ShapeMap::linkShapes(int id1, int dir1, int id2, int dir2, float weight) {
    bool success = false;
    Connector &connector = m_connectors[size_t(id1)];
    if (dir1 == 1) {
        success = depthmapX::addIfNotExists(connector.m_forward_segconns, SegmentRef(dir2, id2), weight);
    } else {
        success = depthmapX::addIfNotExists(connector.m_back_segconns, SegmentRef(dir2, id2), weight);
    }

    // checking success != -1 avoids duplicate entries adding to connectivity
    if (success) {
        int conn_col = m_attributes->getOrInsertLockedColumn("Connectivity");
        auto &row = getAttributeRowFromShapeIndex(id1);
        row.incrValue(conn_col);
        int weight_col = m_attributes->getOrInsertLockedColumn("Weighted Connectivity");
        row.incrValue(weight_col, weight);
    }

    return true;
}

bool ShapeMap::unlinkShapes(const Point2f &p) {
    if (m_selection_set.size() != 1) {
        return false;
    }
    int index1 = std::distance(m_shapes.begin(), m_shapes.find(*m_selection_set.begin()));
    int index2 = pointInPoly(p);
    if (index2 == -1) {
        // try looking for a polyline instead
        index2 = getClosestOpenGeom(p);
    }
    if (index2 == -1) {
        return false;
    }
    clearSel();

    unlinkShapes(index1, index2);

    return true;
}

bool ShapeMap::unlinkShapesFromRefs(int ref1, int ref2, bool refresh) {
    int index1 = depthmapX::findIndexFromKey(m_shapes, ref1);
    int index2 = depthmapX::findIndexFromKey(m_shapes, ref2);
    return unlinkShapes(index1, index2, refresh);
}

// note: uses rowids rather than shape key
bool ShapeMap::unlinkShapes(int index1, int index2, bool refresh) {
    int conn_col = m_attributes->getColumnIndex("Connectivity");
    bool update = false;

    if (index1 != index2) {
        // unlink these shapes...
        // first look for explicit links and clear
        OrderedIntPair unlink(index1, index2);
        auto linkiter = std::find(m_links.begin(), m_links.end(), unlink);
        if (linkiter != m_links.end()) {
            m_links.erase(linkiter);
            update = true;
        } else {
            // then check if linked already
            auto &connections1 = m_connectors[size_t(index1)].m_connections;
            auto &connections2 = m_connectors[size_t(index2)].m_connections;
            auto linkIter1 = std::find(connections1.begin(), connections1.end(), index2);
            auto linkIter2 = std::find(connections2.begin(), connections2.end(), index1);
            if (linkIter1 != connections1.end() && linkIter2 != connections2.end()) {
                // finally, unlink the two shapes
                depthmapX::addIfNotExists(m_unlinks, unlink);
                update = true;
            }
        }
    }

    if (update && conn_col != -1) {
        depthmapX::findAndErase(m_connectors[size_t(index1)].m_connections, index2);
        depthmapX::findAndErase(m_connectors[size_t(index2)].m_connections, index1);
        auto &row1 = getAttributeRowFromShapeIndex(index1);
        auto &row2 = getAttributeRowFromShapeIndex(index2);
        row1.incrValue(conn_col, -1.0f);
        row2.incrValue(conn_col, -1.0f);
        if (refresh && getDisplayedAttribute() == conn_col) {
            invalidateDisplayedAttribute();
            setDisplayedAttribute(conn_col); // <- reflect changes to connectivity counts
        }
    }
    return update;
}

bool ShapeMap::unlinkShapesByKey(int key1, int key2, bool refresh) {
    int conn_col = m_attributes->getColumnIndex("Connectivity");
    bool update = false;

    int index1 = std::distance(m_shapes.begin(), m_shapes.find(key1));
    int index2 = std::distance(m_shapes.begin(), m_shapes.find(key2));

    if (key1 != key2) {
        // unlink these shapes...
        // first look for explicit links and clear
        OrderedIntPair unlink(index1, index2);
        auto linkiter = std::find(m_links.begin(), m_links.end(), unlink);
        if (linkiter != m_links.end()) {
            m_links.erase(linkiter);
            update = true;
        } else {
            // then check if linked already
            auto &connections1 = m_connectors[size_t(index1)].m_connections;
            auto &connections2 = m_connectors[size_t(index2)].m_connections;
            auto linkIter1 = std::find(connections1.begin(), connections1.end(), index2);
            auto linkIter2 = std::find(connections2.begin(), connections2.end(), index1);
            if (linkIter1 != connections1.end() && linkIter2 != connections2.end()) {
                // finally, unlink the two shapes
                depthmapX::addIfNotExists(m_unlinks, unlink);
                update = true;
            }
        }
    }

    if (update && conn_col != -1) {
        depthmapX::findAndErase(m_connectors[size_t(index1)].m_connections, index2);
        depthmapX::findAndErase(m_connectors[size_t(index2)].m_connections, index1);
        auto &row1 = m_attributes->getRow(AttributeKey(key1));
        auto &row2 = m_attributes->getRow(AttributeKey(key1));
        row1.incrValue(conn_col, -1.0f);
        row2.incrValue(conn_col, -1.0f);
        if (refresh && getDisplayedAttribute() == conn_col) {
            invalidateDisplayedAttribute();
            setDisplayedAttribute(conn_col); // <- reflect changes to connectivity counts
        }
    }
    return update;
}

bool ShapeMap::clearLinks() {
    for (size_t i = 0; i < m_unlinks.size(); i++) {
        OrderedIntPair link = m_unlinks[i];
        depthmapX::insert_sorted(m_connectors[size_t(link.a)].m_connections, link.b);
        depthmapX::insert_sorted(m_connectors[size_t(link.b)].m_connections, link.a);
    }
    m_unlinks.clear();

    for (size_t j = 0; j < m_links.size(); j++) {
        OrderedIntPair link = m_links[j];
        depthmapX::findAndErase(m_connectors[size_t(link.a)].m_connections, link.b);
        depthmapX::findAndErase(m_connectors[size_t(link.b)].m_connections, link.a);
    }
    m_links.clear();

    return true;
}

bool ShapeMap::unlinkShapeSet(std::istream &idset, int refcol) {
    std::string line;
    std::vector<std::pair<int, int>> unlinks;
    do {
        std::pair<int, int> unlink;
        dXstring::safeGetline(idset, line);
        if (!line.empty()) {
            auto tokens = dXstring::split(line, '\t');
            if (tokens.size() < 2) {
                return false;
            }
            try {
                unlink.first = stoi(tokens[0]);
                unlink.second = stoi(tokens[1]);
                unlinks.push_back(unlink);
            } catch (const std::invalid_argument) {
                ;
            } catch (const std::out_of_range) {
                ;
            } // don't do anything if it can't parse the numbers, just ignore (e.g., first line)
        }
    } while (!idset.eof());

    if (refcol != -1) {
        // not using the standard "Ref", find the proper key
        std::vector<AttributeIndexItem> idx =
            refcol != -1 ? makeAttributeIndex(*m_attributes, refcol) : std::vector<AttributeIndexItem>();

        AttributeKey dummykey(-1);
        AttributeRowImpl dummyrow(*m_attributes);

        for (size_t i = 0; i < unlinks.size(); i++) {
            auto iter = depthmapX::findBinary(idx, AttributeIndexItem(dummykey, unlinks[i].first, dummyrow));
            unlinks[i].first = (iter == idx.end()) ? -1 : iter->key.value;
            iter = depthmapX::findBinary(idx, AttributeIndexItem(dummykey, unlinks[i].second, dummyrow));
            unlinks[i].second = (iter == idx.end()) ? -1 : iter->key.value;
        }
    }
    for (size_t i = 0; i < unlinks.size(); i++) {
        unlinkShapesByKey(unlinks[i].first, unlinks[i].second, false);
    }

    int conn_col = m_attributes->getColumnIndex("Connectivity");
    if (getDisplayedAttribute() == conn_col) {
        invalidateDisplayedAttribute();
        setDisplayedAttribute(conn_col); // <- reflect changes to connectivity counts
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::findNextLinkLine() const {
    if (m_curlinkline < (int)m_links.size()) {
        m_curlinkline++;
    }
    return (m_curlinkline < (int)m_links.size());
}

Line ShapeMap::getNextLinkLine() const {
    // note, links are stored directly by rowid, not by key:
    if (m_curlinkline < (int)m_links.size()) {
        return Line(depthmapX::getMapAtIndex(m_shapes, m_links[m_curlinkline].a)->second.getCentroid(),
                    depthmapX::getMapAtIndex(m_shapes, m_links[m_curlinkline].b)->second.getCentroid());
    }
    return Line();
}

std::vector<SimpleLine> ShapeMap::getAllLinkLines() {
    std::vector<SimpleLine> linkLines;
    for (size_t i = 0; i < m_links.size(); i++) {
        linkLines.push_back(SimpleLine(depthmapX::getMapAtIndex(m_shapes, m_links[i].a)->second.getCentroid(),
                                       depthmapX::getMapAtIndex(m_shapes, m_links[i].b)->second.getCentroid()));
    }
    return linkLines;
}

// note: these functions would need slight work for arbitrary shape overlaps

bool ShapeMap::findNextUnlinkPoint() const {
    if (m_curunlinkpoint < (int)m_unlinks.size()) {
        m_curunlinkpoint++;
    }
    return (m_curunlinkpoint < (int)m_unlinks.size());
}

Point2f ShapeMap::getNextUnlinkPoint() const {
    // note, links are stored directly by rowid, not by key:
    if (m_curunlinkpoint < (int)m_unlinks.size()) {
        return intersection_point(depthmapX::getMapAtIndex(m_shapes, m_unlinks[m_curunlinkpoint].a)->second.getLine(),
                                  depthmapX::getMapAtIndex(m_shapes, m_unlinks[m_curunlinkpoint].b)->second.getLine(),
                                  TOLERANCE_A);
    }
    return Point2f();
}
std::vector<Point2f> ShapeMap::getAllUnlinkPoints() {
    std::vector<Point2f> unlinkPoints;
    for (size_t i = 0; i < m_unlinks.size(); i++) {
        unlinkPoints.push_back(intersection_point(depthmapX::getMapAtIndex(m_shapes, m_unlinks[i].a)->second.getLine(),
                                                  depthmapX::getMapAtIndex(m_shapes, m_unlinks[i].b)->second.getLine(),
                                                  TOLERANCE_A));
    }
    return unlinkPoints;
}

void ShapeMap::outputUnlinkPoints(std::ofstream &stream, char delim) {
    stream << "x" << delim << "y" << std::endl;

    stream.precision(12);
    for (size_t i = 0; i < m_unlinks.size(); i++) {
        // note, links are stored directly by rowid, not by key:
        Point2f p =
            intersection_point(depthmapX::getMapAtIndex(m_shapes, m_unlinks[i].a)->second.getLine(),
                               depthmapX::getMapAtIndex(m_shapes, m_unlinks[i].b)->second.getLine(), TOLERANCE_A);
        stream << p.x << delim << p.y << std::endl;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::makeBSPtree() const {
    if (m_bsp_tree) {
        return true;
    }

    std::vector<TaggedLine> partitionlines;
    for (auto shape : m_shapes) {
        if (shape.second.isLine()) {
            partitionlines.push_back(TaggedLine(shape.second.getLine(), shape.first));
        }
    }

    if (partitionlines.size()) {
        //
        // Now we'll try the BSP tree:
        //
        if (m_bsp_root) {
            delete m_bsp_root;
            m_bsp_root = NULL;
        }
        m_bsp_root = new BSPNode();

        BSPTree::make(NULL, 0, partitionlines, m_bsp_root);
        m_bsp_tree = true;
    }

    partitionlines.clear();

    return m_bsp_tree;
}

/////////////////////////////////////////////////////////////////////////////////

// SPECIALS BELOW

////////////////////////////////////////////////////////////////////////////////////////////////

int findwinner(double *bins, int bincount, int &difficult, int &impossible) {
    difficult = 0;
    impossible = 0;
    //
    double total = 0.0;
    //
    double maxvalue = -1.0;
    int maxbin = -1;
    int i;
    for (i = 0; i < bincount; i++) {
        if (i == 0 || bins[i] > maxvalue) {
            maxvalue = bins[i];
            maxbin = i;
        }
        total += bins[i];
    }
    if (maxvalue > total * 0.8) {
        return maxbin;
    }
    int lastwinner = maxbin;
    // no immediate clear winner, so see if across two adjacent bins:
    double savebin = bins[bincount - 1];
    double savebins0 = bins[0];
    for (i = 0; i < bincount - 1; i++) {
        double lastbin = savebin;
        savebin = bins[i];
        bins[i] += bins[i + 1] + lastbin;
    }
    bins[bincount - 1] += savebins0 + savebin;

    // now check again for a clear winner:
    maxvalue = -1.0;
    maxbin = -1;
    for (i = 0; i < bincount; i++) {
        if (i == 0 || bins[i] > maxvalue) {
            maxvalue = bins[i];
            maxbin = i;
        }
    }
    // if it's a tie, the last winner wins it:
    if (maxbin != lastwinner && maxvalue == bins[lastwinner]) {
        maxbin = lastwinner;
    }
    //
    if (maxvalue > total * 0.8) {
        return maxbin;
    }
    //
    // now it's at least hard:
    if (maxvalue > total * 0.6) {
        difficult = 1;
        return maxbin;
    }
    //
    // if not even this is true, it's really a guess in the dark:
    impossible = 1;
    return maxbin;
}

// Quick mod - TV
#if defined(_MSC_VER)
#include <windows.h>
#endif

std::vector<SimpleLine> ShapeMap::getAllShapesAsLines() const {
    std::vector<SimpleLine> lines;
    const std::map<int, SalaShape> &allShapes = getAllShapes();
    for (auto refShape : allShapes) {
        SalaShape &shape = refShape.second;
        if (shape.isLine()) {
            lines.push_back(SimpleLine(shape.getLine()));
        } else if (shape.isPolyLine() || shape.isPolygon()) {
            for (size_t n = 0; n < shape.m_points.size() - 1; n++) {
                lines.push_back(SimpleLine(shape.m_points[n], shape.m_points[n + 1]));
            }
            if (shape.isPolygon()) {
                lines.push_back(SimpleLine(shape.m_points.back(), shape.m_points.front()));
            }
        }
    }
    return lines;
}

std::vector<std::pair<SimpleLine, PafColor>> ShapeMap::getAllLinesWithColour() {
    std::vector<std::pair<SimpleLine, PafColor>> colouredLines;
    std::map<int, SalaShape> &allShapes = getAllShapes();
    int k = -1;
    for (auto &refShape : allShapes) {
        k++;
        SalaShape &shape = refShape.second;
        PafColor colour(dXreimpl::getDisplayColor(AttributeKey(refShape.first),
                                                  m_attributes->getRow(AttributeKey(refShape.first)),
                                                  *m_attribHandle.get(), true));
        if (shape.isLine()) {
            colouredLines.push_back(std::pair<SimpleLine, PafColor>(SimpleLine(shape.getLine()), colour));
        } else if (shape.isPolyLine()) {
            for (size_t n = 0; n < shape.m_points.size() - 1; n++) {
                colouredLines.push_back(
                    std::pair<SimpleLine, PafColor>(SimpleLine(shape.m_points[n], shape.m_points[n + 1]), colour));
            }
        }
    }
    return colouredLines;
}

std::map<std::vector<Point2f>, PafColor> ShapeMap::getAllPolygonsWithColour() {
    std::map<std::vector<Point2f>, PafColor> colouredPolygons;
    std::map<int, SalaShape> &allShapes = getAllShapes();
    for (auto &refShape : allShapes) {
        SalaShape &shape = refShape.second;
        if (shape.isPolygon()) {
            std::vector<Point2f> vertices;
            for (size_t n = 0; n < shape.m_points.size(); n++) {
                vertices.push_back(shape.m_points[n]);
            }
            vertices.push_back(shape.m_points.back());
            PafColor colour(dXreimpl::getDisplayColor(AttributeKey(refShape.first),
                                                      m_attributes->getRow(AttributeKey(refShape.first)),
                                                      *m_attribHandle.get(), true));
            colouredPolygons.insert(std::make_pair(vertices, colour));
        }
    }
    return colouredPolygons;
}

std::vector<std::pair<Point2f, PafColor>> ShapeMap::getAllPointsWithColour() {
    std::vector<std::pair<Point2f, PafColor>> colouredPoints;
    std::map<int, SalaShape> &allShapes = getAllShapes();
    for (auto &refShape : allShapes) {
        SalaShape &shape = refShape.second;
        if (shape.isPoint()) {
            PafColor colour(dXreimpl::getDisplayColor(AttributeKey(refShape.first),
                                                      m_attributes->getRow(AttributeKey(refShape.first)),
                                                      *m_attribHandle.get(), true));
            colouredPoints.push_back(std::make_pair(shape.getCentroid(), colour));
        }
    }
    return colouredPoints;
}
