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


// THIS CODE IS DEPRECATED DO NOT USE: USE SHAPE MAPS INSTEAD 21.08.05

// INCLUDED ONLY TO ALLOW COMPATIBILITY WITH OLD VERSIONS

// DataLayer.h: interface for the DataLayer class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __DATA_LAYER_H__
#define __DATA_LAYER_H__

namespace mgraph440 {

// a layer object, e.g., a gate or a room

class DataException {
public:
   enum {UNDEFINED, LAYER_HAS_COLUMNS};
protected:
   int m_exception;
public:
   DataException(int e = 0) {
      m_exception = e;
   }
   int errorCode() {
      return m_exception;
   }
};

class DataObject
{
   friend class DataLayer;
protected:
   int m_object_ref;
   std::string m_object_name;
   Point2f m_centroid;
   pvecdouble m_data_cols;
   float m_color;
   bool m_selected;
public:
   DataObject(int ref = 0, const std::string& name = std::string()) {
      m_object_ref = ref;
      m_object_name = name;
      m_selected = false;
      m_color = 0.0f;
   }
   double& operator [] (int i) {
      return m_data_cols[i];
   }
   const double& operator [] (int i) const {
      return m_data_cols[i];
   }
   int size() const {
      return (int) m_data_cols.size();
   }
   void setCentroid(const Point2f& p) {
      m_centroid = p;
   }
   const Point2f& getCentroid() const {
      return m_centroid;
   }
   float getColor() const {
      return m_color;
   }
   bool isSelected() const {
      return m_selected;
   }
   void setSelected(bool on = true) {
      m_selected = on;
   }
   //
   friend bool operator == (const DataObject& a, const DataObject& b);
   friend bool operator != (const DataObject& a, const DataObject& b);
   friend bool operator <  (const DataObject& a, const DataObject& b);
   friend bool operator >  (const DataObject& a, const DataObject& b);
   //
   bool read( ifstream& stream, int version );
   bool write(std::ostream &stream );
};

inline bool operator == (const DataObject& a, const DataObject& b) {
   return a.m_object_ref == b.m_object_ref;
}
inline bool operator != (const DataObject& a, const DataObject& b) {
   return a.m_object_ref != b.m_object_ref;
}
inline bool operator <  (const DataObject& a, const DataObject& b) {
   return a.m_object_ref < b.m_object_ref;
}
inline bool operator >  (const DataObject& a, const DataObject& b) {
   return a.m_object_ref > b.m_object_ref;
}

class DataLayer
{
protected:
   int m_layer_ref;
   std::string m_layer_name;
   int m_next_object_ref;
   pqvector<DataObject> m_data_objects;
   std::vector<std::string> m_column_titles;
   int m_display_column;
public:
   DataLayer(int ref = -1, const std::string& name = std::string()) {
      m_layer_ref = ref;
      m_layer_name = name;
      m_next_object_ref = -1;
      m_display_column = 0; // n.b. display column 0 is ref... always present
   }
   // Display column used to just set a column to display:
   // now it recalculates the colours too
   void setDisplayColumn(int i);
   //
   int addObject() {
      m_data_objects.push_back(DataObject(++m_next_object_ref));
      for (size_t i = 0; i < m_column_titles.size(); i++) {
         // this used to be a problem so I was throwing an exception
         // throw LayerException(LayerException::LAYER_HAS_COLUMNS);
         // but I decided just to fill the object with blanks instead:
         m_data_objects.tail().m_data_cols.push_back(0.0);
         m_data_objects.tail().m_color = 0.0;   // <- range 0 to 1
      }
      setDisplayColumn(m_display_column);
      return m_next_object_ref;
   }
   void clear() {
      m_data_objects.clear();
      m_next_object_ref = -1;
      m_display_column = 0;
      m_column_titles.clear();
   }
   void clearSel() {
      for (size_t i = 0; i < m_data_objects.size(); i++) {
         m_data_objects[i].setSelected(false);
      }
   }
   DataObject& operator [] (size_t i) {
      return m_data_objects[i];
   }
   const DataObject& operator [] (size_t i) const {
      return m_data_objects[i];
   }
   int getLayerRef() const
   { return m_layer_ref; }
   void setLayerName(const std::string& name)
   { m_layer_name = name; }
   const std::string& getLayerName() const
   { return m_layer_name; }
   //
   int addColumn(std::string title) {
      // probably ought to insert -1.0 to be consistent with -1.0 nulls in attributes tables
      // but for the time being it's easier to set to 0.0
      // -- and helps as agent trace / collision counts should default to 0.0
      // -- and helps for push column to layer
      auto iter = std::find(m_column_titles.begin(), m_column_titles.end(), title);
      if (iter != m_column_titles.end()) {
         size_t index = iter - m_column_titles.begin();
         for (size_t i = 0; i < m_data_objects.size(); i++) {
            m_data_objects[i][index] = 0.0;
         }
         return (int) index;
      }
      m_column_titles.push_back(title);
      for (size_t i = 0; i < m_data_objects.size(); i++) {
         m_data_objects[i].m_data_cols.push_back(0.0);
      }
      return (int) m_column_titles.size() - 1;
   }
   void delColumn(int col) {
      m_column_titles.erase(m_column_titles.begin() + col);
      for (size_t i = 0; i < m_data_objects.size(); i++) {
         m_data_objects[i].m_data_cols.remove_at(col);
      }
   }
   void setColumnTitle(int i, std::string& name) {
      m_column_titles[i] = name;
   }
   std::string& getColumnTitle(int i) {
      return m_column_titles[i];
   }
   int getColumnIndex(const std::string& title) {
      auto iter = std::find(m_column_titles.begin(), m_column_titles.end(), title);
      return (iter  == m_column_titles.end() ? -1 : int(iter - m_column_titles.begin())); // note: must convert to -1
   }
   int getColumnCount() {
      return (int) m_column_titles.size();
   }
   int getObjectCount() {
      return (int) m_data_objects.size();
   }
public:
   // temporarily public!
   // min and max values displayed:
   double m_display_min;
   double m_display_max;
   // and the average value displayed:
   double m_display_avg;
public:
   int getDisplayColumn() {
      return m_display_column; // nb, 0 is used for ref... always present
   }
   float getObjectValue(int object_ref) {
      if (m_display_column == 0) return (float)object_ref;
      else return (float)m_data_objects[object_ref].m_data_cols[m_display_column-1];
   }
   std::string getObjectText(int object_ref) {
      char val[16];
      if (m_display_column == 0) sprintf(val,"%d",object_ref);
      else sprintf(val,"%g",m_data_objects[object_ref].m_data_cols[m_display_column-1]);
      return val;
   }
   PafColor getObjectColor(int object_ref) {
      PafColor color;
      return color.makeColor(m_data_objects[object_ref].m_color, DisplayParams());
   }
   //
   friend bool operator == (const DataLayer& a, const DataLayer& b);
   friend bool operator != (const DataLayer& a, const DataLayer& b);
   friend bool operator <  (const DataLayer& a, const DataLayer& b);
   friend bool operator >  (const DataLayer& a, const DataLayer& b);
   //
   bool read( ifstream& stream, int version );
   bool write( ofstream& stream );
   bool output( ofstream& stream );
};

inline bool operator == (const DataLayer& a, const DataLayer& b) {
   return a.m_layer_ref == b.m_layer_ref;
}
inline bool operator != (const DataLayer& a, const DataLayer& b) {
   return a.m_layer_ref != b.m_layer_ref;
}
inline bool operator <  (const DataLayer& a, const DataLayer& b) {
   return a.m_layer_ref < b.m_layer_ref;
}
inline bool operator >  (const DataLayer& a, const DataLayer& b) {
   return a.m_layer_ref > b.m_layer_ref;
}

class DataLayers
{
public:
   // standard types I've thought of so far
   // Note that BOUNDARIES comprise blocked locations (which may or may not be visibility graph members)
   enum { GATES = 0, ATTRACTORS = 1, GENERATORS = 2, BOUNDARIES = 3 };
protected:
   int m_current_layer_index;
   pqvector<DataLayer> m_layers;
public:
   DataLayers() {;}
   bool addLayer(int layer_ref, const std::string& layer_name) {
      size_t index = m_layers.searchindex(layer_ref);
      if (index != paftl::npos) {
         m_layers.remove_at(index);
      }
      m_current_layer_index = (int) m_layers.add(DataLayer(layer_ref,layer_name));
      return true;
   }
   bool layerExists(int layer_ref) {
      size_t index = m_layers.searchindex(layer_ref);
      return (index != paftl::npos);
   }
   bool setCurrentLayerRef(int ref) {
      size_t index = m_layers.searchindex(ref);
      if (index == paftl::npos) {
         return false;
      }
      m_current_layer_index = (int) index;
      return true;
   }
   int getCurrentLayerRef() const {
      return m_layers[m_current_layer_index].getLayerRef();
   }
   int getLayerRef(int i) const {
      return m_layers[i].getLayerRef();
   }
   DataLayer& getLayer(int layer_ref) {
      return m_layers.search(layer_ref);
   }
   const DataLayer& getLayer(int layer_ref) const {
      return m_layers.search(layer_ref);
   }
   DataLayer& operator [] (int i) {
      return m_layers[i];
   }
   const DataLayer& operator [] (int i) const {
      return m_layers[i];
   }
   DataLayer& getCurrentLayer() {
      return m_layers[m_current_layer_index];
   }
   const DataLayer& getCurrentLayer() const {
      return m_layers[m_current_layer_index];
   }
   int getLayerCount() const {
      return (int) m_layers.size();
   }
   //
   bool read( ifstream& stream, int version );
   bool write( ofstream& stream );
};

}

#endif
