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
#include "depthmapX/gltrianglesuniform.h"
#include <qopengl.h>
#include <QVector>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <memory>

/**
 * @brief The GLPolygons class is a plain wrapper class for multiple GLPolygon
 * that acts as if it's a single globject
 */
class GLPolygons
{
public:
    void loadPolygonData(const std::map<std::vector<Point2f>, PafColor>& colouredPolygons);
    void paintGL(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel);
    void initializeGL(bool m_core);
    void updateGL(bool m_core);
    void cleanup();

private:
    std::vector<std::unique_ptr<GLTrianglesUniform>> m_polygons;
};
