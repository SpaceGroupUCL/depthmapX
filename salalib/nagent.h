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


//

#ifndef __NAGENT_H__
#define __NAGENT_H__

#include "salalib/pixelref.h"
#include "salalib/point.h"
#include "salalib/pointdata.h"
#include "salalib/shapemap.h"
#include "salalib/tracemap.h"

#include "genlib/pflipper.h"

const char g_col_total_counts[] = "Gate Counts";
const char g_col_gate_counts[] = "__Internal_Gate_Counts";
const char g_col_gate[] = "__Internal_Gate";

//////////////////////////////////////////////////////////////////////

// weighting 
struct wpair
{
   double weight;
   int node;
   wpair(double w = 0.0, int n = -1) 
   { weight = w; node = n; }
};


/*
// this is how pixels are referenced in depthmapX
struct Pixel
{
   // note, bit fields are stored the wrong way round:
   short y : 16;
   short x : 16;
   Pixel()
   { x = -1; y = -1; }
   Pixel(int v)
   { x = v >> 16; y = v & 0xffff; }
   operator int ()
   { return (int&) *this; }
};
*/

// convert an x / y difference to it's corresponding connection direction
inline char connectValue(PixelRef dir)
{
   if (dir.y > 0) {
      return (Point::CONNECT_NE << (1 - dir.x));
   }
   else if (dir.y < 0) {
      return (Point::CONNECT_SW << (dir.x + 1));
   }
   else if (dir.x == 1) {
      return (char) Point::CONNECT_E;
   }
   else {
      return (char) Point::CONNECT_W;
   }
}

/*
// some basic mathematical operators you might want
struct DPoint2 : public DPoint
{
   DPoint2()
   { x = 0.0; y = 0.0; }
   DPoint2(double p, double q)
   { x = p; y = q; }
   DPoint2(const DPoint& d) 
   { x = d.x; y = d.y; }
   operator DPoint() { return DPoint(x,y); }
   double length() { return sqrt(x * x + y * y); }
   DPoint2& normalise() { double len = length(); x /= len; y /= len; return *this; }
   DPoint2& scale(double s) { x *= s; y *= s; return *this; }
   // normalise first before using angle / bin functions
   double angle() const { return y < 0 ? 2.0 * DLL_PI - acos(x) : acos(x); }
   // note the add 0.5 means angles from e.g., -1/32 to 1/32 are in bin 0
   int bin() const { return int(32.0 * (0.5 * angle() / DLL_PI) + 0.5);}
   double& operator [] (int i) { return i == 0 ? x : y; }
   const double operator [] (int i) const { return i == 0 ? x : y; }
   friend DPoint2 operator + (const DPoint2& a, const DPoint2& b);
   friend DPoint2 operator - (const DPoint2& a, const DPoint2& b);
   friend double det(const DPoint2& a, const DPoint2& b);
   friend double dot(const DPoint2& a, const DPoint2& b);
   friend double dist(const DPoint2& a, const DPoint2& b);
   friend DPoint2 operator * (double s, const DPoint2& b);
   friend DPoint2 operator * (const DPoint2& a, double s);
   //
   DPoint2& rotate(const double theta) 
   { double t = x; x =  x * cos(theta) - y * sin(theta);
                   y =  y * cos(theta) + t * sin(theta); return *this; }
};
inline DPoint2 operator + (const DPoint2& a, const DPoint2& b)
{ return DPoint2(a.x + b.x, a.y + b.y); }
inline DPoint2 operator - (const DPoint2& a, const DPoint2& b)
{ return DPoint2(a.x - b.x, a.y - b.y); }
inline double det(const DPoint2& a, const DPoint2& b)
{ return (a.x * b.y - a.y * b.x); }
inline double dot(const DPoint2& a, const DPoint2& b)
{ return (a.x * b.x + a.y * b.y); }
inline double dist(const DPoint2& a, const DPoint2& b)
{ DPoint2 c = b - a; return c.length(); }
inline DPoint2 operator * (double s, const DPoint2& b)
{ return DPoint2(s * b.x, s * b.y); }
inline DPoint2 operator * (const DPoint2& a, double s)
{ return DPoint2(s * a.x, s * a.y); }
*/
////////////////////////////////////////////////////////////////

class AgentEngine;
struct AgentSet;
struct AgentProgram;
class Agent;

class AgentEngine
{
public: // public for now for speed
    std::vector<AgentSet> agentSets;
   int m_gatelayer;
   int m_timesteps;
public:
   bool m_record_trails;
   int m_trail_count = 50;
public:
   AgentEngine();
   void run(Communicator *comm, PointMap *pointmap);
   void insertTrailsInMap(ShapeMap& trailMap);
   void insertTrailsInMap(TraceMap& trailMap);
};

struct AgentProgram
{
   // comparative is comparative with current heading
   enum { SEL_LOS = 0x0001, SEL_LOS_OCC = 0x0002,
          SEL_TARGETTED = 0x1000, 
          SEL_STANDARD = 0x1001, SEL_WEIGHTED = 0x1002, 
          SEL_GIBSONIAN = 0x2000, 
          SEL_LENGTH = 0x2001, SEL_OPTIC_FLOW = 0x2002,
          SEL_COMPARATIVE_LENGTH = 0x2003, SEL_COMPARATIVE_OPTIC_FLOW = 0x2004,
          SEL_GIBSONIAN2 = 0x4000,
          SEL_OPTIC_FLOW2 = 0x4001,
          SEL_OCCLUSION = 0x9000,
          SEL_OCC_ALL = 0x9001,
          SEL_OCC_BIN45 = 0x9002, SEL_OCC_BIN60 = 0x9003, 
          SEL_OCC_STANDARD = 0x9004,
          SEL_OCC_WEIGHT_DIST = 0x9005, SEL_OCC_WEIGHT_ANG = 0x9006, SEL_OCC_WEIGHT_DIST_ANG = 0x9007,
          SEL_OCC_MEMORY = 0x9008
   };
   int m_sel_type;
   int m_steps;
   int m_vbin;
   // these three variables for evolved Gibsonian agents:
   int m_rule_order[4];
   float m_rule_threshold[4];
   float m_rule_probability[4];
   // these are for optic flow 2 agents
   int m_vahead;  // how wide your ahead vision is
   float m_ahead_threshold;      // will turn if neg flow greater than this threshold (set in range 1/100 to 1)
   float m_feeler_threshold;     // will turn if flow greater than this threshold (set in range 1 to 5)
   float m_feeler_probability;   // turn with this much probability if a feeler triggers
   //
   // simple long range destinations:
   bool m_destination_directed;
   bool m_los_sqrd;
   //
   // if it is going to evolved, then have it remember its fitness:
   double m_fitness;
   //
   AgentProgram();
   //
   // for evolution
   void mutate();
   friend AgentProgram crossover(const AgentProgram& prog_a, const AgentProgram& prog_b);
   // to reload later:
   void save(const std::string& filename);
   bool open(const std::string& filename);
   std::vector<std::vector<Event2f>> m_trails;
};

struct AgentSet : public AgentProgram
{
   std::vector<Agent> agents;
   std::vector<int> m_release_locations;
   int m_release_locations_seed = 0;
   double m_release_rate;
   int m_lifetime;
   AgentSet();
   void move(int currentTimeStep);
   void init(int agent, int trail_num = -1);
};

const int POPSIZE = 500;
// redo ASSAYs -- assaysize * assays (3 * 200 = 600 evaluations total)
// then take mean fitness: due to large variation in fitnesses with
// short assays such as this
const int ASSAYS = 3;
const int ASSAYSIZE = 25000;
const int GENERATIONS = 10000;
const int TIMESTEPS = 1600;

struct ProgramPopulation
{
public:
   AgentProgram m_population[POPSIZE];
public:
   ProgramPopulation() {;}
   AgentProgram *makeChild();
   void sort();
};

class Agent
{
public:
   enum { OUTPUT_NOTHING = 0x00, OUTPUT_COUNTS = 0x01, OUTPUT_GATE_COUNTS = 0x02, OUTPUT_TRAILS = 0x04 };
protected:
   AgentProgram *m_program;
   PointMap *m_pointmap;
   //
   PixelRef m_node;
   int m_step;
   int m_frame;
   int m_gate;
   bool m_stuck;
   bool m_stopped;
   bool m_target_lock;
   bool m_gate_encountered;
   bool m_at_target;
   bool m_at_destination;
   int m_output_mode;
   Point2f m_loc;
   Point2f m_target;
   Point2f m_vector;
   PixelRef m_target_pix;
   // a long term goal:
   Point2f m_destination;
   //
   // for recording trails:
   int m_trail_num;
   //
   // for occlusion memory
   pflipper<PixelRefVector> m_occ_memory;
   //
   // extra memory of last observed values for Gibsonian agents:
   float m_last_los[9];
   float m_curr_los[9];
public:
   Agent()
   { m_program = NULL; m_pointmap = NULL; m_output_mode = OUTPUT_NOTHING; }
   Agent(AgentProgram *program, PointMap *pointmap, int output_mode = OUTPUT_NOTHING);
   void onInit(PixelRef node, int trail_num = -1);
   void onClose();
   Point2f onLook(bool wholeisovist);
   Point2f onStandardLook(bool wholeisovist);
   Point2f onWeightedLook(bool wholeisovist);
   Point2f onOcclusionLook(bool wholeisovist, int looktype);
   Point2f onLoSLook(bool wholeisovist, int look_type);
   Point2f onDirectedLoSLook(bool wholeisovist, int look_type);
   Point2f onGibsonianLook(bool wholeisovist);
   Point2f onGibsonianLook2(bool wholeisovist);
   int onGibsonianRule(int rule);
   void calcLoS(int directionbin, bool curr);
   void calcLoS2(int directionbin, bool curr);
   void onMove();
   void onUpdateTrail(double currentTime);
   void onTarget();
   void onDestination();
   void onStep();
   bool diagonalStep();
   bool goodStep(PixelRef node);
   bool gateEncountered()
   { return m_gate_encountered; }
   const Point2f& getLoc() const 
   { return m_loc; }
   //
   const bool atTarget() const { return m_at_target; }
   const bool atDestination() const { return m_at_destination; }
   //
   const Point2f& getLocation() const
   { return m_loc; }
   const Point2f& getVector() const
   { return m_vector; }
   const PixelRef getNode() const
   { return m_node; }
   const int getFrame() const
   { return m_frame; }
   const PointMap& getPointMap() const
   { return *m_pointmap; }
};

// note the add 0.5 means angles from e.g., -1/32 to 1/32 are in bin 0
inline int binfromvec(const Point2f& p)
{ return int(32.0 * (0.5 * p.angle() / M_PI) + 0.5); }

// a random angle based on a bin direction
inline double anglefrombin2(int here)
{
   return (2.0 * M_PI) * ((double(here)-0.5)/32.0 + prandom()/32.0);
}

inline int binsbetween(int bin1, int bin2)
{
   int b = abs(bin1 - bin2);
   if (b > 16) {
      b = 32 - b;
   }
   return b;
}

/////////////////////////////////////////////////////////////////////////////////////

// Playback for recorded traces

struct Trace
{
   double starttime;
   double endtime;
   std::vector<Event2f> events;
};

/////////////////////////////////////////////////////////////////////////////////////

#endif
