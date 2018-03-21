// depthmapX - spatial network analysis platform
// Copyright (C) 2017, Petros Koutsolampros

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

#include "depthmapX/glpointmap.h"
#include "salalib/linkutils.h"
#include "salalib/geometrygenerators.h"

void GLPointMap::loadGLObjects(PointMap& pointMap) {
    QtRegion region = pointMap.getRegion();
    m_pointMap.loadRegionData(region.bottom_left.x, region.bottom_left.y, region.top_right.x, region.top_right.y);

    if(m_showGrid) {
        std::vector<SimpleLine> gridData;
        double spacing = pointMap.getSpacing();
        double offsetX = region.bottom_left.x;
        double offsetY = region.bottom_left.y;
        for(int x = 1; x < pointMap.getCols(); x++) {
            gridData.push_back(SimpleLine(offsetX + x*spacing, region.bottom_left.y, offsetX + x*spacing, region.top_right.y));
        }
        for(int y = 1; y < pointMap.getRows(); y++) {
            gridData.push_back(SimpleLine(region.bottom_left.x, offsetY + y*spacing, region.top_right.x, offsetY + y*spacing));
        }
        m_grid.loadLineData(gridData, m_gridColour);
    }
    if(m_showLinks) {
        const std::vector<SimpleLine> &mergedPixelLines = depthmapX::getMergedPixelsAsLines(pointMap);
        std::vector<Point2f> mergedPixelLocations;
        for (auto& mergeLine: mergedPixelLines)
        {
            mergedPixelLocations.push_back(mergeLine.start());
            mergedPixelLocations.push_back(mergeLine.end());
        }

        const std::vector<Point2f> &linkFillTriangles =
                GeometryGenerators::generateMultipleDiskTriangles(32, pointMap.getSpacing()*0.25, mergedPixelLocations);
        m_linkFills.loadTriangleData(linkFillTriangles, qRgb(0,0,0));

        std::vector<SimpleLine> linkFillPerimeters =
                GeometryGenerators::generateMultipleCircleLines(32, pointMap.getSpacing()*0.25, mergedPixelLocations);
        linkFillPerimeters.insert( linkFillPerimeters.end(), mergedPixelLines.begin(), mergedPixelLines.end() );
        m_linkLines.loadLineData(linkFillPerimeters, qRgb(0,255,0));
    }
}
void GLPointMap::loadGLObjectsRequiringGLContext(const PointMap& currentPointMap) {
    QImage data(currentPointMap.getCols(),currentPointMap.getRows(), QImage::Format_RGBA8888);
    data.fill(Qt::transparent);

    for (int y = 0; y < currentPointMap.getRows(); y++) {
        for (int x = 0; x < currentPointMap.getCols(); x++) {
            PixelRef pix(x, y);
            PafColor colour = currentPointMap.getPointColor( pix );
            if (colour.alphab() != 0)
            { // alpha == 0 is transparent
                data.setPixelColor(x, y, qRgb(colour.redb(),colour.greenb(),colour.blueb()));
            }
        }
    }
    m_pointMap.loadPixelData(data);
}
