#include "viewhelpers.h"
#include <time.h>

namespace ViewHelpers {
    Point2f calculateCenter(const QPoint& point, const QPoint &oldCentre, double factor)
    {
        int diffX = oldCentre.x() - point.x();
        int diffY = oldCentre.y() - point.y();
        return Point2f(point.x() + double(diffX) * factor,
                       point.y() + double(diffY) * factor);
    }

    std::string getCurrentDate()
    {
        time_t now = time(NULL);
        char timeString[11];
        strftime(timeString, 11, "%Y/%m/%d", localtime(&now));
        return timeString;
    }

}
