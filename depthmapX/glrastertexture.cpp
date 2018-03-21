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

#include "glrastertexture.h"
#include <qmath.h>
#include <QOpenGLFunctions>

static const char *vertexShaderSourceCore =
    "#version 150\n"
    "in vec4 vertex;\n"
    "in vec4 texCoord;\n"
    "out vec4 texc;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "void main() {\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "    texc = texCoord;\n"
    "}\n";

static const char *fragmentShaderSourceCore =
    "#version 150\n"
    "uniform sampler2D texture;\n"
    "in mediump vec4 texc;\n"
    "void main(void)\n"
    "{\n"
    "    gl_FragColor = texture2D(texture, texc.st);\n"
    "}\n";

static const char *vertexShaderSource =
    "attribute highp vec4 vertex;\n"
    "attribute mediump vec4 texCoord;\n"
    "varying mediump vec4 texc;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "void main(void)\n"
    "{\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "   texc = texCoord;\n"
    "}\n";

static const char *fragmentShaderSource =
    "uniform sampler2D texture;\n"
    "varying mediump vec4 texc;\n"
    "void main(void)\n"
    "{\n"
    "    gl_FragColor = texture2D(texture, texc.st);\n"
    "}\n";

GLRasterTexture::GLRasterTexture()
    : m_count(0),
      m_program(0),
      m_texture(QOpenGLTexture::Target2D)
{

}
void GLRasterTexture::loadRegionData(float minX, float minY, float maxX, float maxY)
{
    m_built = false;

    m_count = 0;
    m_data.resize(4 * DATA_DIMENSIONS);

    add(QVector3D(minX,minY,0),QVector2D(0, 0));
    add(QVector3D(maxX,minY,0),QVector2D(1, 0));
    add(QVector3D(maxX,maxY,0),QVector2D(1, 1));
    add(QVector3D(minX,maxY,0),QVector2D(0, 1));
}

void GLRasterTexture::setupVertexAttribs()
{
    m_vbo.bind();
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, DATA_DIMENSIONS * sizeof(GLfloat),
                             0);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, DATA_DIMENSIONS * sizeof(GLfloat),
                             reinterpret_cast<void *>(3 * sizeof(GLfloat)));
    m_vbo.release();
}

void GLRasterTexture::initializeGL(bool coreProfile) {
    if(m_data.size() == 0) return;
    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, coreProfile ? vertexShaderSourceCore : vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, coreProfile ? fragmentShaderSourceCore : fragmentShaderSource);
    m_program->bindAttributeLocation("vertex", 0);
    m_program->bindAttributeLocation("texCoord", 1);
    m_program->link();

    m_program->bind();
    m_projMatrixLoc = m_program->uniformLocation("projMatrix");
    m_mvMatrixLoc = m_program->uniformLocation("mvMatrix");
    m_textureSamplerLoc = m_program->uniformLocation("texture");

    m_vao.create();
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

    // Setup our vertex buffer object.
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(constData(), m_count * sizeof(GLfloat));

    // Store the vertex attribute bindings for the program.
    setupVertexAttribs();

    m_program->setUniformValue(m_textureSamplerLoc, 0);

    m_program->release();
    m_built = true;
}

void GLRasterTexture::updateGL(bool coreProfile) {
    if(m_program == 0) {
        // has not been initialised yet, do that instead
        initializeGL(coreProfile);
    } else {
        m_vbo.bind();
        m_vbo.allocate(constData(), m_count * sizeof(GLfloat));
        m_vbo.release();
        m_built = true;
    }
}

void GLRasterTexture::loadPixelData(QImage &data)
{
    if(!m_built) return;
    m_program->bind();
    if(m_texture.isCreated())
    {
        m_texture.destroy();
    }
    m_texture.setData(data);
    m_program->release();
}

void GLRasterTexture::cleanup()
{
    if(!m_built) return;
    m_vbo.destroy();
    m_texture.destroy();
    delete m_program;
    m_program = 0;
}

void GLRasterTexture::paintGL(const QMatrix4x4 &m_mProj, const QMatrix4x4 &m_mView, const QMatrix4x4 &m_mModel)
{
    if(!m_built) return;
    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
    m_program->bind();
    m_program->setUniformValue(m_projMatrixLoc, m_mProj);
    m_program->setUniformValue(m_mvMatrixLoc, m_mView * m_mModel);

    m_texture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount());

    m_program->release();
}

void GLRasterTexture::add(const QVector3D &v, const QVector2D &tc)
{
    GLfloat *p = m_data.data() + m_count;
    *p++ = v.x();
    *p++ = v.y();
    *p++ = v.z();
    *p++ = tc.x();
    *p++ = tc.y();
    m_count += DATA_DIMENSIONS;
}
