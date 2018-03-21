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
#include "depthmapX/glshapemap.h"
#include "depthmapX/gllinesuniform.h"

class GLShapeGraph
{
public:
    void initializeGL(bool m_core)
    {
        m_shapeMap.initializeGL(m_core);
        m_linkLines.initializeGL(m_core);
        m_linkFills.initializeGL(m_core);
        m_unlinkFills.initializeGL(m_core);
        m_unlinkLines.initializeGL(m_core);
    }
    void updateGL(bool m_core)
    {
        m_shapeMap.updateGL(m_core);
        m_linkLines.updateGL(m_core);
        m_linkFills.updateGL(m_core);
        m_unlinkFills.updateGL(m_core);
        m_unlinkLines.updateGL(m_core);
    }
    void cleanup()
    {
        m_shapeMap.cleanup();
        m_linkLines.cleanup();
        m_linkFills.cleanup();
        m_unlinkFills.cleanup();
        m_unlinkLines.cleanup();
    }
    void paintGLOverlay(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel)
    {
        if(m_showLinks)
        {
            glLineWidth(3);
            m_linkLines.paintGL(m_mProj, m_mView, m_mModel);
            m_linkFills.paintGL(m_mProj, m_mView, m_mModel);
            m_unlinkLines.paintGL(m_mProj, m_mView, m_mModel);
            m_unlinkFills.paintGL(m_mProj, m_mView, m_mModel);
            glLineWidth(1);
        }
    }
    void paintGL(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel)
    {
        m_shapeMap.paintGL(m_mProj, m_mView, m_mModel);
    }
    void showLinks(bool showLinks) {
        m_showLinks = showLinks;
    }
    void loadGLObjects(ShapeGraph &shapeGraph);
private:
    GLShapeMap m_shapeMap;
    GLLinesUniform m_linkLines;
    GLTrianglesUniform m_linkFills;
    GLLinesUniform m_unlinkLines;
    GLTrianglesUniform m_unlinkFills;

    bool m_showLinks = false;
};
