// adapted from: https://stackoverflow.com/questions/12600325/force-glutesselator-to-generate-only-gl-triangles

#include "glutriangulator.h"

struct TessContext
{
    ~TessContext()
    {
        for( size_t i = 0; i < combined.size(); ++i )
        {
            delete[] combined[i];
        }
    }

    typedef std::pair< double, double > Point;
    std::vector< Point > pts;
    std::vector< GLdouble* > combined;
};

#ifdef _WIN32
void __stdcall tess_begin( GLenum ) {}
void __stdcall tess_edgeFlag( GLboolean ) {}
void __stdcall tess_end() {}
#else
void tess_begin( GLenum ) {}
void tess_edgeFlag( GLboolean ) {}
void tess_end() {}
#endif

#ifdef _WIN32
void __stdcall tess_vertex
#else
void tess_vertex
#endif
    (
    void*           data,
    TessContext*    ctx
    )
{
    GLdouble* coord = (GLdouble*)data;
    ctx->pts.push_back( TessContext::Point( coord[0], coord[1] ) );
}

#ifdef _WIN32
void __stdcall tess_combine
#else
void tess_combine
#endif
    (
    GLdouble        coords[3],
    void*           vertex_data[4],
    GLfloat         weight[4],
    void**          outData,
    TessContext*    ctx
    )
{
    GLdouble* newVert = new GLdouble[3];
    ctx->combined.push_back( newVert );

    newVert[0] = coords[0];
    newVert[1] = coords[1];
    newVert[2] = coords[2];
    *outData = newVert;
}

std::vector< Point2f > GLUTriangulator::triangulate
    (
    const std::vector< Point2f >& polygon
    )
{
    std::vector< GLdouble > coords;
    for( size_t i = 0; i < polygon.size(); ++i )
    {
        coords.push_back( polygon[i].x );
        coords.push_back( polygon[i].y );
        coords.push_back( 0 );
    }

    GLUtesselator* tess = gluNewTess();
#ifdef _WIN32
    gluTessCallback( tess, GLU_TESS_BEGIN,          (void (__stdcall *)())   tess_begin      );
    gluTessCallback( tess, GLU_TESS_EDGE_FLAG,      (void (__stdcall *)())   tess_edgeFlag   );
    gluTessCallback( tess, GLU_TESS_VERTEX_DATA,    (void (__stdcall *)())   tess_vertex     );
    gluTessCallback( tess, GLU_TESS_END,            (void (__stdcall *)())   tess_end        );
    gluTessCallback( tess, GLU_TESS_COMBINE_DATA,   (void (__stdcall *)())   tess_combine    );
#else
    gluTessCallback( tess, GLU_TESS_BEGIN,          (void (*)())   tess_begin      );
    gluTessCallback( tess, GLU_TESS_EDGE_FLAG,      (void (*)())   tess_edgeFlag   );
    gluTessCallback( tess, GLU_TESS_VERTEX_DATA,    (void (*)())   tess_vertex     );
    gluTessCallback( tess, GLU_TESS_END,            (void (*)())   tess_end        );
    gluTessCallback( tess, GLU_TESS_COMBINE_DATA,   (void (*)())   tess_combine    );
#endif
    gluTessNormal( tess, 0.0, 0.0, 1.0 );

    TessContext ctx;

    gluTessBeginPolygon( tess, &ctx );
    gluTessBeginContour( tess );

    for( size_t i = 0; i < polygon.size(); ++i )
    {
        gluTessVertex( tess, &coords[i*3], &coords[i*3] );
    }

    gluTessEndContour( tess );
    gluTessEndPolygon( tess );

    gluDeleteTess(tess);

    std::vector< Point2f > ret( ctx.pts.size() );
    for( size_t i = 0; i < ret.size(); ++i )
    {
        ret[i].x = ctx.pts[i].first;
        ret[i].y = ctx.pts[i].second;
    }

    return ret;
}
