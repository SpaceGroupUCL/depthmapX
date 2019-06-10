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



#include "mapinfodata.h"
#include "salalib/mgraph.h"
#include "salalib/shapemap.h"
#include "salalib/axialmap.h"

#include <numeric>

int MapInfoData::import(std::istream& miffile, std::istream& midfile, ShapeMap& map)
{
   int mapLoaded = MINFO_OK;

   // read the header...
   if (!readheader(miffile)) {
      return MINFO_HEADER;
   }

   std::vector<std::string> columnheads;

   AttributeTable& attributes = map.getAttributeTable();

   // read mif table
   if (!readcolumnheaders(miffile, columnheads)) {
      return MINFO_TABLE;
   }

   // set up a list of readable columns from the headers:
   // 
   std::vector<std::string> colnames;
   std::vector<size_t> readable, colindexes;
   for (size_t i = 0; i < columnheads.size(); i++) {
      dXstring::ltrim(columnheads[i]);
      auto tokens = dXstring::split(columnheads[i], ' ',true);
      if (dXstring::beginsWith<std::string>(tokens[1],"Integer")
              || dXstring::beginsWith<std::string>(tokens[1],"Smallint")
              || dXstring::beginsWith<std::string>(tokens[1],"Decimal")
              || dXstring::beginsWith<std::string>(tokens[1],"Float"))
      {
         colnames.push_back(tokens[0]);
         attributes.insertOrResetColumn(colnames.back());
         readable.push_back(i);
      }
   }

   for (std::string colname: colnames) {
      colindexes.push_back(attributes.getColumnIndex(colname));
   }

   std::string textline;
   std::vector<std::vector<Point2f>> pointsets;
   std::vector<size_t> duplicates;
   std::vector<int> types;

   try {
   // now read line data into the axial map   
   while (!miffile.eof()) {
      dXstring::safeGetline(miffile, textline);
      dXstring::ltrim(textline);
      dXstring::toLower(textline);
      if (textline.empty()) {
         continue;
      }
      if (dXstring::beginsWith<std::string>(textline,"point")) {
         auto tokens = dXstring::split(textline,' ',true);
         pointsets.push_back(std::vector<Point2f>());
         types.push_back(SalaShape::SHAPE_POINT);
         pointsets.back().push_back(Point2f(stod(tokens[1]),stod(tokens[2])));
      }
      if (dXstring::beginsWith<std::string>(textline,"line")) {
         auto tokens = dXstring::split(textline,' ',true);
         pointsets.push_back(std::vector<Point2f>());
         types.push_back(SalaShape::SHAPE_LINE);
         pointsets.back().push_back(Point2f(stod(tokens[1]),stod(tokens[2])));
         pointsets.back().push_back(Point2f(stod(tokens[3]),stod(tokens[4])));
      }
      else if (dXstring::beginsWith<std::string>(textline,"pline") || dXstring::beginsWith<std::string>(textline,"region")) {
         int type = dXstring::beginsWith<std::string>(textline,"pline") ? SalaShape::SHAPE_POLY : (SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED);
         // note: polylines, even multiple lines, are condensed into a single line
         auto tokens = dXstring::split(textline,' ',true);
         int multiple = 1;
         if (tokens.size() > 1) {
            if (tokens[1] == "multiple") {
               multiple = stoi(tokens[2]);
            }
            else if (type & SalaShape::SHAPE_CLOSED) {
               multiple = stoi(tokens[1]);
            }
            // if for some reason c_int fails:
            if (multiple == 0) {
               multiple = 1;
            }
         }
         for (int i = 0; i < multiple; i++) {
            int count = -1;
            if ((type & SalaShape::SHAPE_CLOSED) != SalaShape::SHAPE_CLOSED && tokens.size() == 2) {
               // token 2 can apparently be used for count in pline rather than a newline being used...
               count = stoi(tokens[1]);
            }
            else {
               dXstring::safeGetline(miffile, textline);
               dXstring::ltrim(textline);
               count = stoi(textline);
            }
            pointsets.push_back(std::vector<Point2f>());
            types.push_back(type);
            for (int j = 0; j < count; j++) {
               dXstring::safeGetline(miffile, textline);
               dXstring::ltrim(textline);
               auto tokens = dXstring::split(textline,' ',true);
               pointsets.back().push_back(Point2f(stod(tokens[0]),stod(tokens[1])));
            }
            if (i != 0) {
               // warn about extraneous pline data
               mapLoaded = MINFO_MULTIPLE;
               duplicates.push_back(pointsets.size() - 1);
            }
         }
      }
   }
   }
   // TODO: Use the proper exception
   catch (std::exception) {
      // unhandled parsing exceptions return read error:
      return MINFO_MIFPARSE;
   }

   size_t nextduplicate = 0;
   AttributeRow *lastrow;

   QtRegion region(pointsets[0][0],pointsets[0][0]);
   for (size_t i = 0; i < pointsets.size(); i++) {
      for (size_t j = 0; j < pointsets[i].size(); j++) {
         region.encompass(pointsets[i][j]);
      }
   }

   try {
     // switch lines into our format
     map.init(pointsets.size(),region);
     for (size_t i = 0; i < pointsets.size(); i++) {
         bool open = false;
         if ((types[i] & SalaShape::SHAPE_CLOSED) == 0) {
            open = true;
         }
         map.makePolyShape(pointsets[i],open);
         AttributeRow &row = *attributes.back().second;
         //
         // table data entries:
         if (nextduplicate < duplicates.size() && duplicates[nextduplicate] == i) {
            // duplicate last row:
            for (int colindex: colindexes) {
               row.setValue(colindex, lastrow->getValue(colindex));
            }
            nextduplicate++;
         }
         else {
            // read next row:
            std::string line;
            while (!midfile.eof() && line.empty()) {
               dXstring::safeGetline(midfile, line);
            }
            if (line.empty()) {
               return MINFO_OBJROWS;
            }
            bool instring = false;
            size_t here = 0, first = 0, reading = 0, nextreadable = 0;
            while (nextreadable < readable.size()) {
               char next = line[here];
               if (next == '\"') {
                  instring = !instring;
               }
               here++;
               if ((!instring && next == m_delimiter) || here >= line.length()) {
                  int length = (here < line.length()) ? here-first-1 : here-first;
                  std::string field = line.substr(first,length);
                  first = here;
                  if (reading == readable[nextreadable]) {
                     float val = stof(field);
                     row.setValue(colindexes[nextreadable],val);
                     nextreadable++;
                  }
                  reading++;
               }
            }
         }
         lastrow = &row;
      }
   }
   // TODO: use a proper exception
   catch (std::exception) {
      // unhandled parsing exceptions return read error:
      return MINFO_TABLE;
   }

   return mapLoaded;
}
/*
bool MapInfoData::exportFile(std::ostream& miffile, std::ostream& midfile, const ShapeGraph& map)
{
   // if bounds has not been filled in, fill it in
   if (m_bounds.empty()) {
      char bounds[256];
      sprintf(bounds,"Bounds (%10f, %10f) (%10f, %10f)", map.m_region.bottom_left.x, 
                                                         map.m_region.bottom_left.y,
                                                         map.m_region.top_right.x,
                                                         map.m_region.top_right.y);
      m_bounds = bounds;
   }

   // write the header...
   writeheader(miffile);

   // write the mif table
   writetable(miffile,midfile,map.m_attributes);

   miffile.precision(16);

   for (int i = 0; i < map.m_lines.size(); i++) {
      miffile << "Line " << map.m_lines[i].line.start().x << " " 
                         << map.m_lines[i].line.start().y << " " 
                         << map.m_lines[i].line.end().x << " " 
                         << map.m_lines[i].line.end().y << std::endl;
      miffile << "    Pen (1,2,0)" << std::endl;
   }

   return true;
}
*/
bool MapInfoData::exportFile(std::ostream& miffile, std::ostream& midfile, const PointMap& points)
{
   // if bounds has not been filled in, fill it in
   if (m_bounds.empty()) {
      char bounds[256];
      sprintf(bounds,"Bounds (%10f, %10f) (%10f, %10f)", points.m_region.bottom_left.x, 
                                                         points.m_region.bottom_left.y,
                                                         points.m_region.top_right.x,
                                                         points.m_region.top_right.y);
      m_bounds = bounds;
   }

   // write the header...
   writeheader(miffile);
      
   // write the mif table
   writetable(miffile, midfile, points.getAttributeTable(), points.m_layers);

   miffile.precision(16);

   for (auto iter = points.getAttributeTable().begin(); iter != points.getAttributeTable().end(); iter++) {
      PixelRef pix = iter->getKey().value;
      Point2f p = points.depixelate(pix);
      miffile << "Point " << p.x << " " << p.y << std::endl;
      miffile << "    Symbol (32,0,10)" << std::endl;
   }

   return true;
}

bool MapInfoData::exportFile(std::ostream& miffile, std::ostream& midfile, const ShapeMap& map)
{
   // if bounds has not been filled in, fill it in
   if (m_bounds.empty()) {
      char bounds[256];
      sprintf(bounds,"Bounds (%10f, %10f) (%10f, %10f)", map.getRegion().bottom_left.x,
                                                         map.getRegion().bottom_left.y,
                                                         map.getRegion().top_right.x,
                                                         map.getRegion().top_right.y);
      m_bounds = bounds;
   }

   miffile.precision(8);
   midfile.precision(8);

   // write the header...
   writeheader(miffile);

   // write the mid table
   writetable(miffile, midfile, *map.m_attributes, map.m_layers);

   miffile.precision(16);
   midfile.precision(16);

   for (auto shape: map.m_shapes) {
      // note, attributes must align for this:
      if (isObjectVisible(map.m_layers, map.getAttributeTable().getRow(AttributeKey(shape.first)))) {
         const SalaShape& poly = shape.second;
         if (poly.isPoint()) {
            miffile << "POINT " << poly.getPoint().x << " " << poly.getPoint().y << std::endl;
            miffile << "    SYMBOL (32,0,10)" << std::endl;
         }
         else if (poly.isLine()) {
            miffile << "LINE " << poly.getLine().start().x << " " 
                               << poly.getLine().start().y << " " 
                               << poly.getLine().end().x << " " 
                               << poly.getLine().end().y << std::endl;
            miffile << "    PEN (1,2,0)" << std::endl;
         }
         else if (poly.isPolyLine()) {
            miffile << "PLINE" << std::endl;
            miffile << "  " << poly.m_points.size() << std::endl;
            for (auto& point: poly.m_points) {
               miffile << point.x << " " << point.y << std::endl;
            }
            miffile << "    PEN (1,2,0)" << std::endl;
         }
         else if (poly.isPolygon()) {
            miffile << "REGION  1" << std::endl;
            miffile << "  " << poly.m_points.size() + 1 << std::endl;
            for (auto& point: poly.m_points) {
               miffile << point.x << " " << point.y << std::endl;
            }
            miffile << poly.m_points[0].x << " " << poly.m_points[0].y << std::endl;
            miffile << "    PEN (1,2,0)" << std::endl;
            miffile << "    BRUSH (2,16777215,16777215)" << std::endl;
            miffile << "    CENTER " << poly.getCentroid().x << " " << poly.getCentroid().y << std::endl;
         }
      }
   }

   return true;
}

bool MapInfoData::exportPolygons(std::ostream& miffile, std::ostream& midfile, const std::vector<std::vector<Point2f>>& polygons, const QtRegion& region)
{
   // if bounds has not been filled in, fill it in
   if (m_bounds.empty()) {
      char bounds[256];
      sprintf(bounds,"Bounds (%10f, %10f) (%10f, %10f)", region.bottom_left.x, 
                                                         region.bottom_left.y,
                                                         region.top_right.x,
                                                         region.top_right.y);
      m_bounds = bounds;
   }

   // write the header...
   writeheader(miffile);

   // dummy attributes table:
   AttributeTable attributes;
   for (size_t i = 0; i < polygons.size(); i++) {
      attributes.addRow(AttributeKey(i));
   }

   // dummy layers:
   LayerManagerImpl layers;

   // write the mid table
   writetable(miffile, midfile, attributes, layers);

   miffile.precision(16);
   for (auto& polygon: polygons) {
      Point2f centre;
      miffile << "QtRegion  1" << std::endl;
      miffile << "  " << polygon.size() + 1 << std::endl;
      for (auto& point: polygon) {
         centre += point;
         miffile << point.x << " " << point.y << std::endl;
      }
      miffile << polygon[0].x << " " << polygon[0].y << std::endl;
      miffile << "    Pen (1,2,0)" << std::endl;
      miffile << "    Brush (2,16777215,16777215)" << std::endl;
      centre /= polygon.size();
      miffile << "    Center " << centre.x << " " << centre.y << std::endl;
   }

   return true;
}


///////////////////////////////////////////////////////////////////////

MapInfoData::MapInfoData()
{
   m_version = "Version 300";
   m_charset = "Charset \"WindowsLatin1\"";
   m_delimiter = ',';
   m_index = "Index 1";
   m_coordsys = "CoordSys NonEarth Units \"m\" ";
   // note: m_bounds is filled in later
}

bool MapInfoData::readheader(std::istream& miffile)
{
   std::string line;

   dXstring::safeGetline(miffile, m_version);
   dXstring::safeGetline(miffile, m_charset);
   dXstring::makeInitCaps(m_charset);
   // this should read "Charset..." but some files have delimiter straight away...
   if (dXstring::beginsWith<std::string>(m_charset,"Delimiter")) {
      line = m_charset;
      m_charset = "Charset \"WindowsLatin1\"";
   }
   else {
      dXstring::safeGetline(miffile, line);
   }
   size_t index = line.find_first_of("\"");
   if (index == std::string::npos) {
      return false;
   }
   m_delimiter = line[index+1];
   dXstring::safeGetline(miffile, line);
   dXstring::makeInitCaps(line);
   while (dXstring::beginsWith<std::string>(line,"Index") || dXstring::beginsWith<std::string>(line,"Unique")) {
      m_index = line;
      dXstring::safeGetline(miffile, line);
   }

   dXstring::ltrim(line);
   dXstring::makeInitCaps(line);
   if (dXstring::beginsWith<std::string>(line,"Coordsys")) {
      line[5] = 'S'; // set back to CoordSys
      // coordsys and bounds together in one line
      auto boundIndex = line.find("Bounds");
      if(boundIndex != std::string::npos) {
          m_coordsys = line.substr(0,boundIndex);
          m_bounds = line.substr(boundIndex);
      } else {
          m_coordsys = line;
          m_bounds = "";
      }
   }
   else {
      return false;
   }

   return true;
}

bool MapInfoData::readcolumnheaders(std::istream& miffile, std::vector<std::string>& columnheads)
{
   std::string line;

   dXstring::safeGetline(miffile, line);
   dXstring::makeInitCaps(line);
   auto bits = dXstring::split(line, ' ');

   if (line.find("Columns") == std::string::npos || bits.size() < 2 )
   {
      return false;
   }
   int cols = stoi(bits[1]);

   for (int i = 0; i < cols; i++) {
      dXstring::safeGetline(miffile, line);
      dXstring::makeInitCaps(line);
      columnheads.push_back(line);
   }

   dXstring::safeGetline(miffile, line);
   dXstring::makeInitCaps(line);
   if (line != "Data") {
      return false;
   }

   return true;
}

void MapInfoData::writeheader(std::ostream& miffile)
{
   miffile << m_version << std::endl;
   miffile << m_charset << std::endl;
   miffile << "Delimiter \"" << m_delimiter << "\"" << std::endl;
   miffile << m_index << std::endl;
   miffile << m_coordsys;
   miffile << m_bounds << std::endl;
}

// note: stopped using m_table and m_columnheads as of VERSION_MAPINFO_SHAPES
// simply hack up the table now for own purposes

void MapInfoData::writetable(std::ostream& miffile, std::ostream& midfile, const AttributeTable& attributes, const LayerManagerImpl layers)
{
   miffile << "Columns " << attributes.getNumColumns() + 1 << std::endl;
   /*
   miffile << "Columns " << m_columnheads.size() + 1 + attributes.getColumnCount() << std::endl;

   for (int i = 0; i < m_columnheads.size(); i++) {
      miffile << m_columnheads[i] << std::endl;
   }
   */
   miffile << "  Depthmap_Ref Integer" << std::endl;

   // TODO: For compatibility write the columns in alphabetical order
   // but the physical columns in the order inserted

   std::vector<size_t> indices(attributes.getNumColumns());
   std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

   std::sort(indices.begin(), indices.end(),
       [&](size_t a, size_t b) {
       return attributes.getColumnName(a) < attributes.getColumnName(b);
   });

   for (int idx: indices) {
      std::string colname = attributes.getColumnName(idx);
      miffile << "  ";
      bool lastalpha = false;
      for (size_t i = 0; i < colname.length(); i++) {
         // get rid of any character that's not alphanumeric:
         if (isalnum(colname[i])) {
            miffile << colname[i];
            lastalpha = true;
         }
         else if (lastalpha) {
            miffile << "_";
            lastalpha = false;
         }
      }
      miffile << " Float" << std::endl;
   }

   miffile << "Data" << std::endl << std::endl;

   for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
       int rowKey = iter->getKey().value;
      /*
      if (k < m_table.size()) {
         midfile << m_table[k] << m_delimiter;
      }
      */
      if (isObjectVisible(layers, iter->getRow())) {
         midfile << rowKey;
         for (int idx: indices) {
             midfile << m_delimiter << iter->getRow().getValue(idx);
         }
         midfile << std::endl;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

std::istream& MapInfoData::read(std::istream& stream)
{
   m_version = dXstring::readString(stream);
   m_charset = dXstring::readString(stream);
   m_delimiter = stream.get();
   m_index = dXstring::readString(stream);
   m_coordsys = dXstring::readString(stream);
   m_bounds = dXstring::readString(stream);
   
   return stream;
}

std::ostream& MapInfoData::write(std::ostream& stream)
{
   dXstring::writeString(stream, m_version);
   dXstring::writeString(stream, m_charset);
   stream.put(m_delimiter);
   dXstring::writeString(stream, m_index);
   dXstring::writeString(stream, m_coordsys);
   dXstring::writeString(stream, m_bounds);
   /*
   // No longer used as of VERSION_MAPINFO_SHAPES
   int columns = m_columnheads.size();
   int rows = m_table.size();
   stream.write((char *)&columns, sizeof(columns));
   for (int i = 0; i < m_columnheads.size(); i++) {
      m_columnheads[i].write(stream);
   }
   stream.write((char *)&rows, sizeof(rows));
   for (int j = 0; j < m_table.size(); j++) {
      m_table[j].write(stream);
   }
   */
   return stream;
}
