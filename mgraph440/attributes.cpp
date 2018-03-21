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


#include <math.h>
#include <float.h>

#include <mgraph440/mgraph.h>
#include <mgraph440/attributes.h>

#include "mgraph440/stringutils.h"

#include <stdexcept>

namespace mgraph440 {

////////////////////////////////////////////////////////////////////////////////////

// helpers: local sorting routines

int compareValuePair(const void *p1, const void *p2)
{
   double v = (((ValuePair *)p1)->value - ((ValuePair *)p2)->value);
   return (v > 0.0 ? 1 : v < 0.0 ? -1 : 0);
}

////////////////////////////////////////////////////////////////////////////////////

AttributeTable::AttributeTable(const std::string& name)
{
   // need memory initialised somewhere for these...
   g_ref_number_name = std::string("Ref Number");
   g_ref_number_formula = std::string();
   //
   m_name = name;
   // initially not showing any column:
   m_display_column = -2;
   // initially no selection:
   m_sel_count = 0;
   m_sel_value = 0.0;
   //
   // everything apart from the default layer is available for use:
   // Quick mod - TV
   m_available_layers = 0xffffffff << 32 + 0xfffffffe;
   // display the default layer only (everything):
   m_visible_layers = 0x1;
   m_layers.add(1,"Everything");
   m_visible_size = 0;
}

int AttributeTable::insertColumn(const std::string& name)
{
   size_t index = m_columns.searchindex(AttributeColumn(name));
   if (index != paftl::npos) {
      m_columns[index].reset();
      int phys_col = m_columns[index].m_physical_col;
      for (size_t i = 0; i < size(); i++) {
         at(i)[phys_col] = -1.0f;
      }
   }
   else {
      index = m_columns.add(AttributeColumn(name));
      for (size_t i = 0; i < size(); i++) {
         at(i).push_back(-1.0f);
      }
      m_columns[index].m_physical_col = m_columns.size() - 1;
   }
   return index;
}

int AttributeTable::insertRow(int key)
{
   int index = add(key,AttributeRow());
   value(index).init(m_columns.size());
   return index;
}

void AttributeTable::deselectAll() const
{
   m_sel_count = 0;
   m_sel_value = 0.0;
   for (size_t i = 0; i < size(); i++) {
      value(i).m_selected = false;
   }
}

//////////////////////////////////////////////////////////////////////////////////////

void AttributeTable::setDisplayColumn(int col, bool override) const
{
   if (col != m_display_column || override) {
      if (col != -2) {
         m_sel_value = 0.0; // reset selection total for new column
         // note, visible size is actually picked up by the display index
         m_visible_size = m_display_index.makeIndex(*this, col, true);
         if (col == -1)
            m_display_params = m_ref_display_params;
         else
            m_display_params = m_columns[col].getDisplayParams();
      }
      m_display_column = col;
   }
}

//////////////////////////////////////////////////////////////////////////////////////

bool AttributeTable::read( ifstream& stream, int version )
{
   if (version >= VERSION_MAP_LAYERS) {
      m_layers.clear();
      stream.read((char *)&m_available_layers,sizeof(int64));
      stream.read((char *)&m_visible_layers,sizeof(int64));
      int count;
      stream.read((char *)&count,sizeof(int));
      for (int i = 0; i < count; i++) {
         int64 key;
         stream.read((char *)&key,sizeof(key));
         m_layers.add(key,dXstring440::readString(stream));
      }
   }
   int colcount;
   stream.read((char *)&colcount, sizeof(colcount));
   for (int j = 0; j < colcount; j++) {
      m_columns.push_back(AttributeColumn());
      m_columns.tail().read(stream, version);
      // this may need a bit of reordering, as the reader can chop up names:
      m_columns.sort();
   }
   int rowcount, rowkey;
   stream.read((char *)&rowcount, sizeof(rowcount));
   for (int i = 0; i < rowcount; i++) {
      stream.read((char *)&rowkey, sizeof(rowkey));
      int index = add(rowkey,AttributeRow());
      if (version >= VERSION_MAP_LAYERS) {
         stream.read((char *)&(value(index).m_layers),sizeof(int64));
      }
      value(index).read(stream);
   }
   if (version >= VERSION_GATE_MAPS) {
      // ref column display params
      stream.read((char *)&m_display_params,sizeof(m_display_params));
   }
   return true;
}

////////////////////////////////////////////////////////////////////////

void AttributeRow::init(size_t length)
{
   if (m_data) {
      delete [] m_data;
      m_data = NULL;
   }
   while (length >= storage_size())
      m_shift++;
   m_data = new float [storage_size()];
   m_length = length;

   for (size_t i = 0; i < m_length; i++) {
      at(i) = -1.0;
   }
}

////////////////////////////////////////////////////////////////////////

void AttributeColumn::reset()
{
   m_min = -1.0;
   m_max = 0.0;
   m_tot = 0.0;
   m_visible_min = -1.0;
   m_visible_max = 0.0;
   m_visible_tot = 0.0;
}

bool AttributeColumn::read( ifstream& stream, int version )
{
   m_updated = false;
   m_name = dXstring440::readString(stream);
   float min, max;
   stream.read((char *)&min, sizeof(min));
   stream.read((char *)&max, sizeof(max));
   m_min = min;
   m_max = max;
   if (version >= VERSION_ATTRIBUTES_TABLE)  // m_tot has always been a double
      stream.read((char *)&m_tot, sizeof(m_tot));
   else
      m_tot = 0.0;
   stream.read((char *)&m_physical_col, sizeof(m_physical_col));
   stream.read((char *)&m_hidden, sizeof(m_hidden));
   if (version >= VERSION_ATTRIBUTE_LOCKING) {
      stream.read((char *)&m_locked, sizeof(m_locked));
   }
   else {
      if (m_name == "Connectivity" || m_name == "Connectivity (Degree)" || m_name == "Axial Line Ref" || m_name == "Segment Length" || m_name == "Line Length") {
         m_locked = true;
      }
      else {
         m_locked = false;
      }
   }
   if (version >= VERSION_STORE_COLOR) {
      stream.read((char*)&m_display_params,sizeof(m_display_params));
   }
   if (version >= VERSION_STORE_FORMULA) {
      m_formula = dXstring440::readString(stream);
   }
   if (version >= VERSION_STORE_COLUMN_CREATOR && version < VERSION_FORGET_COLUMN_CREATOR) {
      std::string dummy_creator = dXstring440::readString(stream);
   }
   return true;
}

////////////////////////////////////////////////////////////////////////

AttributeIndex::AttributeIndex()
{
   m_col = -1;
   m_data = NULL;
}

void AttributeIndex::clear()
{
   m_col = -1;
   pvector<ValuePair>::clear();
}

int AttributeIndex::makeIndex(const AttributeTable& table, int col, bool setdisplayinfo)
{
   // clear contents:
   clear();

   // local copy since this will be reused a lot
   size_t rowcount = table.getRowCount();

   // preallocate vector:
   while (rowcount >= storage_size())
      m_shift++;
   m_data = new ValuePair[storage_size()];
   m_length = rowcount;
   //
   m_col = col;
   //
   double min = -1.0f, max = -1.0f, vismin = -1.0f, vismax = -1.0f;
   double total = 0.0f, vistotal = 0.0;
   // note that, for safety, *everything* is always indexed,
   // viscount is simply a count of everything that is visible
   int viscount = 0;
   // n.b., attributes, axial lines and line refs must match
   size_t i;
   for (i = 0; i < rowcount; i++)
   {
      at(i).index = i;
      if (col != -1) {
         at(i).value = double(table.getValue(i,col));
         if (at(i).value != -1) {
            if (min == -1.0f || at(i).value < min) {
               min = (double) at(i).value;
            }
            if (max == -1.0f || at(i).value > max) {
               max = (double) at(i).value;
            }
            total += at(i).value;
            if (table.isVisible(i)) {
               // note, this may be useful -- the visible count does not include nulls
               viscount++;
               if (vismin == -1.0f || at(i).value < vismin) {
                  vismin = (double) at(i).value;
               }
               if (vismax == -1.0f || at(i).value > vismax) {
                  vismax = (double) at(i).value;
               }
               vistotal += at(i).value;
            }
         }
         // note: qsort is slow when many values are the same -- so these values are perturbed
         // -> perturbation used to be random, but now sub sort by ref number
         // note: value needs to be double to work out in large tables
         // (note also, max may build up through table, causing some disturbance to order)
         at(i).value += (max * 1e-9 * double(i)) / table.getRowCount();
      }
      else {
         if (table.isVisible(i)) {
            // note, viscount is used by scatterplots at least
            viscount++;
            // eventually there should be a colour override on the ref column as well as any other)
            vismax = i;
            if (vismin == -1) {
               vismin = i;
            }
         }
         at(i).value = double(table.getRowKey(i))/table.getMaxRowKey();
      }
   }

   // mutable override:
   if (col != -1) {
      table.setColumnInfo(col,min,max,total,vismin,vismax,vistotal);
   }

   qsort(m_data,rowcount,sizeof(ValuePair),compareValuePair);

   for (i = 0; i < rowcount; i++) {
      // note: this is to ensure we have save settings for the table ranges where data has been overwritten:
      if (setdisplayinfo) {
         at(i).value = (col != -1) ? table.getNormValue(at(i).index,col) : double(table.getRowKey(at(i).index))/table.getMaxRowKey();
         // be able to lookup index pos from row:
         ValuePair vp2;
         vp2.index = i;
         vp2.value = at(i).value;
         table.setDisplayInfo(at(i).index,vp2);
      }
      else {
         // don't normalise: you want the exact value for this row
         at(i).value = (col != -1) ? table.getValue(at(i).index,col) : double(table.getRowKey(at(i).index));
      }
   }
   return viscount;
}

bool AttributeTable::write( ostream& stream, int version )
{

   stream.write((char *)&m_available_layers,sizeof(int64));
   stream.write((char *)&m_visible_layers,sizeof(int64));
   int count = m_layers.size();
   stream.write((char *)&count,sizeof(int));
   for (size_t i = 0; i < m_layers.size(); i++) {
      int64 key = m_layers.key(i);
      stream.write((char *)&key,sizeof(key));
      dXstring440::writeString(stream ,m_layers.value(i));
   }

   int colcount = m_columns.size();
   stream.write((char *)&colcount, sizeof(colcount));
   for (int j = 0; j < colcount; j++) {
      m_columns[j].write(stream,version);
   }
   int rowcount = size(), rowkey;
   stream.write((char *)&rowcount, sizeof(rowcount));
   for (int i = 0; i < rowcount; i++) {
      rowkey = key(i);
      stream.write((char *)&rowkey, sizeof(rowkey));
      stream.write((char *)&(value(i).m_layers),sizeof(int64));
      value(i).write(stream);
   }
   // ref column display params
   stream.write((char *)&m_display_params,sizeof(m_display_params));
   return true;
}

bool AttributeColumn::write( ostream& stream, int version )
{
   m_updated = false;
   dXstring440::writeString(stream, m_name);
   float min = (float) m_min;
   float max = (float) m_max;
   stream.write((char *)&min, sizeof(min));
   stream.write((char *)&max, sizeof(max));
   stream.write((char *)&m_tot, sizeof(m_tot));
   stream.write((char *)&m_physical_col, sizeof(m_physical_col));
   stream.write((char *)&m_hidden, sizeof(m_hidden));
   stream.write((char *)&m_locked, sizeof(m_locked));
   stream.write((char *)&m_display_params,sizeof(m_display_params));
   dXstring440::writeString(stream, m_formula);
   return true;
}

}
