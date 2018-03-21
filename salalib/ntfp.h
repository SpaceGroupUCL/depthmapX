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



#ifndef __NTFP_H__
#define __NTFP_H__

struct NtfPoint {
   char m_chars;
   int a;
   int b;
   NtfPoint(char chars = 10)  // apparently 10 is NTF default
   { m_chars = chars; };
   int parse(const std::string& token, bool secondhalf = false);
};

class NtfGeometry : public prefvec<Line>
{
public:
   NtfGeometry() {;}
};

class NtfLayer : public prefvec<NtfGeometry> {
   friend class NtfMap;
protected:
   std::string m_name;
   int m_line_count;
public:
   NtfLayer(const std::string& name = std::string())
      { m_name = name; m_line_count = 0; };
   int getLineCount()
      { return m_line_count; }
   std::string getName()
      { return m_name; }
};

class NtfMap : public prefvec<NtfLayer>
{
public:
   enum {NTF_UNKNOWN, NTF_LANDLINE, NTF_MERIDIAN};
protected:
   NtfPoint m_offset;      // note: in metres
   QtRegion m_region;        // made in metres, although points are in cm
   int m_line_count;
public:
   NtfMap() {;}
   Line makeLine(const NtfPoint& a, const NtfPoint& b);
   
   void open(const std::vector<std::string> &fileset, Communicator *comm);
   const QtRegion& getRegion() const
   { return m_region; }
   int getLineCount() const
   { return m_line_count; }
protected:
   void fitBounds(const Line& li);
   void addGeom(int layer, NtfGeometry& geom);
};

#endif
