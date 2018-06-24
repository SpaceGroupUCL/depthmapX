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

#include <salalib/mgraph.h>
#include <salalib/attributes.h>

#include "genlib/stringutils.h"

#include <stdexcept>
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

void AttributeTable::removeColumn(int col)
{
   int phys_col = m_columns[col].m_physical_col;
   // remove data:
   for (size_t i = 0; i < size(); i++) {
      at(i).remove_at(phys_col);
   }
   // remove column head:
   m_columns.remove_at(col);
   // adjust other columns:
   for (size_t j = 0; j < m_columns.size(); j++) {
      if (m_columns[j].m_physical_col > phys_col) {
         m_columns[j].m_physical_col -= 1;
      }
   }
   // done
}


// note: returns new column id and may reorder the name columns
int AttributeTable::renameColumn(int col, const std::string& name)
{
   size_t index = m_columns.searchindex(name);
   if (index == col) {
      // no change in name
      return col;
   }
   else if (index != paftl::npos) {
      // column name already exists!
      return -1;
   }
   // a little slow, but it shouldn't happen that often:
   // switch round the column names and re-add.
   // Copy column exactly (with same physical_col id)
   AttributeColumn newcolumn(m_columns[col]);
   m_columns.remove_at(col);
   newcolumn.m_name = name;
   index = m_columns.add(newcolumn);
   // you will now have to alter the displayed attribute accordingly...
   return index;
}

int AttributeTable::insertRow(int key)
{
   int index = add(key,AttributeRow());
   value(index).init(m_columns.size());
   return index;
}

void AttributeTable::removeRow(int key)
{
   size_t index = searchindex(key);
   if (index != paftl::npos) {
      remove_at(index);
   }
}

void AttributeTable::setColumnValue(int col, float val)
{
   int phys_col = m_columns[col].m_physical_col;
   m_columns[col].m_tot = 0.0;
   m_columns[col].m_min = val;
   m_columns[col].m_max = val;
   for (size_t i = 0; i < size(); i++) {
      value(i).at(phys_col) = val;
      m_columns[col].m_tot += val;
   }
}

//////////////////////////////////////////////////////////////////////////////////////

// selection feature:

bool AttributeTable::selectRowByKey(int key) const
{
   size_t index = searchindex(key);
   if (index != paftl::npos) {
      if ((m_visible_layers & value(index).m_layers) != 0 && !value(index).m_selected) {
         value(index).m_selected = true;
         m_sel_count++;
         addSelValue(getValue(index,m_display_column));
      }
      else {
         // already selected or not visible
         index = -1;
      }
   }
   return index != -1;
}

bool AttributeTable::selectRowByIndex(int index) const
{
   if (index != -1) {
      if ((m_visible_layers & value(index).m_layers) != 0 && !value(index).m_selected) {
         value(index).m_selected = true;
         m_sel_count++;
         addSelValue(getValue(index,m_display_column));
      }
      else {
         // already selected
         index = -1;
      }
   }
   return index != -1;
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

// display features:

void AttributeTable::setDisplayParams(const DisplayParams& dp)
{
   m_ref_display_params = dp; 
   for (unsigned int i = 0; i < m_columns.size(); i++) {
      m_columns[i].setDisplayParams(dp); 
   }      
   m_display_params = dp;
}

void AttributeTable::setDisplayParams(int col, const DisplayParams& dp)
{
   if (col != -1) 
      m_columns[col].setDisplayParams(dp); 
   else 
      m_ref_display_params = dp; 

   if (col == m_display_column) {
      m_display_params = dp;
   } 
}

void AttributeTable::setVisibleLayers(int64 layers, bool override)
{
   if (layers != m_visible_layers || override) {
      m_visible_layers = layers;
      // will need to recalculate display information
      setDisplayColumn(m_display_column,true);
   }
}

void AttributeTable::setLayerVisible(int layer, bool show)
{
   int64 showlayers = 0;
   int64 key = m_layers.key(layer);
   bool on = (key & m_visible_layers) != 0;
   if (key == 0x1) {
      if (show && !on) {
         // show just the everything layer
         showlayers = 0x1;
      }
      else if (!show && on) {
         // turn everything off
         showlayers = 0x0;
      }
   }
   else {
      if (show && !on) {
         // turn on this layer, but also turn off "everything" layer is it's displayed
         showlayers = (key | m_visible_layers) & (~0x1);
      }
      if (!show && on) {
         // simply turn this individual layer off
         showlayers = m_visible_layers & (~key);
      }
   }
   setVisibleLayers(showlayers);
}

bool AttributeTable::selectionToLayer(const std::string& name)
{
   // this slight messing selects the next available layer
   int loc = 1;
   while (loc < 64 && ((m_available_layers>>loc) & 0x1) == 0) {
      loc++;
   }
   if (loc == 64) {
      // too many layers -- maximum 64
      return false;
   }
   int64 newlayer = 0x1 << loc;
   // now layer has been found, eliminate from available layers 
   // and add a lookup for the name
   m_available_layers = (m_available_layers & (~newlayer));
   m_layers.add(newlayer,name);

   // convert everything in the selection to the new layer
   for (size_t i = 0; i < size(); i++) {
      if (isVisible(i) && isSelected(i)) {
         at(i).m_layers |= newlayer;
      }
   }

   // set so just this layer is displayed...
   setVisibleLayers(newlayer);

   return true;
}

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

bool AttributeTable::read( std::istream& stream, int version )
{

   m_layers.clear();
   stream.read((char *)&m_available_layers,sizeof(int64));
   stream.read((char *)&m_visible_layers,sizeof(int64));
   int count;
   stream.read((char *)&count,sizeof(int));
   for (int i = 0; i < count; i++) {
       int64 key;
       stream.read((char *)&key,sizeof(key));
       m_layers.add(key,dXstring::readString(stream));
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

      stream.read((char *)&(value(index).m_layers),sizeof(int64));

      value(index).read(stream);
   }

   // ref column display params
   stream.read((char *)&m_display_params,sizeof(m_display_params));

   return true;
}

bool AttributeTable::write( std::ofstream& stream, int version )
{

   stream.write((char *)&m_available_layers,sizeof(int64));
   stream.write((char *)&m_visible_layers,sizeof(int64));
   int count = m_layers.size();
   stream.write((char *)&count,sizeof(int));
   for (size_t i = 0; i < m_layers.size(); i++) {
      int64 key = m_layers.key(i);
      stream.write((char *)&key,sizeof(key));
      dXstring::writeString(stream ,m_layers.value(i));
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

bool AttributeTable::outputHeader( std::ostream& stream, char delimiter, bool updated_only ) const
{
   for (size_t i = 0; i < m_columns.size(); i++) {
      if (!updated_only || m_columns[i].m_updated) {
         stream << delimiter << m_columns[i].m_name;
      }
   }
   stream << std::endl;

   return true;
}

bool AttributeTable::outputRow( int row, std::ostream& stream, char delim, bool updated_only ) const
{
   int prec = stream.precision(8);

   for (size_t i = 0; i < m_columns.size(); i++) {
      if (!updated_only || m_columns[i].m_updated) {
         stream << delim << value(row).at(m_columns[i].m_physical_col);
      }
   }
   stream << std::endl;

   stream.precision(prec);

   return true;
}

// note, export is a keyword, so use exportTable as function name,
// similar convention for importTable
bool AttributeTable::exportTable(std::ostream& stream, bool updated_only)
{
   stream << "Ref";
   outputHeader(stream,'\t',updated_only);
   for (int i = 0; i < getRowCount(); i++) {
      if (isVisible(i)) {
         stream << getRowKey(i);
         outputRow(i,stream,'\t',updated_only);
      }
   }
   return true;
}

// From UrbanBuzz I-VALUL project (c) SSL licensed to UCL (written by Alasdair)
// import values imports columns into an attribute tables 
// uses ref numbers, does not overwrite geom locations
// if "merge" is selected, column values are added together,
// otherwise the columns are cleared
bool AttributeTable::importTable(std::istream& stream, bool merge)
{
   std::string inputline;
   stream >> inputline;
   
   // check for a tab delimited header line...
   auto strings = dXstring::split(inputline, '\t');
   if (strings.size() < 1) {
      return false;
   }
   // the first column *must* be "Ref"
   if (strings[0] != "Ref" && strings[0] != "ref") {  //EF replace || with &&
      return false;
   }

   pvecint colrefs;

   for (size_t i = 1; i < strings.size(); i++) {
      int col = getOrInsertColumnIndex(strings[i]);
      colrefs.push_back( col );
   }

   // check no columns to import (note, this is not necessarily an error, there may 
   // simply be no columns to import) -- handle false return appropriately
   if (colrefs.size() == 0) {
      return false;
   }

   while (!stream.eof()) {
      stream >> inputline;
      if (!inputline.empty()) {
         auto strings = dXstring::split(inputline, '\t');
         if (!strings.size()) {
            continue;
         }
         if (strings.size() != 1 + colrefs.size()) {
            return false;
         }
         try {
            int ref = std::stoi(strings[0]);
            int rowid = getRowid(ref);
            if (rowid != -1) {
               for (size_t i = 1; i < strings.size(); i++) {
                  if (merge) { //EF setValue only if not merge
                     auto value = std::stof(strings[i]);
                     if (value != -1.0) { // only add the value if not -1.0
                        incrValue(rowid,colrefs[i-1],value);
                     }
                  }
                  else {
                     setValue(rowid,colrefs[i-1],std::stof(strings[i]));
                  }
               }
            }
            else {
               // major problem -- row doesn't exist in attribute table
               return false;
            }
         }
         catch (std::invalid_argument &) {
            return false;
         }
      }
   }
   // note: there's no test to check all rowids have been updated
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

bool AttributeColumn::read( std::istream& stream, int version )
{
   m_updated = false;
   m_name = dXstring::readString(stream);
   float min, max;
   stream.read((char *)&min, sizeof(min));
   stream.read((char *)&max, sizeof(max));
   m_min = min;
   m_max = max;

   stream.read((char *)&m_tot, sizeof(m_tot));

   stream.read((char *)&m_physical_col, sizeof(m_physical_col));
   stream.read((char *)&m_hidden, sizeof(m_hidden));

   stream.read((char *)&m_locked, sizeof(m_locked));

   stream.read((char*)&m_display_params,sizeof(m_display_params));
   m_formula = dXstring::readString(stream);
   return true;
}

bool AttributeColumn::write( std::ofstream& stream, int version )
{
   m_updated = false;
   dXstring::writeString(stream, m_name);
   float min = (float) m_min;
   float max = (float) m_max;
   stream.write((char *)&min, sizeof(min));
   stream.write((char *)&max, sizeof(max));
   stream.write((char *)&m_tot, sizeof(m_tot));
   stream.write((char *)&m_physical_col, sizeof(m_physical_col));
   stream.write((char *)&m_hidden, sizeof(m_hidden));
   stream.write((char *)&m_locked, sizeof(m_locked));
   stream.write((char *)&m_display_params,sizeof(m_display_params));
   dXstring::writeString(stream, m_formula);
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
