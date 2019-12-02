// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Tasos Varoudis

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

// This is my code to make a set of axial lines from a set of boundary lines

// spatial data

#include "salalib/spacepix.h"

#include "genlib/stringutils.h"
#include "genlib/readwritehelpers.h"
#include "genlib/containerutils.h"

#include <float.h>
#include <math.h>
#include <set>

#ifndef _WIN32
#define _finite finite
#endif

/*
// Algorithm from Chi
// make sure dx > dy
   dx = x1 - x0;
   dy = y1 - y0;
   x = x0; y = y0;
   d = 2*dy - dx;
   inc1 = 2*dy;
   inc2 = 2*(dy-dx);
   while (x < x1) {
      if (d <= 0) {
         d += inc1;
         x += 1;
      }
      else {
         d += inc2;
         x++;
         y++;
      }
      pixel_list.push_back( PixelRef(x,y) );
   }
*/

PixelRefVector PixelBase::pixelateLine(Line l, int scalefactor) const {
    PixelRefVector pixel_list;

    // this is *not* correct for lines that are off the edge...
    // should use non-constrained version (false), and find where line enters the region
    PixelRef a = pixelate(l.start(), true, scalefactor);
    PixelRef b = pixelate(l.end(), true, scalefactor);

    l.normalScale(m_region);

    pixel_list.push_back(a);

    int scaledcols = m_cols * scalefactor;
    int scaledrows = m_rows * scalefactor;

    int parity = 1; // Line goes upwards
    if (a.y > b.y) {
        parity = -1; // Line goes downwards
        a.y *= -1;
        b.y *= -1; // Set ay and by saves work on comparisons later on
    }

    // special case 1
    if (a.x == b.x) {
        while (a.y < b.y) {
            a.y += 1;
            pixel_list.push_back(PixelRef(a.x, parity * a.y));
        }
    } else if (a.y == b.y) {
        while (a.x < b.x) {
            a.x += 1;
            pixel_list.push_back(PixelRef(a.x, parity * a.y)); // Lines always go left to right
        }
    } else {

        double hw_ratio =
            l.height() / l.width(); // Working all of these out leaves less scope for floating point error
        double wh_ratio = l.width() / l.height();
        double x0_const = l.ay() - double(parity) * hw_ratio * l.ax();
        double y0_const = l.ax() - double(parity) * wh_ratio * l.ay();

        while (a.x < b.x || a.y < b.y) {
            PixelRef e;
            e.y = parity *
                  int(double(scaledrows) * (x0_const + parity * hw_ratio * (double(a.x + 1) / double(scaledcols))));
            // Note when decending 1.5 -> 1 and ascending 1.5 -> 2
            if (parity < 0) {
                e.x = int(double(scaledcols) * (y0_const + wh_ratio * (double(a.y) / double(scaledrows))));
            } else {
                e.x = int(double(scaledcols) * (y0_const + wh_ratio * (double(a.y + 1) / double(scaledrows))));
            }

            if (a.y < e.y) {
                while (a.y < e.y && a.y < b.y) {
                    a.y += 1;
                    pixel_list.push_back(PixelRef(a.x, parity * a.y));
                }
                if (a.x < b.x) {
                    a.x += 1;
                    pixel_list.push_back(PixelRef(a.x, parity * a.y));
                }
            } else if (a.x < e.x) {
                while (a.x < e.x && a.x < b.x) {
                    a.x += 1;
                    pixel_list.push_back(PixelRef(a.x, parity * a.y));
                }
                if (a.y < b.y) {
                    a.y += 1;
                    pixel_list.push_back(PixelRef(a.x, parity * a.y));
                }
            } else {
                // Special case: exactly diagonal step (should only require one step):
                // (Should actually never happen) (Doesn't: checked with RFH)
                a.x += 1;
                pixel_list.push_back(PixelRef(a.x, parity * a.y));
                a.y += 1;
                pixel_list.push_back(PixelRef(a.x, parity * a.y));
            }
        }
    }
    return pixel_list;
}

// this version includes all pixels through which the line passes with touching
// counting as both pixels.

PixelRefVector PixelBase::pixelateLineTouching(Line l, double tolerance) const {
    PixelRefVector pixel_list;

    // now assume that scaling to region then scaling up is going to give pixelation
    // this is not necessarily the case!
    l.normalScale(m_region);
    l.scale(Point2f(m_cols, m_rows));

    // but it does give us a nice line...
    int dir;
    double grad, constant;

    if (l.width() > l.height()) {
        dir = XAXIS;
        grad = l.grad(YAXIS);
        constant = l.constant(YAXIS);
    } else {
        dir = YAXIS;
        grad = l.grad(XAXIS);
        constant = l.constant(XAXIS);
    }
    PixelRef bounds(m_cols, m_rows);

    if (dir == XAXIS) {
        int first = (int)floor(l.ax() - tolerance);
        int last = (int)floor(l.bx() + tolerance);
        for (int i = first; i <= last; i++) {
            int j1 = (int)floor((first == i ? l.ax() : double(i)) * grad + constant - l.sign() * tolerance);
            int j2 = (int)floor((last == i ? l.bx() : double(i + 1)) * grad + constant + l.sign() * tolerance);
            if (bounds.encloses(PixelRef(i, j1))) {
                pixel_list.push_back(PixelRef(i, j1));
            }
            if (j1 != j2) {
                if (bounds.encloses(PixelRef(i, j2))) {
                    pixel_list.push_back(PixelRef(i, j2));
                }
                if (abs(j2 - j1) == 2) {
                    // this rare event happens if lines are exactly diagonal
                    int j3 = (j1 + j2) / 2;
                    if (bounds.encloses(PixelRef(i, j3))) {
                        pixel_list.push_back(PixelRef(i, j3));
                    }
                }
            }
        }
    } else {
        int first = (int)floor(l.bottom_left.y - tolerance);
        int last = (int)floor(l.top_right.y + tolerance);
        for (int i = first; i <= last; i++) {
            int j1 = (int)floor((first == i ? l.bottom_left.y : double(i)) * grad + constant - l.sign() * tolerance);
            int j2 = (int)floor((last == i ? l.top_right.y : double(i + 1)) * grad + constant + l.sign() * tolerance);
            if (bounds.encloses(PixelRef(j1, i))) {
                pixel_list.push_back(PixelRef(j1, i));
            }
            if (j1 != j2) {
                if (bounds.encloses(PixelRef(j2, i))) {
                    pixel_list.push_back(PixelRef(j2, i));
                }
                if (abs(j2 - j1) == 2) {
                    // this rare event happens if lines are exactly diagonal
                    int j3 = (j1 + j2) / 2;
                    if (bounds.encloses(PixelRef(j3, i))) {
                        pixel_list.push_back(PixelRef(j3, i));
                    }
                }
            }
        }
    }

    return pixel_list;
}

// this version for a quick set of pixels

PixelRefVector PixelBase::quickPixelateLine(PixelRef p, PixelRef q) {
    PixelRefVector list;

    double dx = q.x - p.x;
    double dy = q.y - p.y;
    int polarity = -1;
    double t = 0;
    // Quick mod - TV
#if defined(_MSC_VER)
    if (abs(dx) == abs(dy)) {
#else
    if (fabs(dx) == fabs(dy)) {
#endif
        polarity = 0;
    }
#if defined(_MSC_VER)
    else if (abs(dx) > abs(dy)) {
        t = abs(dx);
#else
    else if (fabs(dx) > fabs(dy)) {
        t = fabs(dx);
#endif
        polarity = 1;
    } else {
#if defined(_MSC_VER)
        t = abs(dy);
#else
        t = fabs(dy);
#endif
        polarity = 2;
    }

    dx /= t;
    dy /= t;
    double ppx = p.x + 0.5;
    double ppy = p.y + 0.5;

    for (int i = 0; i <= t; i++) {
        if (polarity == 1 && fabs(floor(ppy) - ppy) < 1e-9) {
            list.push_back(PixelRef((short)floor(ppx), (short)floor(ppy + 0.5)));
            list.push_back(PixelRef((short)floor(ppx), (short)floor(ppy - 0.5)));
        } else if (polarity == 2 && fabs(floor(ppx) - ppx) < 1e-9) {
            list.push_back(PixelRef((short)floor(ppx + 0.5), (short)floor(ppy)));
            list.push_back(PixelRef((short)floor(ppx - 0.5), (short)floor(ppy)));
        } else {
            list.push_back(PixelRef((short)floor(ppx), (short)floor(ppy)));
        }
        ppx += dx;
        ppy += dy;
    }

    return list;
}

SpacePixel::SpacePixel(const std::string &name) : m_pixel_lines(0, 0) {
    m_name = name;
    m_show = true;
    m_edit = false;

    m_cols = 0;
    m_rows = 0;

    m_ref = -1;
    m_test = 0;

    m_newline = false;

    m_style = 0;
    m_color = 0;
}

SpacePixel::SpacePixel(const SpacePixel &spacepixel)
    : m_pixel_lines(spacepixel.m_pixel_lines.rows(), spacepixel.m_pixel_lines.columns()) {
    // n.b., not strictly allowed
    construct(spacepixel);
}

SpacePixel &SpacePixel::operator=(const SpacePixel &spacepixel) {
    if (this != &spacepixel) {
        construct(spacepixel);
    }
    return *this;
}

void SpacePixel::construct(const SpacePixel &spacepixel) {
    m_name = spacepixel.m_name;
    m_show = spacepixel.m_show;
    m_edit = spacepixel.m_edit;

    m_rows = spacepixel.m_rows;
    m_cols = spacepixel.m_cols;

    m_region = spacepixel.m_region;

    m_ref = spacepixel.m_ref;
    m_test = spacepixel.m_test;
    m_lines = spacepixel.m_lines;
    m_newline = true;

    if (!m_rows || !m_cols) {
        m_display_lines.clear();
        return;
    }

    m_pixel_lines = spacepixel.m_pixel_lines;

    m_color = spacepixel.m_color;
    m_style = spacepixel.m_style;

    // m_pixel_height = spacepixel.m_pixel_height;
    // m_pixel_width  = spacepixel.m_pixel_width;
}

PixelRef SpacePixel::pixelate(const Point2f &p, bool constrain, int) const {
    PixelRef r;

    Point2f p1 = p;
    p1.normalScale(m_region);

    r.x = short(p1.x * double(m_cols - 1e-9));
    if (constrain) {
        if (r.x >= m_cols)
            r.x = m_cols - 1;
        else if (r.x < 0)
            r.x = 0;
    }
    r.y = short(p1.y * double(m_rows - 1e-9));
    if (constrain) {
        if (r.y >= m_rows)
            r.y = m_rows - 1;
        else if (r.y < 0)
            r.y = 0;
    }

    return r;
}

void SpacePixel::makeViewportLines(const QtRegion &viewport) const {
    if (m_display_lines.empty() || m_newline) {
        m_display_lines = std::vector<int>(m_lines.size());
        m_newline = false;
        std::fill(m_display_lines.begin(), m_display_lines.end(), 0);
    }

    m_current = -1; // note: findNext expects first to be labelled -1

    /*
    // Fixing bounding rectangle: normalisation removed
    QtRegion r_viewport = viewport;

    r_viewport.normalScale( m_region );
    */

    PixelRef bl = pixelate(viewport.bottom_left);
    PixelRef tr = pixelate(viewport.top_right);

    for (int i = bl.x; i <= tr.x; i++) {
        for (int j = bl.y; j <= tr.y; j++) {
            auto &pixel_lines = m_pixel_lines(static_cast<size_t>(j), static_cast<size_t>(i));
            for (int pixel_line : pixel_lines) {
                m_display_lines[size_t(depthmapX::findIndexFromKey(m_lines, pixel_line))] = 1;
            }
        }
    }
}

// expect to be used as:
//
// if (findNext())
//    getNext();

bool SpacePixel::findNextLine(bool &nextlayer) const {
    if (m_newline) // after adding a line you must reinitialise the display lines
        return false;

    while (++m_current < (int)m_lines.size() && m_display_lines[m_current] == 0)
        ;

    if (m_current < (int)m_lines.size()) {
        return true;
    } else {
        m_current = (int)m_lines.size();
        nextlayer = true;
        return false;
    }
}

const Line &SpacePixel::getNextLine() const {
    m_display_lines[m_current] = 0; // You've drawn it
    /*
    // Fixing: removed rectangle scaling
    l.denormalScale( m_region );
    */
    return m_lines.find(m_current)->second.line;
}

void SpacePixel::initLines(int size, const Point2f &min, const Point2f &max, double density) {
    m_display_lines.clear();
    m_lines.clear();
    m_ref = -1;
    m_test = 0;

    // work out extents...
    m_region = QtRegion(min, max);

    double wh_ratio = m_region.width() / m_region.height();
    double hw_ratio = m_region.height() / m_region.width();

    m_rows = (int)sqrt(double(size) * wh_ratio * density);
    m_cols = (int)sqrt(double(size) * hw_ratio * density);

    if (m_rows < 1)
        m_rows = 1;
    if (m_cols < 1)
        m_cols = 1;

    // could work these two out on the fly, but it's easier to have them stored:
    // m_pixel_height = m_region.height() / double(m_rows);
    // m_pixel_width  = m_region.width()  / double(m_cols);

    m_pixel_lines = depthmapX::RowMatrix<std::vector<int>>(static_cast<size_t>(m_rows), static_cast<size_t>(m_cols));
}

void SpacePixel::reinitLines(double density) {
    m_display_lines.clear();

    double wh_ratio = m_region.width() / m_region.height();
    double hw_ratio = m_region.height() / m_region.width();

    m_rows = (int)sqrt(double(m_lines.size()) * wh_ratio * density);
    m_cols = (int)sqrt(double(m_lines.size()) * hw_ratio * density);

    if (m_rows < 1)
        m_rows = 1;
    if (m_cols < 1)
        m_cols = 1;

    m_pixel_lines = depthmapX::RowMatrix<std::vector<int>>(static_cast<size_t>(m_rows), static_cast<size_t>(m_cols));

    // now re-add the lines:
    for (auto line : m_lines) {
        PixelRefVector list = pixelateLine(line.second.line);
        for (size_t j = 0; j < list.size(); j++) {
            // note: m_pixel_lines will be reordered by sortPixelLines
            m_pixel_lines(static_cast<size_t>(list[j].y), static_cast<size_t>(list[j].x)).push_back(line.first);
        }
    }

    // and finally sort:
    sortPixelLines();

    // flag as newline just in case:
    m_newline = true;
}

// Add line: pixelate the line

void SpacePixel::addLine(const Line &line) {
    // Fairly simple: just pixelates the line!
    m_ref++; // need unique keys for the lines so they can be added / removed at any time
    m_lines.insert(std::make_pair(m_ref, LineTest(line, 0)));
    m_newline = true;

    PixelRefVector list = pixelateLine(line);

    for (size_t i = 0; i < list.size(); i++) {
        // note: m_pixel_lines will be reordered by sortPixelLines
        m_pixel_lines(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x)).push_back(m_ref);
    }
}

int SpacePixel::addLineDynamic(const Line &line) {
    m_ref++; // need unique keys for the lines so they can be added / removed at any time
    m_lines.insert(std::make_pair(m_ref, LineTest(line, 0)));
    m_newline = true;

    PixelRefVector list = pixelateLine(line);

    for (size_t i = 0; i < list.size(); i++) {
        // note: dynamic lines could be dodgy... only pixelate bits that fall in range
        if (list[i].x >= 0 && list[i].y >= 0 && list[i].x < m_cols && list[i].y < m_rows) {
            // note, this probably won't be reordered on dynamic
            m_pixel_lines(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x)).push_back(m_ref);
        }
    }

    return m_ref;
}

void SpacePixel::sortPixelLines() {
    for (size_t i = 0; i < static_cast<size_t>(m_cols); i++) {
        for (size_t j = 0; j < static_cast<size_t>(m_rows); j++) {
            std::vector<int> &pixel_lines = m_pixel_lines(j, i);
            // tidy up in case of removal
            for (auto rev_iter = pixel_lines.rbegin(); rev_iter != pixel_lines.rend(); ++rev_iter) {
                if (m_lines.find(*rev_iter) == m_lines.end()) {
                    pixel_lines.erase(std::next(rev_iter).base());
                }
            }
            std::sort(pixel_lines.begin(), pixel_lines.end());
        }
    }
}

bool SpacePixel::intersect(const Line &l, double tolerance) {
    m_test++; // note loops! (but vary rarely: inevitabley, lines will have been marked before it loops)

    PixelRefVector list = pixelateLine(l);

    for (size_t i = 0; i < list.size(); i++) {
        auto &pixel_lines = m_pixel_lines(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
        for (int lineref : pixel_lines) {
            LineTest &linetest = m_lines.find(lineref)->second;
            if (linetest.test != m_test) {
                if (intersect_region(linetest.line, l)) {
                    if (intersect_line(linetest.line, l, tolerance)) {
                        return true;
                    }
                }
                linetest.test = m_test;
            }
        }
    }

    return false;
}

bool SpacePixel::intersect_exclude(const Line &l, double tolerance) {
    m_test++; // note loops! (but vary rarely: inevitabley, lines will have been marked before it loops)

    PixelRefVector list = pixelateLine(l);

    for (size_t i = 0; i < list.size(); i++) {
        auto &pixel_lines = m_pixel_lines(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
        for (int lineref : pixel_lines) {
            LineTest &linetest = m_lines.find(lineref)->second;
            if (linetest.test != m_test) {
                if (intersect_region(linetest.line, l)) {
                    if (intersect_line(linetest.line, l, tolerance)) {
                        if (linetest.line.start() != l.start() && linetest.line.start() != l.end() &&
                            linetest.line.end() != l.start() && linetest.line.end() != l.end()) {
                            return true;
                        }
                    }
                }
                linetest.test = m_test;
            }
        }
    }

    return false;
}

void SpacePixel::cutLine(Line &l, short dir) {
    m_test++;

    double tolerance = l.length() * 1e-9;

    std::set<double> loc;
    PixelRefVector vec = pixelateLine(l);

    int axis;
    if (l.width() >= l.height()) {
        axis = XAXIS;
    } else {
        axis = YAXIS;
    }
    Point2f truestart = (dir == l.direction()) ? l.start() : l.end();
    Point2f trueend = (dir == l.direction()) ? l.end() : l.start();

    bool found = false;
    std::vector<Line> touching_lines;

    for (size_t i = 0; i < vec.size() && !found; i++) {
        // depending on direction of line either move head to tail or tail to head
        PixelRef pix = (dir == l.direction()) ? vec[i] : vec[vec.size() - 1 - i];
        auto &pixel_lines = m_pixel_lines(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
        for (int lineref : pixel_lines) {
            // try {
            LineTest &linetest = m_lines.find(lineref)->second;
            if (linetest.test != m_test) {
                if (intersect_region(linetest.line, l, tolerance * linetest.line.length())) {
                    switch (intersect_line_distinguish(linetest.line, l, tolerance * linetest.line.length())) {
                    case 0:
                        break;
                    case 2: {
                        loc.insert(l.intersection_point(linetest.line, axis));
                    } break;
                    case 1:
                        if (truestart != linetest.line.start() && truestart != linetest.line.end()) {
                            if (!touching_lines.size()) {
                                touching_lines.push_back(linetest.line);
                            } else {
                                Point2f a, b;
                                int pair = -1;
                                // if there may be more than one touches in the same pixel, we have to build a list of
                                // possibles...
                                for (size_t k = 0; k < touching_lines.size() && pair == -1; k++) {
                                    if (linetest.line.start() == touching_lines[k].start() ||
                                        linetest.line.end() == touching_lines[k].end()) {
                                        a = linetest.line.end() - linetest.line.start();
                                        pair = k;
                                    } else if (linetest.line.start() == touching_lines[k].end() ||
                                               linetest.line.end() == touching_lines[k].start()) {
                                        a = linetest.line.start() - linetest.line.end();
                                        pair = k;
                                    }
                                    if (pair != -1) {
                                        b = touching_lines[pair].end() - touching_lines[pair].start();
                                        Point2f p = trueend - truestart;
                                        double oa = det(p, a);
                                        double ob = det(p, b);
                                        if (sgn(oa) != sgn(ob) || fabs(oa) < tolerance * linetest.line.length() ||
                                            fabs(ob) < tolerance * linetest.line.length()) {
                                            // crossed
                                            if (fabs(oa) >
                                                tolerance * linetest.line.length()) { // checks not parallel...
                                                loc.insert(l.intersection_point(linetest.line, axis));
                                            } else if (fabs(ob) > tolerance * linetest.line.length()) {
                                                loc.insert(l.intersection_point(touching_lines[pair], axis));
                                            } else {
                                                // parallel with both lines ... this shouldn't happen...
                                                std::cerr << "couldn't chop at boundary" << std::endl;
                                            }
                                        }
                                    }
                                    pair = -1;
                                }
                                touching_lines.push_back(linetest.line);
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }
                linetest.test = m_test;
            }
            //}
            // catch (pexception) {
            // the lineref may have been deleted -- this is supposed to be tidied up
            // just ignore...
            //   cerr << "cut line exception -- missing line?" << endl;
            //}
        }
        if (loc.size()) {
            // there's no guarantee the loc actually happened in this pixel...
            // check the first loc actually occurred in this pixel...
            if ((dir == l.direction() && (axis == XAXIS || l.sign() == 1)) ||
                (dir != l.direction() && (axis == YAXIS && l.sign() == -1))) {
                if (pix == pixelate(l.point_on_line(*loc.begin(), axis))) {
                    found = true;
                }
            } else {
                if (pix == pixelate(l.point_on_line(*loc.rbegin(), axis))) {
                    found = true;
                }
            }
        }
    }

    if (loc.size()) {
        // it intersected...
        double pos;
        if (dir == l.direction()) {
            if (axis == XAXIS) {
                pos = *loc.begin();
                l.by() = l.ay() + l.sign() * l.height() * (pos - l.ax()) / l.width();
                l.bx() = pos;
            } else if (l.sign() == 1) {
                pos = *loc.begin();
                l.bx() = l.ax() + l.width() * (pos - l.ay()) / l.height();
                l.by() = pos;
            } else {
                pos = *loc.rbegin();
                l.bx() = l.ax() + l.width() * (l.ay() - pos) / l.height();
                l.by() = pos;
            }
        } else {
            if (axis == XAXIS) {
                pos = *loc.rbegin();
                l.ay() = l.by() - l.sign() * l.height() * (l.bx() - pos) / l.width();
                l.ax() = pos;
            } else if (l.sign() == 1) {
                pos = *loc.rbegin();
                l.ax() = l.bx() - l.width() * (l.by() - pos) / l.height();
                l.ay() = pos;
            } else {
                pos = *loc.begin();
                l.ax() = l.bx() - l.width() * (pos - l.by()) / l.height();
                l.ay() = pos;
            }
        }
    }
}

bool SpacePixel::read(std::istream &stream, int version) {
    // clear anything that was there:
    m_display_lines.clear();
    m_lines.clear();

    // read name:

    m_name = dXstring::readString(stream);
    stream.read((char *)&m_show, sizeof(m_show));

    if (m_name.empty()) {
        m_name = "<unknown>";
    }

    m_edit = false; // <- just default to not editable on read

    stream.read((char *)&m_color, sizeof(m_color));

    // read extents:
    stream.read((char *)&m_region, sizeof(m_region));

    // read rows / cols
    int rows, cols;
    stream.read(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.read(reinterpret_cast<char *>(&cols), sizeof(cols));
    m_rows = static_cast<size_t>(rows);
    m_cols = static_cast<size_t>(cols);

    // could work these two out on the fly, but it's easier to have them stored:
    // m_pixel_height = m_region.height() / double(m_rows);
    // m_pixel_width  = m_region.width()  / double(m_cols);

    // prepare loader:
    m_pixel_lines = depthmapX::RowMatrix<std::vector<int>>(static_cast<size_t>(m_rows), static_cast<size_t>(m_cols));

    stream.read((char *)&m_ref, sizeof(m_ref));
    dXreadwrite::readIntoMap(stream, m_lines);
    // now load into structure:
    int n = -1;
    for (auto line : m_lines) {
        n++;

        PixelRefVector list = pixelateLine(line.second.line);

        for (size_t m = 0; m < list.size(); m++) {
            // note: m_pixel_lines is an *ordered* list! --- used by other ops.
            m_pixel_lines(static_cast<size_t>(list[m].y), static_cast<size_t>(list[m].x)).push_back(n);
        }
    }

    return true;
}

bool SpacePixel::write(std::ofstream &stream) {
    // write name:
    dXstring::writeString(stream, m_name);
    stream.write((char *)&m_show, sizeof(m_show));
    stream.write((char *)&m_color, sizeof(m_color));

    // write extents:
    stream.write((char *)&m_region, sizeof(m_region));

    // write rows / cols
    int rows = static_cast<int>(m_rows);
    int cols = static_cast<int>(m_cols);
    stream.write(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.write(reinterpret_cast<char *>(&cols), sizeof(cols));

    // write lines:
    stream.write((char *)&m_ref, sizeof(m_ref));

    dXreadwrite::writeMap(stream, m_lines);

    return true;
}
