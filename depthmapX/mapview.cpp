#include "mapview.h"

MapView::MapView(QGraphDoc &pDoc, Settings &settings, QWidget *parent)
    : QOpenGLWidget(parent), m_pDoc(pDoc), m_settings(settings)
{

}
