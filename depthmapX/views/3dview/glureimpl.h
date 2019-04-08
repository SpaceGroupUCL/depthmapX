// Taken from https://github.com/Alexpux/superglu/blob/master/libutil/project.c
// Only provided here temporarily until the 3DView is converted to modern OpenGL

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
**
** http://oss.sgi.com/projects/FreeB
**
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
**
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
**
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
** $Date$ $Revision$
** $Header$
*/
#pragma once

#include <math.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

namespace gluReimpl {
    /*
    ** Make m an identity matrix
    */
    static void __gluMakeIdentityd(GLdouble m[16]) {
        m[0 + 4 * 0] = 1;
        m[0 + 4 * 1] = 0;
        m[0 + 4 * 2] = 0;
        m[0 + 4 * 3] = 0;
        m[1 + 4 * 0] = 0;
        m[1 + 4 * 1] = 1;
        m[1 + 4 * 2] = 0;
        m[1 + 4 * 3] = 0;
        m[2 + 4 * 0] = 0;
        m[2 + 4 * 1] = 0;
        m[2 + 4 * 2] = 1;
        m[2 + 4 * 3] = 0;
        m[3 + 4 * 0] = 0;
        m[3 + 4 * 1] = 0;
        m[3 + 4 * 2] = 0;
        m[3 + 4 * 3] = 1;
    }

#define __glPi 3.14159265358979323846

    void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
        GLdouble m[4][4];
        double sine, cotangent, deltaZ;
        double radians = fovy / 2 * __glPi / 180;

        deltaZ = zFar - zNear;
        sine = sin(radians);
        if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
            return;
        }
        cotangent = cos(radians) / sine;

        __gluMakeIdentityd(&m[0][0]);
        m[0][0] = cotangent / aspect;
        m[1][1] = cotangent;
        m[2][2] = -(zFar + zNear) / deltaZ;
        m[2][3] = -1;
        m[3][2] = -2 * zNear * zFar / deltaZ;
        m[3][3] = 0;
        glMultMatrixd(&m[0][0]);
    }

    static void __gluMultMatrixVecd(const GLdouble matrix[16], const GLdouble in[4], GLdouble out[4]) {
        int i;

        for (i = 0; i < 4; i++) {
            out[i] = in[0] * matrix[0 * 4 + i] + in[1] * matrix[1 * 4 + i] + in[2] * matrix[2 * 4 + i] +
                     in[3] * matrix[3 * 4 + i];
        }
    }

    /*
    ** inverse = invert(src)
    */
    static int __gluInvertMatrixd(const GLdouble src[16], GLdouble inverse[16]) {
        int i, j, k, swap;
        double t;
        GLdouble temp[4][4];

        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                temp[i][j] = src[i * 4 + j];
            }
        }
        __gluMakeIdentityd(inverse);

        for (i = 0; i < 4; i++) {
            /*
            ** Look for largest element in column
            */
            swap = i;
            for (j = i + 1; j < 4; j++) {
                if (fabs(temp[j][i]) > fabs(temp[i][i])) {
                    swap = j;
                }
            }

            if (swap != i) {
                /*
                ** Swap rows.
                */
                for (k = 0; k < 4; k++) {
                    t = temp[i][k];
                    temp[i][k] = temp[swap][k];
                    temp[swap][k] = t;

                    t = inverse[i * 4 + k];
                    inverse[i * 4 + k] = inverse[swap * 4 + k];
                    inverse[swap * 4 + k] = t;
                }
            }

            if (temp[i][i] == 0) {
                /*
                ** No non-zero pivot.  The matrix is singular, which shouldn't
                ** happen.  This means the user gave us a bad matrix.
                */
                return GL_FALSE;
            }

            t = temp[i][i];
            for (k = 0; k < 4; k++) {
                temp[i][k] /= t;
                inverse[i * 4 + k] /= t;
            }
            for (j = 0; j < 4; j++) {
                if (j != i) {
                    t = temp[j][i];
                    for (k = 0; k < 4; k++) {
                        temp[j][k] -= temp[i][k] * t;
                        inverse[j * 4 + k] -= inverse[i * 4 + k] * t;
                    }
                }
            }
        }
        return GL_TRUE;
    }

    static void __gluMultMatricesd(const GLdouble a[16], const GLdouble b[16], GLdouble r[16]) {
        int i, j;

        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                r[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] + a[i * 4 + 1] * b[1 * 4 + j] +
                               a[i * 4 + 2] * b[2 * 4 + j] + a[i * 4 + 3] * b[3 * 4 + j];
            }
        }
    }

    GLint gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16],
                       const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *objx, GLdouble *objy,
                       GLdouble *objz) {
        double finalMatrix[16];
        double in[4];
        double out[4];

        __gluMultMatricesd(modelMatrix, projMatrix, finalMatrix);
        if (!__gluInvertMatrixd(finalMatrix, finalMatrix))
            return (GL_FALSE);

        in[0] = winx;
        in[1] = winy;
        in[2] = winz;
        in[3] = 1.0;

        /* Map x and y from window coordinates */
        in[0] = (in[0] - viewport[0]) / viewport[2];
        in[1] = (in[1] - viewport[1]) / viewport[3];

        /* Map to range -1 to 1 */
        in[0] = in[0] * 2 - 1;
        in[1] = in[1] * 2 - 1;
        in[2] = in[2] * 2 - 1;

        __gluMultMatrixVecd(finalMatrix, in, out);
        if (out[3] == 0.0)
            return (GL_FALSE);
        out[0] /= out[3];
        out[1] /= out[3];
        out[2] /= out[3];
        *objx = out[0];
        *objy = out[1];
        *objz = out[2];
        return (GL_TRUE);
    }
} // namespace glureimpl
