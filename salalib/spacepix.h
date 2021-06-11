// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2018, Petros Koutsolampros

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

#pragma once

#include "salalib/pafcolor.h"
#include "salalib/pixelref.h"

#include "genlib/p2dpoly.h"
#include "genlib/pafmath.h"
#include "genlib/simplematrix.h"
#include "genlib/stringutils.h"

#include <deque>
#include <map>

class SalaShape;

class PixelBase {
  protected:
    size_t m_rows;
    size_t m_cols;
    QtRegion m_region;

  public:
    PixelBase() { ; }
    // constrain is constrain to bounding box (i.e., in row / col bounds)
    virtual PixelRef pixelate(const Point2f &, bool constrain = true, int scalefactor = 1) const = 0;
    PixelRefVector pixelateLine(Line l, int scalefactor = 1) const;
    PixelRefVector pixelateLineTouching(Line l, double tolerance) const;
    PixelRefVector quickPixelateLine(PixelRef p, PixelRef q);
    bool includes(const PixelRef pix) const { return (pix.x >= 0 && pix.x < static_cast<short>(m_cols) &&
                                                      pix.y >= 0 && pix.y < static_cast<short>(m_rows)); }
    size_t getCols() const { return m_cols; }
    size_t getRows() const { return m_rows; }
    const QtRegion &getRegion() const { return m_region; }
};

/////////////////////////////////////////////

// couple of quick helpers

struct LineTest {
    Line line;
    unsigned int test;
    LineTest(const Line &l = Line(), int t = -1) {
        line = l;
        test = t;
    }
    // operator Line() {return line;}
};

struct LineKey {
    unsigned int file : 4;
    unsigned int layer : 6;
    unsigned int lineref : 20;
    operator int() { return *(int *)this; }
    LineKey(int value = 0) { *(int *)this = value; }
    friend bool operator<(LineKey a, LineKey b);
    friend bool operator>(LineKey a, LineKey b);
    friend bool operator==(LineKey a, LineKey b);
};
inline bool operator<(LineKey a, LineKey b) { return int(a) < int(b); }
inline bool operator>(LineKey a, LineKey b) { return int(a) > int(b); }
inline bool operator==(LineKey a, LineKey b) { return int(a) == int(b); }

/////////////////////////////////////////////

class SpacePixel : public PixelBase {
    friend class PointMap;
    friend class AxialMaps;
    friend class AxialPolygons;
    friend class ShapeMap; // for transfer to everything being ShapeMaps
  protected:
    bool m_lock;
    mutable bool m_newline;

  protected:
    PafColor m_color;
    int m_style; // allows for bold / dotted lines etc
    std::string m_name;
    bool m_show;
    bool m_edit;
    depthmapX::RowMatrix<std::vector<int>> m_pixel_lines;

    int m_ref;
    std::map<int, LineTest> m_lines;
    //
    // for screen drawing
    mutable std::vector<int> m_display_lines;
    mutable int m_current;
    //
    // for line testing
    mutable unsigned int m_test;
    //
  public:
    SpacePixel(const std::string &name = std::string("Default"));
    //
    SpacePixel(const SpacePixel &spacepixel);
    SpacePixel &operator=(const SpacePixel &spacepixel);
    void construct(const SpacePixel &spacepixel);
    //
    PixelRef pixelate(const Point2f &p, bool constrain = true, int = 1) const;
    //   PixelRefVector pixelate( const Line& l ) const;
    //
    void initLines(int size, const Point2f &min, const Point2f &max, double density = 1.0);
    void reinitLines(double density); // just reinitialises pixel lines, keeps lines, current ref and test setting
    //
    void addLine(const Line &l);
    void sortPixelLines();
    //
    int addLineDynamic(const Line &l);

    virtual void makeViewportLines(const QtRegion &viewport) const;
    virtual bool findNextLine(bool &) const;
    virtual const Line &getNextLine() const;
    //
    bool intersect(const Line &l, double tolerance = 1e-9);
    bool intersect_exclude(const Line &l, double tolerance = 1e-9);

    void cutLine(Line &l, short dir);

    QtRegion &getRegion() const { return (QtRegion &)m_region; }

    void setRegion(QtRegion &region) { m_region = region; }
    //
    const std::map<int, LineTest> &getAllLines() const // Danger! Use solely to look at the raw line data
    {
        return m_lines;
    }
    //
    // For easy layer manipulation:
    void setName(const std::string &name) { m_name = name; }
    std::string getName() { return m_name; }
    void setShow(bool show = true) { m_show = show; }
    bool isShown() const { return m_show; }
    void setEditable(bool edit = true) { m_edit = edit; }
    bool isEditable() const { return m_edit; }

  public:
    virtual bool read(std::istream &stream);
    virtual bool write(std::ofstream &stream);
    friend bool operator==(const SpacePixel &a, const SpacePixel &b);
};

// simply check they are the same name... useful for findindex from the group
inline bool operator==(const SpacePixel &a, const SpacePixel &b) { return a.m_name == b.m_name; }
