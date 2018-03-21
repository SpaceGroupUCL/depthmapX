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

#include "genlib/p2dpoly.h"
#include "salalib/pafcolor.h"
#include <qopengl.h>
#include <QVector>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QRgb>

class GLLines
{
    friend class testgllines;

public:
    GLLines();
    void loadLineData(const std::vector<std::pair<SimpleLine, PafColor> > &colouredLines);
    void paintGL(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel);
    void initializeGL(bool coreProfile);
    void updateGL(bool coreProfile);
    void cleanup();
    int vertexCount() const { return m_count / DATA_DIMENSIONS; }
    GLLines( const GLLines& ) = delete;
    GLLines& operator=(const GLLines& ) = delete;

private:
    const int DATA_DIMENSIONS = 6;
    void setupVertexAttribs();
    const GLfloat *constData() const { return m_data.constData(); }
    void add(const QVector3D &v, const QVector3D &c);

    QVector<GLfloat> m_data;
    int m_count;
    bool m_built = false;

    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;
    QOpenGLShaderProgram *m_program;
    int m_projMatrixLoc;
    int m_mvMatrixLoc;
};
