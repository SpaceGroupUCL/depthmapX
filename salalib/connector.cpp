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
#include <time.h>
#include <genlib/paftl.h>
#include <genlib/comm.h> // for communicator

#include <salalib/mgraph.h> // purely for the version info --- as phased out should replace
#include <salalib/connector.h>

bool Connector::read( istream& stream, int version, pvecint *keyvertices )
{
   m_connections.clear();
   m_forward_segconns.clear();
   m_back_segconns.clear();

   // n.b., must set displayed attribute as soon as loaded...
   m_connections.read(stream);

   stream.read((char *)&m_segment_axialref, sizeof(m_segment_axialref));
   m_forward_segconns.read(stream);
   m_back_segconns.read(stream);

   return true;
}

bool Connector::write( ofstream& stream )
{
   // n.b., must set displayed attribute as soon as loaded...
   m_connections.write(stream);
   // m_keyvertices.write(stream);
   stream.write((char *)&m_segment_axialref, sizeof(m_segment_axialref));
   m_forward_segconns.write(stream);
   m_back_segconns.write(stream);

   return true;
}

/////////////////////////////////////////////////////////////////////////////////

// Cursor extras

int Connector::count(int mode) const
{
   int c = 0;
   switch (mode) {
   case CONN_ALL:
      c = m_connections.size();
      break;
   case SEG_CONN_ALL:
      c = m_back_segconns.size() + m_forward_segconns.size();
      break;
   case SEG_CONN_FW:
      c = m_forward_segconns.size();
      break;
   case SEG_CONN_BK:
      c = m_back_segconns.size();
      break;
   }
   return c;
}
int Connector::cursor(int mode) const
{
   int cur = -1;
   if (m_cursor != -1) {
      switch (mode) {
      case CONN_ALL:
         if (m_cursor < (int)m_connections.size()) {
            cur = m_connections[m_cursor];
         }
         break;
      case SEG_CONN_ALL:
         if (m_cursor < (int)m_back_segconns.size()) {
            cur = m_back_segconns.key(m_cursor).ref;
         }
         else if (m_cursor - m_back_segconns.size() < m_forward_segconns.size()) {
            cur = m_forward_segconns.key(m_cursor - m_back_segconns.size()).ref;
         }
         break;
      case SEG_CONN_FW:
         if (m_cursor < (int)m_forward_segconns.size()) {
            cur = m_forward_segconns.key(m_cursor).ref;
         }
         break;
      case SEG_CONN_BK:
         if (m_cursor < (int)m_back_segconns.size()) {
            cur = m_back_segconns.key(m_cursor).ref;
         }
         break;
      }
   }
   if (cur == -1) {
      m_cursor = -1;
   }
   return cur;
}
int Connector::direction(int mode) const
{
   int direction = 0;
   if (m_cursor != -1) {
      switch (mode) {
      case SEG_CONN_ALL:
         if (m_cursor < (int)m_back_segconns.size()) {
            direction = m_back_segconns.key(m_cursor).dir;
         }
         else if (m_cursor - m_back_segconns.size() < m_forward_segconns.size()) {
            direction = m_forward_segconns.key(m_cursor - m_back_segconns.size()).dir;
         }
         break;
      case SEG_CONN_FW:
         direction = m_forward_segconns.key(m_cursor).dir;
         break;
      case SEG_CONN_BK:
         direction = m_back_segconns.key(m_cursor).dir;
         break;
      }
   }
   return direction;
}
float Connector::weight(int mode) const
{
   float weight = 0.0f;
   if (m_cursor != -1) {
      switch (mode) {
      case SEG_CONN_ALL:
         if (m_cursor < (int)m_back_segconns.size()) {
            weight = m_back_segconns.value(m_cursor);
         }
         else if (m_cursor - m_back_segconns.size() < m_forward_segconns.size()) {
            weight = m_forward_segconns.value(m_cursor - m_back_segconns.size());
         }
         break;
      case SEG_CONN_FW:
         weight = m_forward_segconns.value(m_cursor);
         break;
      case SEG_CONN_BK:
         weight = m_back_segconns.value(m_cursor);
         break;
      }
   }
   return weight;
}
