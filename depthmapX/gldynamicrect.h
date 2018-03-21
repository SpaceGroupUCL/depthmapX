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

#include <qopengl.h>
#include <QVector>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QRgb>

class GLDynamicRect
{
public:
    GLDynamicRect();
    void paintGL(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel, const QMatrix2x2 &m_selectionBounds);
    void initializeGL(bool m_core);
    void updateGL(bool m_core);
    void cleanup();
    void setFillColour(const QRgb &fillColour);
    void setStrokeColour(const QRgb &strokeColour);
    int vertexCount() const { return m_count / DATA_DIMENSIONS; }

    GLDynamicRect( const GLDynamicRect& ) = delete;
    GLDynamicRect& operator=(const GLDynamicRect& ) = delete;

protected:
    void add(const GLfloat v);

    int m_count;
    bool m_built = false;
    QVector<GLfloat> m_data;

    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLShaderProgram *m_program;

    int m_diagVertices2DLoc;
    int m_projMatrixLoc;
    int m_mvMatrixLoc;
    int m_colourVectorLoc;

    QVector4D m_colour_fill = QVector4D(0.0f, 1.0f, 0.0f, 0.3f);
    QVector4D m_colour_stroke = QVector4D(1.0f, 1.0f, 1.0f, 1.0f);

private:
    const int DATA_DIMENSIONS = 1;
    void setupVertexAttribs();
    int count() const { return m_count; }
    const GLfloat *constData() const { return m_data.constData(); }
};
