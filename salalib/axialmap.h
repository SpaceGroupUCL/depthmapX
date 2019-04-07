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


#pragma once

#include "salalib/spacepixfile.h"
#include "salalib/spacepix.h"
#include "salalib/connector.h"

struct AxialVertex;
struct AxialVertexKey;
struct RadialLine;
struct PolyConnector;

// used during angular analysis
struct AnalysisInfo
{
   // lists used for multiple radius analysis
   bool leaf;
   bool choicecovered;
   SegmentRef previous;
   int depth;
   double choice;            
   double weighted_choice;
   double weighted_choice2; //EFEF
   AnalysisInfo() {
      choicecovered = false; leaf = true; previous = SegmentRef(); depth = 0; choice = 0.0; weighted_choice = 0.0; weighted_choice2 = 0.0; 
   }
   void clearLine() {
      choicecovered = false; leaf = true; previous = SegmentRef(); depth = 0; // choice values are cummulative and not cleared
   }
};

class MapInfoData;

typedef std::vector<std::set<int>> KeyVertices;

class ShapeGraph : public ShapeMap
{
   friend class AxialMinimiser;
   friend class MapInfoData;
protected:
   KeyVertices m_keyvertices;       // but still need to return keyvertices here
   int m_keyvertexcount;
protected:
public:
   bool outputMifPolygons(std::ostream& miffile, std::ostream& midfile) const;
   void outputNet(std::ostream& netfile) const;
public:
   ShapeGraph(const std::string& name = "<axial map>", int type = ShapeMap::AXIALMAP);
   virtual ~ShapeGraph() {;}
   void initialiseAttributesAxial();
   void makeConnections(const KeyVertices &keyvertices = KeyVertices());
   bool stepdepth(Communicator *comm = NULL);
   // lineset and connectionset are filled in by segment map
   void makeNewSegMap();
   void makeSegmentMap(std::vector<Line> &lines, std::vector<Connector> &connectors, double stubremoval);
   void initialiseAttributesSegment();
   void makeSegmentConnections(std::vector<Connector> &connectionset);
   void pushAxialValues(ShapeGraph& axialmap);
   //
   virtual bool read(std::istream& stream);
   bool readold(std::istream& stream);
   virtual bool write(std::ofstream& stream);
   void writeAxialConnectionsAsDotGraph(std::ostream &stream);
   void writeAxialConnectionsAsPairsCSV(std::ostream &stream);
   void writeSegmentConnectionsAsPairsCSV(std::ostream &stream);
   void unlinkAtPoint(const Point2f& unlinkPoint);
   void unlinkFromShapeMap(const ShapeMap& shapemap);
};
