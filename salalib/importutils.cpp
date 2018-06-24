// Copyright (C) 2017 Petros Koutsolampros

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

#include "salalib/importutils.h"
#include "genlib/stringutils.h"
#include <sstream>

namespace depthmapX {

    const int DXFCIRCLERES = 36;

    bool importFile(MetaGraph &mgraph, std::istream &stream, Communicator *communicator, std::string name, ImportType mapType, ImportFileType fileType) {

        // This function is still too fiddly but at least it shows the common interface for
        // drawing and data maps and how different file types may be parsed to be imported
        // into them

        int state = MetaGraph::NONE;
        int viewClass = MetaGraph::NONE;

        switch(mapType) {
            case DRAWINGMAP: {
                state = MetaGraph::LINEDATA;
                viewClass = MetaGraph::SHOWSHAPETOP;
                break;
            }
            case DATAMAP: {
                state = MetaGraph::DATAMAPS;
                viewClass = MetaGraph::SHOWSHAPETOP;
                break;
            }
        }

        int oldstate = mgraph.getState();
        mgraph.setState(oldstate & ~state);

        // Currently the drawing shapemaps are understood as two-level trees (file -> layers)
        // while the data shapemaps are flat. Therefore, for the moment, when we load dxfs as
        // drawing shapemaps then we let the filename be the parent and the layers the children.
        // For text files (csv, tsv) we create an artificial parent with the relevant name
        // Drawing shapemaps also carry region data that needs to be initialised and their parents
        // updated when they are created.
        // Ideally datamaps and drawingmaps should be more similar.

        if(mapType == DRAWINGMAP) {
            mgraph.m_spacePixels.emplace_back(name);
        }

        bool parsed = false;

        switch (fileType) {
            case CSV: {
                ShapeMap &shapeMap = mgraph.createNewShapeMap(mapType, name);
                parsed = importTxt(shapeMap, stream, ',');

                if(!parsed) {
                    mgraph.deleteShapeMap(mapType, shapeMap);
                    break;
                }
                if(mapType == DRAWINGMAP) {
                    mgraph.updateParentRegions(shapeMap);
                }
                break;
            }
            case TSV: {
                ShapeMap &shapeMap = mgraph.createNewShapeMap(mapType, name);
                parsed = importTxt(shapeMap, stream, '\t');

                if(!parsed) {
                    mgraph.deleteShapeMap(mapType, shapeMap);
                    break;
                }
                if(mapType == DRAWINGMAP) {
                    mgraph.updateParentRegions(shapeMap);
                }
                break;
            }
            case DXF: {

                DxfParser dp;

                if (communicator) {
                    dp = DxfParser( communicator );

                    try {
                        *communicator >> dp;
                    }
                    catch (Communicator::CancelledException) {
                        return 0;
                    }
                    catch (pexception) {
                        return -1;
                    }

                    if (communicator->IsCancelled()) {
                        return 0;
                    }
                }
                else {
                    dp.open(stream);
                }

                for (auto& layer: dp.getLayers())
                {

                    const DxfLayer& dxfLayer = layer.second;

                    if (dxfLayer.empty()) {
                        continue;
                    }

                    ShapeMap &shapeMap = mgraph.createNewShapeMap(mapType, layer.first);
                    parsed = importDxfLayer(dxfLayer, shapeMap);

                    if(!parsed) {
                        mgraph.deleteShapeMap(mapType, shapeMap);
                        break;
                    }
                    if(mapType == DRAWINGMAP) {
                        mgraph.updateParentRegions(shapeMap);
                    }
                }
                break;
            }
        }

        if(parsed) {
            mgraph.setState(mgraph.getState() | state);
            mgraph.setViewClass(viewClass);
            return true;
        } else {
            mgraph.setState(oldstate);
            return false;
        }
    }

    bool importTxt(ShapeMap &shapeMap, std::istream &stream, char delimiter = '\t') {
        Table table = csvToTable(stream, delimiter);
        std::vector<std::string> columns;
        int xcol = -1, ycol = -1, x1col = -1, y1col = -1, x2col = -1, y2col = -1, refcol = -1;
        for(auto const& column: table) {
            if (column.first == "x" || column.first == "easting")
                xcol = columns.size();
            else if (column.first == "y" || column.first == "northing")
                ycol = columns.size();
            else if (column.first == "x1")
                x1col = columns.size();
            else if (column.first == "x2")
                x2col = columns.size();
            else if (column.first == "y1")
                y1col = columns.size();
            else if (column.first == "y2")
                y2col = columns.size();
            else if (column.first == "Ref")
                refcol = columns.size();
            columns.push_back(column.first);
        }

        if (xcol != -1 && ycol != -1 && refcol != -1) {
            std::map<int, Point2f> points = extractPointsWithRefs(table[columns[xcol]], table[columns[ycol]], table[columns[refcol]]);
            table.erase(table.find(columns[xcol]));
            table.erase(table.find(columns[ycol]));
            table.erase(table.find(columns[refcol]));

            QtRegion region;

            for(auto& point: points) {
                if(region.atZero()) {
                    region = point.second;
                } else {
                    region = runion(region, point.second);
                }
            }

            shapeMap.init(points.size(),region);
            shapeMap.importPointsWithRefs(points, table);

        } else if (xcol != -1 && ycol != -1) {
            std::vector<Point2f> points = extractPoints(table[columns[xcol]], table[columns[ycol]]);
            table.erase(table.find(columns[xcol]));
            table.erase(table.find(columns[ycol]));

            QtRegion region;

            for(auto& point: points) {
                if(region.atZero()) {
                    region = point;
                } else {
                    region = runion(region, point);
                }
            }

            shapeMap.init(points.size(),region);
            shapeMap.importPoints(points, table);

        } else if (x1col != -1 && y1col != -1 && x2col != -1 && y2col != -1 && refcol != -1) {
            std::map<int, Line> lines = extractLinesWithRef(table[columns[x1col]],
                    table[columns[y1col]],
                    table[columns[x2col]],
                    table[columns[y2col]],
                    table[columns[refcol]]);
            table.erase(table.find(columns[x1col]));
            table.erase(table.find(columns[y1col]));
            table.erase(table.find(columns[x2col]));
            table.erase(table.find(columns[y2col]));
            table.erase(table.find(columns[refcol]));

            QtRegion region;

            for(auto& line: lines) {
                if(region.atZero()) {
                    region = line.second;
                } else {
                    region = runion(region, line.second);
                }
            }

            shapeMap.init(lines.size(),region);
            shapeMap.importLinesWithRefs(lines, table);
        } else if (x1col != -1 && y1col != -1 && x2col != -1 && y2col != -1) {
            std::vector<Line> lines = extractLines(table[columns[x1col]], table[columns[y1col]], table[columns[x2col]], table[columns[y2col]]);
            table.erase(table.find(columns[x1col]));
            table.erase(table.find(columns[y1col]));
            table.erase(table.find(columns[x2col]));
            table.erase(table.find(columns[y2col]));

            QtRegion region;

            for(auto& line: lines) {
                if(region.atZero()) {
                    region = line;
                } else {
                    region = runion(region, line);
                }
            }

            shapeMap.init(lines.size(),region);
            shapeMap.importLines(lines, table);
        }
        return true;
    }

    Table csvToTable(std::istream &stream, char delimiter = '\t') {

        Table table;
        std::vector<std::string> columns;

        std::string inputline;
        std::getline(stream, inputline);

        // check for a matching delimited header line...
        auto strings = dXstring::split(inputline, delimiter);
        if (strings.size() < 2) {
            // throw exception
            return table;
        }

        for (auto& columnName: strings) {
            if (!columnName.empty()) {
                dXstring::ltrim(columnName,'\"');
                dXstring::rtrim(columnName,'\"');
            }
            table.insert( std::make_pair( columnName, std::vector<std::string>() ) );
            columns.push_back( columnName );
        }

        while (!stream.eof()) {
            std::getline(stream, inputline);
            if (!inputline.empty()) {
                auto strings = dXstring::split(inputline, delimiter);
                if(strings.size() != columns.size()) {
                    std::stringstream message;
                    message << "Cells in line " << inputline << "not the same number as the columns" << flush;
                    throw RuntimeException(message.str().c_str());
                }
                if (!strings.size()) {
                    continue;
                }
                for (size_t i = 0; i < strings.size(); i++) {
                    table[columns[i]].push_back(strings[i]);
                }
            }
        }
        return table;
    }

    std::vector<Line> extractLines(ColumnData &x1col, ColumnData &y1col, ColumnData &x2col, ColumnData &y2col) {
        std::vector<Line> lines;
        for(size_t i = 0; i < x1col.size(); i++) {
            double x1 = stod(x1col[i]);
            double y1 = stod(y1col[i]);
            double x2 = stod(x2col[i]);
            double y2 = stod(y2col[i]);
            lines.push_back(Line(Point2f(x1, y1), Point2f(x2, y2)));
        }
        return lines;
    }

    std::map<int, Line> extractLinesWithRef(ColumnData &x1col, ColumnData &y1col, ColumnData &x2col, ColumnData &y2col, ColumnData &refcol) {
        std::map<int, Line> lines;
        for(size_t i = 0; i < x1col.size(); i++) {
            double x1 = stod(x1col[i]);
            double y1 = stod(y1col[i]);
            double x2 = stod(x2col[i]);
            double y2 = stod(y2col[i]);
            int ref = stoi(refcol[i]);
            lines.insert(std::make_pair(ref, Line(Point2f(x1, y1), Point2f(x2, y2))));
        }
        return lines;
    }
    std::vector<Point2f> extractPoints(ColumnData &x, ColumnData &y) {
        std::vector<Point2f> points;
        for(size_t i = 0; i < x.size(); i++) {
            points.push_back(Point2f(stod(x[i]), stod(y[i])));
        }
        return points;
    }
    std::map<int, Point2f> extractPointsWithRefs(ColumnData &x, ColumnData &y, ColumnData &ref) {
        std::map<int, Point2f> points;
        for(size_t i = 0; i < x.size(); i++) {
            points.insert(std::make_pair(stoi(ref[i]), Point2f(stod(x[i]), stod(y[i]))));
        }
        return points;
    }

    bool importDxfLayer(const DxfLayer& dxfLayer, ShapeMap &shapeMap)
    {
        std::vector<Point2f> points;
        std::vector<Line> lines;
        std::vector<Polyline> polylines;

        for (int jp = 0; jp < dxfLayer.numPoints(); jp++) {
            const DxfVertex& dxf_point = dxfLayer.getPoint( jp );
            points.push_back(Point2f(dxf_point.x, dxf_point.y));

        }

        for (int j = 0; j < dxfLayer.numLines(); j++) {
            const DxfLine& dxf_line = dxfLayer.getLine( j );
            Line line = Line( Point2f(dxf_line.getStart().x, dxf_line.getStart().y),
                              Point2f(dxf_line.getEnd().x  , dxf_line.getEnd().y) );
            lines.push_back( line );
        }

        for (int k = 0; k < dxfLayer.numPolyLines(); k++) {
            const DxfPolyLine& poly = dxfLayer.getPolyLine( k );
            std::vector<Point2f> vertices;
            for (int m = 0; m < poly.numVertices(); m++) {
                DxfVertex v = poly.getVertex(m);
                vertices.push_back(Point2f(v.x, v.y));
            }
            polylines.push_back(depthmapX::Polyline(vertices, (poly.getAttributes() & DxfPolyLine::CLOSED) == DxfPolyLine::CLOSED));
        }

        for (int l = 0; l < dxfLayer.numSplines(); l++) {
            const DxfSpline& poly = dxfLayer.getSpline( l );
            std::vector<Point2f> vertices;
            for (int m = 0; m < poly.numVertices(); m++) {
                DxfVertex v = poly.getVertex(m);
                vertices.push_back(Point2f(v.x, v.y));
            }
            polylines.push_back(depthmapX::Polyline(vertices, (poly.getAttributes() & DxfPolyLine::CLOSED) == DxfPolyLine::CLOSED));

        }

        for (int n = 0; n < dxfLayer.numArcs(); n++) {
            const DxfArc& circ = dxfLayer.getArc( n );
            std::vector<Point2f> vertices;
            int segments = circ.numSegments(DXFCIRCLERES);
            if (segments > 1) {
                for (int m = 0; m <= segments; m++) {
                    DxfVertex v = circ.getVertex(m,segments);
                    vertices.push_back(Point2f(v.x, v.y));
                }
            }
            polylines.push_back(depthmapX::Polyline(vertices, false));
        }

        for (int n = 0; n < dxfLayer.numEllipses(); n++) {
            const DxfEllipse& ellipse = dxfLayer.getEllipse( n );
            std::vector<Point2f> vertices;
            int segments = ellipse.numSegments(DXFCIRCLERES);
            if (segments > 1) {
                for (int m = 0; m <= segments; m++) {
                    DxfVertex v = ellipse.getVertex(m, segments);
                    vertices.push_back(Point2f(v.x, v.y));
                }
            }
            polylines.push_back(depthmapX::Polyline(vertices, false));
        }

        for (int nc = 0; nc < dxfLayer.numCircles(); nc++) {
            const DxfCircle& circ = dxfLayer.getCircle( nc );
            std::vector<Point2f> vertices;
            for (int m = 0; m < DXFCIRCLERES; m++) {
                DxfVertex v = circ.getVertex(m,DXFCIRCLERES);
                vertices.push_back(Point2f(v.x, v.y));
            }
            polylines.push_back(depthmapX::Polyline(vertices, true));
        }
        DxfVertex layerMin = dxfLayer.getExtMin();
        DxfVertex layerMax = dxfLayer.getExtMax();
        
        QtRegion region = QtRegion(Point2f(layerMin.x, layerMin.y),Point2f(layerMax.x, layerMax.y));

        shapeMap.init(points.size() + lines.size() + polylines.size(),region);
        // parameters could be passed in the Table here such as the layer/block/colour/linetype etc.
        shapeMap.importPoints(points, Table());
        shapeMap.importLines(lines, Table());
        shapeMap.importPolylines(polylines, Table());
        return true;
    }
}
