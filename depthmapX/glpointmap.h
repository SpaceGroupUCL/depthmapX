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

#pragma once

#include "salalib/mgraph.h"
#include "depthmapX/gllinesuniform.h"
#include "depthmapX/glrastertexture.h"
#include "depthmapX/gltrianglesuniform.h"

class GLPointMap
{
public:
    void initializeGL(bool m_core)
    {
        m_grid.initializeGL(m_core);
        m_pointMap.initializeGL(m_core);
        m_linkLines.initializeGL(m_core);
        m_linkFills.initializeGL(m_core);
    }
    void updateGL(bool m_core)
    {
        m_pointMap.updateGL(m_core);
        m_grid.updateGL(m_core);
        m_linkLines.updateGL(m_core);
        m_linkFills.updateGL(m_core);
    }
    void cleanup()
    {
        m_grid.cleanup();
        m_pointMap.cleanup();
        m_linkLines.cleanup();
        m_linkFills.cleanup();
    }
    void paintGLOverlay(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel)
    {
        if(m_showLinks) {
            glLineWidth(3);
            m_linkLines.paintGL(m_mProj, m_mView, m_mModel);
            m_linkFills.paintGL(m_mProj, m_mView, m_mModel);
            glLineWidth(1);
        }
    }
    void paintGL(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel)
    {
        m_pointMap.paintGL(m_mProj, m_mView, m_mModel);
        if(m_showGrid)
            m_grid.paintGL(m_mProj, m_mView, m_mModel);
    }
    void setGridColour(QRgb gridColour) {
        m_gridColour = gridColour;
    }
    void showLinks(bool showLinks) {
        m_showLinks = showLinks;
    }
    void showGrid(bool showGrid) {
        m_showGrid = showGrid;
    }
    void loadGLObjects(PointMap& pointMap);
    void loadGLObjectsRequiringGLContext(const PointMap& currentPointMap);
private:
    GLLinesUniform m_grid;
    GLRasterTexture m_pointMap;
    GLLinesUniform m_linkLines;
    GLTrianglesUniform m_linkFills;

    QRgb m_gridColour = (qRgb(255, 255, 255) & 0x006f6f6f) | (qRgb(0, 0, 0) & 0x00a0a0a0);

    bool m_showGrid = true;
    bool m_showLinks = false;
};
