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

#include "gldynamicrect.h"

static const char *vertexShaderSourceCore =
    "#version 150\n"
    "in float vertexIndex;\n"
    "int idxx = int(mod(vertexIndex,2.0));\n"
    "int idxy = int(vertexIndex/2.0);\n"
    "uniform mat2 diagVertices2D;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "void main() {\n"
    "   gl_Position = projMatrix * mvMatrix * vec4(diagVertices2D[0][idxx],diagVertices2D[1][idxy],0,1);\n"
    "}\n";

static const char *fragmentShaderSourceCore =
    "#version 150\n"
    "uniform vec4 colourVector;\n"
    "out highp vec4 fragColor;\n"
    "void main() {\n"
    "   fragColor = colourVector;\n"
    "}\n";

static const char *vertexShaderSource =
    "attribute float vertexIndex;\n"
    "int idxx = int(mod(vertexIndex,2.0));\n"
    "int idxy = int(vertexIndex/2.0);\n"
    "uniform mat2 diagVertices2D;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "void main() {\n"
    "   gl_Position = projMatrix * mvMatrix * vec4(diagVertices2D[0][idxx],diagVertices2D[1][idxy],0,1);\n"
    "}\n";

static const char *fragmentShaderSource =
    "uniform vec4 colourVector;\n"
    "void main() {\n"
    "   gl_FragColor = colourVector;\n"
    "}\n";

/**
 * @brief GLDynamicRect::GLDynamicRect
 * This class is an OpenGL representation of a dynamic rectangle. The only attribute
 * sent to the shader is the index of 4 vertices, and the actual position of the
 * vertices is sent as a uniform 2x2 matrix (bottom-left-x, bottom-left-y, top-right-x,
 * top-right-y)
 */

GLDynamicRect::GLDynamicRect()
    : m_count(0),
      m_program(0)
{
    m_count = 0;
    m_data.resize(4);

    add(0);
    add(1);
    add(3);
    add(2);
}

void GLDynamicRect::setupVertexAttribs()
{
    m_vbo.bind();
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, DATA_DIMENSIONS, GL_FLOAT, GL_FALSE, DATA_DIMENSIONS * sizeof(GLfloat), 0);
    m_vbo.release();
}

void GLDynamicRect::initializeGL(bool m_core)
{
    if(m_data.size() == 0) return;
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, m_core ? vertexShaderSourceCore : vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, m_core ? fragmentShaderSourceCore : fragmentShaderSource);
    m_program->bindAttributeLocation("vertexIndex", 0);
    m_program->link();

    m_program->bind();
    m_diagVertices2DLoc = m_program->uniformLocation("diagVertices2D");
    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_program->uniformLocation("mvMatrix");
    m_colourVectorLoc = m_program->uniformLocation("colourVector");

    m_vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(constData(), m_count * sizeof(GLfloat));

    setupVertexAttribs();
    m_program->setUniformValue(m_colourVectorLoc, m_colour_fill);
    m_program->release();
    m_built = true;
}

void GLDynamicRect::updateGL(bool m_core) {
    if(m_program == 0) {
        // has not been initialised yet, do that instead
        initializeGL(m_core);
    } else {
        m_vbo.bind();
        m_vbo.allocate(constData(), m_count * sizeof(GLfloat));
        m_vbo.release();
    }
}

void GLDynamicRect::setFillColour(const QRgb &fillColour)
{
    m_colour_fill.setX(qRed(fillColour)/255.0);
    m_colour_fill.setY(qGreen(fillColour)/255.0);
    m_colour_fill.setZ(qBlue(fillColour)/255.0);
}

void GLDynamicRect::setStrokeColour(const QRgb &strokeColour)
{
    m_colour_stroke.setX(qRed(strokeColour)/255.0);
    m_colour_stroke.setY(qGreen(strokeColour)/255.0);
    m_colour_stroke.setZ(qBlue(strokeColour)/255.0);
}

void GLDynamicRect::cleanup()
{
    m_vbo.destroy();
    delete m_program;
    m_program = 0;
}

void GLDynamicRect::paintGL(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel, const QMatrix2x2 &m_selectionBounds)
{
    if(!m_built) return;
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    m_program->bind();
    m_program->setUniformValue(m_projMatrixLoc, m_mProj);
    m_program->setUniformValue(m_mvMatrixLoc, m_mView * m_mModel);
    m_program->setUniformValue(m_diagVertices2DLoc, m_selectionBounds);

    m_program->setUniformValue(m_colourVectorLoc, m_colour_fill);
    glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount());

    m_program->setUniformValue(m_colourVectorLoc, m_colour_stroke);
    glDrawArrays(GL_LINE_LOOP, 0, vertexCount());

    m_program->release();
}

void GLDynamicRect::add(const GLfloat v)
{
    GLfloat *p = m_data.data() + m_count;
    *p++ = v;
    m_count += DATA_DIMENSIONS;
}
