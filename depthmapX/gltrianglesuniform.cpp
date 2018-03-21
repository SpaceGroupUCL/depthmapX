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

#include "gltrianglesuniform.h"
#include <qmath.h>

static const char *vertexShaderSourceCore =
    "#version 150\n"
    "in vec4 vertex;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "void main() {\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "}\n";

static const char *fragmentShaderSourceCore =
    "#version 150\n"
    "uniform vec4 colourVector;\n"
    "out highp vec4 fragColor;\n"
    "void main() {\n"
    "   fragColor = colourVector;\n"
    "}\n";

static const char *vertexShaderSource =
    "attribute vec4 vertex;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "void main() {\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "}\n";

static const char *fragmentShaderSource =
    "uniform vec4 colourVector;\n"
    "void main() {\n"
    "   gl_FragColor = colourVector;\n"
    "}\n";

/**
 * @brief GLTrianglesUniform::GLTrianglesUniform
 * This class is an OpenGL representation of a set of triangles of uniform colour
 */

GLTrianglesUniform::GLTrianglesUniform()
    : m_count(0),
      m_program(0)
{

}

void GLTrianglesUniform::loadTriangleData(const std::vector<Point2f>& points, const QRgb &polyColour)
{
    m_built = false;

    m_count = 0;
    m_data.resize(points.size() * DATA_DIMENSIONS);

    for (auto& point: points)
    {
        add(QVector3D(point.x, point.y, 0.0f));
    }
    m_colour.setX(qRed(polyColour)/255.0);
    m_colour.setY(qGreen(polyColour)/255.0);
    m_colour.setZ(qBlue(polyColour)/255.0);
}

void GLTrianglesUniform::setupVertexAttribs()
{
    m_vbo.bind();
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, DATA_DIMENSIONS * sizeof(GLfloat), 0);
    m_vbo.release();
}

void GLTrianglesUniform::initializeGL(bool m_core)
{
    if(m_data.size() == 0) return;
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, m_core ? vertexShaderSourceCore : vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, m_core ? fragmentShaderSourceCore : fragmentShaderSource);
    m_program->bindAttributeLocation("vertex", 0);
    m_program->link();

    m_program->bind();
    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_program->uniformLocation("mvMatrix");
    m_colourVectorLoc = m_program->uniformLocation("colourVector");

    m_vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(constData(), m_count * sizeof(GLfloat));

    setupVertexAttribs();
    m_program->setUniformValue(m_colourVectorLoc, m_colour);
    m_program->release();
    m_built = true;
}

void GLTrianglesUniform::updateGL(bool m_core) {
    if(m_program == 0) {
        // has not been initialised yet, do that instead
        initializeGL(m_core);
    } else {
        m_vbo.bind();
        m_vbo.allocate(constData(), m_count * sizeof(GLfloat));
        m_vbo.release();
        m_built = true;
    }
}

void GLTrianglesUniform::updateColour(const QRgb &polyColour)
{
    m_colour.setX(qRed(polyColour)/255.0);
    m_colour.setY(qGreen(polyColour)/255.0);
    m_colour.setZ(qBlue(polyColour)/255.0);
    m_program->bind();
    m_program->setUniformValue(m_colourVectorLoc, m_colour);
    m_program->release();
}

void GLTrianglesUniform::cleanup()
{
    if(!m_built) return;
    m_vbo.destroy();
    delete m_program;
    m_program = 0;
}

void GLTrianglesUniform::paintGL(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel)
{
    if(!m_built) return;
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    m_program->bind();
    m_program->setUniformValue(m_projMatrixLoc, m_mProj);
    m_program->setUniformValue(m_mvMatrixLoc, m_mView * m_mModel);

    glDrawArrays(GL_TRIANGLES, 0, vertexCount());

    m_program->release();
}

void GLTrianglesUniform::add(const QVector3D &v)
{
    GLfloat *p = m_data.data() + m_count;
    *p++ = v.x();
    *p++ = v.y();
    *p++ = v.z();
    m_count += DATA_DIMENSIONS;
}
