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

#include "glpolygons.h"
#include "glutriangulator.h"

/**
 * @brief GLPolygons::GLPolygons
 * This class is an OpenGL representation of multiple polygons of different colour
 */

void GLPolygons::loadPolygonData(const std::map<std::vector<Point2f>, PafColor>& colouredPolygons)
{
    m_polygons.clear();
    for (auto& colouredPolygon: colouredPolygons)
    {
        const std::vector<Point2f> & points = colouredPolygon.first;
        QRgb colour = qRgb(colouredPolygon.second.redb(),
                           colouredPolygon.second.greenb(),
                           colouredPolygon.second.blueb());

        m_polygons.push_back(std::unique_ptr<GLTrianglesUniform>(new GLTrianglesUniform));

        std::vector<Point2f> triangulated = GLUTriangulator::triangulate(points);
        m_polygons.back()->loadTriangleData(triangulated, colour);
    }
}

void GLPolygons::initializeGL(bool m_core)
{
    for (auto& polygon: m_polygons)
    {
        polygon->initializeGL(m_core);
    }
}

void GLPolygons::updateGL(bool m_core)
{
    for (auto& polygon: m_polygons)
    {
        polygon->updateGL(m_core);
    }
}

void GLPolygons::cleanup()
{
    for (auto& polygon: m_polygons)
    {
        polygon->cleanup();
    }
}

void GLPolygons::paintGL(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel)
{
    for (auto& polygon: m_polygons)
    {
        polygon->paintGL(m_mProj, m_mView, m_mModel);
    }
}
