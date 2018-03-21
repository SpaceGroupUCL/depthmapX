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



// a simple agent demonstration for Salad (now within depthmapX)
#include <genlib/paftl.h>
#include <genlib/comm.h>

#include <salalib/mgraph.h>
#include <salalib/nagent.h>
#include <salalib/ngraph.h>
#include "genlib/stringutils.h"
#include "genlib/containerutils.h"

int thisrun = 0;

///////////////////////////////////////////////////////////////////////////////////////////////

static int rankselect(int popsize)
{
   int num = int( prandom() * popsize * (popsize + 1) * 0.5 );
   for (int i = 0; i < popsize; i++) {
      if (num < (popsize - i)) {
         return i;
      }
      num -= (popsize - i);
   }
   return 0;   // <- this shouldn't happen
}

// note: this is tested and right: higher fitness, lower rank (so population[0] is best)
int progcompare(const void *a, const void *b )
{
   double test = (((AgentProgram *)a)->m_fitness - ((AgentProgram *)b)->m_fitness);
   if (test < 0.0) {
      return 1;
   }
   else if (test > 0.0) {
      return -1;
   }
   return 0;
}

//

pqvector<Point2f> g_trails[MAX_TRAILS];

///////////////////////////////////////////////////////////////////////////////////////////////

// run one agent engine only

AgentEngine::AgentEngine()
{
   m_timesteps = 5000;
   m_gatelayer = -1;
   m_record_trails = false;
   m_trail_count = MAX_TRAILS;
}

void AgentEngine::run(Communicator *comm, PointMap *pointmap)
{
    for (size_t j = 0; j < size(); j++) {
        if(at(j).m_sel_type == AgentProgram::SEL_LOS_OCC) {
            pointmap->requireIsovistAnalysis();
        }
    }
   // Quick mod - TV
#if defined(_WIN32)   
   __time64_t atime = 0;
#else
   time_t atime =0;
#endif   
   if (comm) {
      qtimer( atime, 0 );
      comm->CommPostMessage( Communicator::NUM_RECORDS, m_timesteps );
   }
   
   AttributeTable& table = pointmap->getAttributeTable();
   int displaycol = table.insertColumn(g_col_total_counts);

   int output_mode = Agent::OUTPUT_COUNTS;
   if (m_gatelayer != -1) {
      output_mode |= Agent::OUTPUT_GATE_COUNTS;
   }

   // remove any agent trails that are left from a previous run
   for (int k = 0; k < MAX_TRAILS; k++) {
      g_trails[k].clear();
   }

   int trail_num = -1;
   if (m_record_trails) {
      if (m_trail_count < 1) {
         m_trail_count = 1;
      }
      if (m_trail_count > MAX_TRAILS) {
         m_trail_count = MAX_TRAILS;
      }
      trail_num = 0;
   }

   // remove any agents that are left from a previous run
   for (size_t j = 0; j < size(); j++) {
      at(j).clear();
   }

   for (int i = 0; i < m_timesteps; i++) {

      size_t j;
      for (j = 0; j < size(); j++) {
         int q = invcumpoisson(prandomr(),at(j).m_release_rate);
         int length = at(j).size();
         int k;
         for (k = 0; k < q; k++) {
            at(j).push_back(Agent(&(at(j)),pointmap,output_mode));
         }
         for (k = 0; k < q; k++) {
            at(j).init(length+k,trail_num);
            if (trail_num != -1) {
               trail_num++;
               // after trail count, stop recording:
               if (trail_num == m_trail_count) {
                  trail_num = -1;
               }
            }
         }
      }

      for (j = 0; j < size(); j++) {
         at(j).move();
      }

      if (comm) {
         if (qtimer( atime, 500 )) {
            if (comm->IsCancelled()) {
               throw Communicator::CancelledException();
            }
            comm->CommPostMessage( Communicator::CURRENT_RECORD, i );
         }         
      }
   }

   // output agent trails to file:
   if (m_record_trails) {
       // just dump in local file...
       ofstream trails("trails.cat");
       outputTrails(trails);
   }

   // actually, no, do this from the 
   pointmap->overrideDisplayedAttribute(-2);
   pointmap->setDisplayedAttribute(displaycol);
}

void AgentEngine::outputTrails(ostream& trailsFile) {
    trailsFile << "CAT" << endl;
    for (int i = 0; i < m_trail_count; i++) {
       trailsFile << "Begin Polyline" << endl;
       for (size_t j = 0; j < g_trails[i].size(); j++) {
          trailsFile << g_trails[i][j].x << " " << g_trails[i][j].y << endl;
       }
       trailsFile << "End Polyline" << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

AgentSet::AgentSet()
{
   m_release_rate = 0.1;
   m_lifetime = 1000;
}

void AgentSet::init(int agent, int trail_num)
{
   if (m_release_locations.size()) {
      int which = pafrand() % m_release_locations.size();
      at(agent).onInit( m_release_locations[which], trail_num );
   }
   else {
      const PointMap& map = at(agent).getPointMap();
      PixelRef pix;
      do {
         pix = map.pickPixel(prandom(m_release_locations_seed));
      } while (!map.getPoint(pix).filled());
      at(agent).onInit(pix, trail_num);
   }
}

void AgentSet::move()
{
   // go through backwards so remove does not affect later agents
   for (size_t i = size() - 1; i != paftl::npos; i--) {
      at(i).onMove();
      if (at(i).getFrame() >= m_lifetime) {
         remove_at(i);
      }
   }
}

///////////////////////////////////////////////////////////////////////////////////////////////

AgentProgram *ProgramPopulation::makeChild()
{
   int a = rankselect(POPSIZE);
   int b = rankselect(POPSIZE);
   while (a == b)
      b = rankselect(POPSIZE);
   m_population[POPSIZE - 1] = crossover(m_population[a],m_population[b]);
   m_population[POPSIZE - 1].mutate();

   return &(m_population[POPSIZE - 1]);
}

// note: this is correct -- do not use &m_population!
void ProgramPopulation::sort()
{
   qsort(m_population,POPSIZE,sizeof(AgentProgram),progcompare);
}

///////////////////////////////////////////////////////////////////////////////////////////////

AgentProgram::AgentProgram()
{
   m_sel_type = SEL_LOS;
   m_steps = 3;
   m_vbin = 7;
   m_destination_directed = false;
   m_los_sqrd = false;
}

/*
AgentProgram::AgentProgram()
{
   // random program for evolution:
   // m_sel_type = SEL_LENGTH + (pafrand() % 4);
   m_sel_type = SEL_OPTIC_FLOW2;

   // non-evolutionary standard agent program
   // note m_steps defaults to 0 for use with evolutionary (reevaulate goal every step)
   m_steps = 0; 
   // also note evolutionary does actually use bins when not following other rules:
   m_vbin = (pafrand() % 7) + 1;

   // optic flow 2
   //m_vahead = (pafrand() % m_vbin);
   //m_ahead_threshold = prandom() * 19.0 + 1.0;
   //m_feeler_threshold = prandom() * 4.0 + 1.0;
   //m_feeler_probability = prandom();
   
   // rule order relies on putting rules into slots:
   for (int i = 0; i < 4; i++) {
      m_rule_order[i] = -1;
   }
   for (int j = 0; j < 4; j++) {
      int choice = pafrand() % (4 - j);
      for (int k = 0; k < choice + 1; k++) {
         if (m_rule_order[k] != -1) {
            choice++;
         }
      }
      m_rule_order[choice] = j;
   }
   for (i = 0; i < 4; i++) {
      m_rule_threshold[i] = prandom() * 100.0;
      m_rule_probability[i] = prandom();
   }

   m_fitness = 0.0;

   m_destination_guided = false;
   m_los_sqrd = false;
}
*/

void AgentProgram::mutate()
{
   // don't mutate program type
/*
   if (pafrand() % 20 == 0) {
      m_vbin = (pafrand() % 7) + 1;
   }
   if (pafrand() % 20 == 0) {
      m_vahead = (pafrand() % m_vbin);
   }
   if (pafrand() % 20 == 0) {
      m_ahead_threshold = prandom() * 19.0 + 1.0;
   }
   if (pafrand() % 20 == 0) {
      m_feeler_threshold = prandom() * 4.0 + 1.0;
   }
   if (pafrand() % 20 == 0) {
      m_feeler_probability = prandom();
   }
*/
  
   // do mutate rule order occassionally:
   if (pafrand() % 20 == 0) {
      // rule order relies on putting rules into slots:
      for (int i = 0; i < 4; i++) {
         m_rule_order[i] = -1;
      }
      for (int j = 0; j < 4; j++) {
         int choice = pafrand() % (4 - j);
         for (int k = 0; k < choice + 1; k++) {
            if (m_rule_order[k] != -1) {
               choice++;
            }
         }
         m_rule_order[choice] = j;
      }
   }
   // mutate the rule threshold / probabilities
   for (int i = 0; i < 4; i++) {
      if (pafrand() % 20 == 0) { // 5% mutation rate
         m_rule_threshold[i] = float(prandom() * 100.0);
      }
      if (pafrand() % 20 == 0) { // 5% mutation rate
         m_rule_probability[i] = float(prandom());
      }
   }
  
}

AgentProgram crossover(const AgentProgram& prog_a, const AgentProgram& prog_b)
{
   AgentProgram child;
   // either one sel type or the other:
  /*
   if (pafrand() % 2) {
      child.m_sel_type = prog_a.m_sel_type;
   }
   else {
      child.m_sel_type = prog_b.m_sel_type;
   }
   */
   /*
   // either one bin radius or the other
   if (pafrand() % 2)
      child.m_vbin = prog_a.m_vbin;
   else
      child.m_vbin = prog_b.m_vbin;
   if (pafrand() % 2)
      child.m_vahead = prog_a.m_vahead;
   else
      child.m_vahead = prog_b.m_vahead;
   if (pafrand() % 2)
      child.m_ahead_threshold = prog_a.m_ahead_threshold;
   else
      child.m_ahead_threshold = prog_b.m_ahead_threshold;
   if (pafrand() % 2)
      child.m_feeler_threshold = prog_a.m_feeler_threshold;
   else
      child.m_feeler_threshold = prog_b.m_feeler_threshold;
   if (pafrand() % 2)
      child.m_feeler_probability = prog_a.m_feeler_probability;
   else
      child.m_feeler_probability = prog_b.m_feeler_probability;
*/
  
   // either one rule priority order or the other (don't try to mix!)
   if (pafrand() % 2) {
      for (int i = 0; i < 4; i++) {
         child.m_rule_order[i] = prog_a.m_rule_order[i];
      }
   }
   else {
      for (int i = 0; i < 4; i++) {
         child.m_rule_order[i] = prog_b.m_rule_order[i];
      }
   }
   // for each rule, either one rule threshold / probability or the other:
   for (int i = 0; i < 4; i++) {
      if (pafrand() % 2) {
         child.m_rule_threshold[i] = prog_a.m_rule_threshold[i];
      }
      else {
         child.m_rule_threshold[i] = prog_b.m_rule_threshold[i];
      }
      if (pafrand() % 2) {
         child.m_rule_probability[i] = prog_a.m_rule_probability[i];
      }
      else {
         child.m_rule_probability[i] = prog_b.m_rule_probability[i];
      }
   }
  
   return child;
}

void AgentProgram::save(const std::string& filename)
{
   // standard ascii:
   ofstream file(filename.c_str());

   file << "Destination selection: ";
   switch (m_sel_type) {
   case SEL_STANDARD:
      file << "Standard" << endl;
      break;
   case SEL_LENGTH:
      file << "Gibsonian Length" << endl;
      break;
   case SEL_OPTIC_FLOW:
      file << "Gibsonian Optic Flow" << endl;
      break;
   case SEL_COMPARATIVE_LENGTH:
      file << "Gibsonian Comparative Length" << endl;
      break;
   case SEL_COMPARATIVE_OPTIC_FLOW:
      file << "Gibsonian Comparative Optic Flow" << endl;
      break;
   default:
      file << "Unknown" << endl;
   }

   file << "Steps: " << m_steps << endl;
   file << "Bins: " << ((m_vbin == -1) ? 32 : m_vbin * 2 + 1) << endl;
   /*
   file << "Ahead bins: " << m_vahead * 2 + 1 << endl;
   file << "Ahead threshold: " << m_ahead_threshold << endl;
   file << "Feeler threshold: " << m_feeler_threshold << endl;
   file << "Feeler probability: " << m_feeler_probability << endl;
*/
   file << "Rule order: " << m_rule_order[0] << " "
                          << m_rule_order[1] << " "
                          << m_rule_order[2] << " "
                          << m_rule_order[3] << endl;

   for (int i = 0; i < 4; i++) {
      file << "Rule " << i << " (Bin -" << 1 + (i * 2) << "/+" << 1 + (i * 2) << ")" << endl;
      file << "Threshold: " << m_rule_threshold[i] << endl;
      file << "Turn Probability: " << m_rule_probability[i] << endl;
   }

   file << "Fitness: " << m_fitness << endl;
}

bool AgentProgram::open(const std::string& filename)
{
   // standard ascii:
   ifstream file(filename.c_str());

   std::string line;
   file >> line;
   if (!line.empty()) {
      dXstring::toLower(line);
      if (line.substr(0,22) != "destination selection:") {
         return false;
      }
      else {
         std::string method = line.substr(22);
         dXstring::ltrim(method);
         if (!method.empty()) {
            if (method == "standard") {
               m_sel_type = SEL_STANDARD;
            }
            else if (method == "gibsonian length") {
               m_sel_type = SEL_LENGTH;
            }
            else if (method == "gibsonian optic flow") {
               m_sel_type = SEL_OPTIC_FLOW;
            }
            else if (method == "gibsonian comparative length") {
               m_sel_type = SEL_COMPARATIVE_LENGTH;
            }
            else if (method == "gibsonian comparative optic flow") {
               m_sel_type = SEL_COMPARATIVE_OPTIC_FLOW;
            }
            file >> line;
         }
         else {
            return false;
         }
      }
   }
   else {
      return false;
   }

   bool foundsteps = false;
   bool foundbins = false;

   if (!line.empty()) {
      dXstring::toLower(line);
      if (line.substr(0,6) == "steps:") {
         std::string steps = line.substr(6);
         dXstring::ltrim(steps);
         m_steps = stoi(steps);
         file >> line;
         foundsteps = true;   
      }
   }
   else {
      return false;
   }

   if (!line.empty()) {
      dXstring::toLower(line);
      if (line.substr(0,5) == "bins:") {
         std::string bins = line.substr(6);
         dXstring::ltrim(bins);
         int binx = stoi(bins);
         if (binx = 32) {
            m_vbin = -1;
         }
         else {
            m_vbin = (atoi(bins.c_str()) - 1) / 2;
         }
         file >> line;
         foundbins = true;   
      }
   }

   if (m_sel_type == SEL_STANDARD) {
      if (foundbins && foundsteps) {
         return true;
      }
      else {
         return false;
      }
   }

   if (!line.empty()) {
      dXstring::toLower(line);
      if (line.substr(0,11) == "rule order:") {
         std::string ruleorder = line.substr(11);
         dXstring::ltrim(ruleorder);
         auto orders = dXstring::split(ruleorder, ' ');
         if (orders.size() != 4) {
            return false;
         }
         for (int i = 0; i < 4; i++) {
            m_rule_order[i] = stoi(orders[i]);
         }
         file >> line;
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
   for (int i = 0; i < 4; i++) {
      if (!line.empty()) {
         dXstring::toLower(line);
         if (line.substr(0,4) == "rule") {
            file >> line;
         }
         dXstring::toLower(line);
         if (line.substr(0,10)  == "threshold:") {
            auto threshold = line.substr(10);
            dXstring::ltrim(threshold);
            m_rule_threshold[i] = stof(threshold);
            file >> line;
         }
         else {
            return false;
         }
         dXstring::toLower(line);
         if (line.substr(0,17) == "turn probability:") {
            auto prob = line.substr(17);
            dXstring::ltrim(prob);
            m_rule_probability[i] = stof(prob);
            file >> line;
         }
         else {
            return false;
         }
      }
      else {
         return false;
      }
   }

   return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////

Agent::Agent(AgentProgram *program, PointMap *pointmap, int output_mode)
{
   m_program = program;
   m_pointmap = pointmap;
   m_output_mode = output_mode;
   m_trail_num = -1;
}

void Agent::onInit(PixelRef node, int trail_num)
{
   m_node = node;
   m_loc = m_pointmap->depixelate(m_node);
   if (m_output_mode & OUTPUT_GATE_COUNTS) {
      int index = m_pointmap->getAttributeTable().getRowid(m_node);
      m_gate = (index != -1) ? (int)m_pointmap->getAttributeTable().getValue(index,g_col_gate) : -1;
   }
   else {
      m_gate = -1;
   }
   m_gate_encountered = false;
   m_step = 0;
   m_stuck = false;
   m_stopped = false;
   m_frame = 0;
   m_target_lock = false;
   m_vector = Point2f(1,0);

   m_at_target = false;
   m_at_destination = false;

   m_trail_num = trail_num;

   m_vector = onLook(true);

   m_vector.normalise();

   m_target_pix = NoPixel;
}

void Agent::onMove()
{
   m_at_target = false;
   m_frame++;
   if (m_program->m_destination_directed && dist(m_loc,m_destination) < 10.0) {
      // reached final destination
      onDestination();
   }
   else if ((m_program->m_sel_type & AgentProgram::SEL_TARGETTED) && dist(m_loc,m_target) < m_pointmap->getSpacing()) {
      // reached target (intermediate destination)
      m_step = 0;
      onTarget();
      m_vector = onLook(false);
   }
   else if (prandomr() < (1.0 / m_program->m_steps) && !m_target_lock) { // note, on average, will change 1 in steps
      m_step = 0;
      m_vector = onLook(false);
      /*
      if (m_program->m_destination_directed) {
         Point2f vec2 = m_destination - m_loc;
         vec2.normalise();
         if (dot(vec2,m_vector) < 0.0) {
            m_vector = onLook(false);
         }
      }
      */
   }
   if (m_stuck) {
      // oops...
      return;
   }
   // now step...
   PixelRef lastnode = m_node;
   onStep();
   if (m_node != lastnode && m_output_mode != OUTPUT_NOTHING) {
      int index = m_pointmap->getAttributeTable().getRowid(m_node);
      if (index != -1) {
         if (m_output_mode & OUTPUT_COUNTS) {
            m_pointmap->getAttributeTable().incrValue(index, g_col_total_counts);
         }
         if (m_output_mode & OUTPUT_GATE_COUNTS) {
            int obj = (int)m_pointmap->getAttributeTable().getValue(index, g_col_gate);
            if (m_gate != obj) {
               m_gate = obj;
               if (m_gate != -1) {
                  m_pointmap->getAttributeTable().incrValue(index, g_col_gate_counts);
                  // actually crossed into a new gate:
                  m_gate_encountered = true;
               }
            }
         }
      }
   }
   // done. happy hamster.
}
void Agent::onDestination()
{
   m_at_destination = true;
}
void Agent::onTarget()
{
   m_occ_memory.a().clear();
   m_at_target = true;
}

////////////////////////////////////////////////////////////////////

void Agent::onStep()
{
   m_stopped = false;
   m_step++;
   //
   Point2f nextloc = m_loc + (m_pointmap->getSpacing() * m_vector);
   // note: false returns unconstrained pixel: goodStep must check it is in bounds using m_pointmap->includes
   PixelRef nextnode = m_pointmap->pixelate(nextloc,false);
   if (nextnode != m_node) {
      // quick check location is okay...
      if (goodStep(nextnode)) {
         m_node = nextnode;
         m_loc = nextloc;
      }
      else {
         // try other nearby nodes...
         if (!diagonalStep()) {
            m_stopped = true;
         }
      }
   }
   else {
      m_loc = nextloc;
   }
   if (!m_stopped && m_trail_num != -1) {
      g_trails[m_trail_num].push_back(m_loc);
   }
}
bool Agent::diagonalStep() 
{
   Point2f vector1 = m_vector;
   vector1.rotate(M_PI / 4.0);
   Point2f nextloc1 = m_loc + (m_pointmap->getSpacing() * vector1);
   // note: "false" does not constrain to bounds: must be checked using m_pointmap->includes before getPoint is used
   PixelRef nextnode1 = m_pointmap->pixelate(nextloc1,false);

   Point2f vector2 = m_vector;
   vector2.rotate(-M_PI / 4.0);
   Point2f nextloc2 = m_loc + (m_pointmap->getSpacing() * vector2);
   // note: "false" does not constrain to bounds: must be checked using m_pointmap->includes before getPoint is used
   int nextnode2 = m_pointmap->pixelate(nextloc2,false);

   bool good = false;
   if (pafrand() % 2 == 0) {
      if (goodStep(nextnode1)) {
         m_node = nextnode1;
         m_loc = nextloc1;
         good = true;
      }
      else if (goodStep(nextnode2)) {
         m_node = nextnode2;
         m_loc = nextloc2;
         good = true;
      }
   }
   else {
      if (goodStep(nextnode2)) {
         m_node = nextnode2;
         m_loc = nextloc2;
         good = true;
      }
      else if (goodStep(nextnode1)) {
         m_node = nextnode1;
         m_loc = nextloc1;
         good = true;
      }
   }
   return good;
}
bool Agent::goodStep(PixelRef node)
{
   if (!m_pointmap->includes(node)) {
      return false;
   }
   // n.b., you have to know how the nodes are labelled for this connectValue trick
   PixelRef dir;
   dir.x = node.x - m_node.x;
   dir.y = node.y - m_node.y;
   // now translate dir to correct CONNECT value
   if (m_pointmap->getPoint(m_node).getGridConnections() & connectValue(dir)) {
      return true;
   }

   return false;
}

//////////////////////////////////////////////////////////////////////

// The various look algorithms

Point2f Agent::onLook(bool wholeisovist)
{
   Point2f dir;
   if (m_program->m_sel_type & AgentProgram::SEL_GIBSONIAN) {
      dir = onGibsonianLook(wholeisovist);
   }
   else if ((m_program->m_sel_type & AgentProgram::SEL_OCCLUSION) == AgentProgram::SEL_OCCLUSION) {
      dir = onOcclusionLook(wholeisovist, m_program->m_sel_type);
   }
   else {
      switch (m_program->m_sel_type) {
      case AgentProgram::SEL_STANDARD:
         dir = onStandardLook(wholeisovist);
         break;
      case AgentProgram::SEL_LOS: case AgentProgram::SEL_LOS_OCC:
         if (m_program->m_destination_directed) {
            dir = onDirectedLoSLook(wholeisovist, m_program->m_sel_type);
         }
         else {
            dir = onLoSLook(wholeisovist, m_program->m_sel_type);
         }
         break;
      case AgentProgram::SEL_OPTIC_FLOW2:
         dir = onGibsonianLook2(wholeisovist);
         break;
      }
   }
   if ((m_program->m_sel_type & AgentProgram::SEL_GIBSONIAN) && !m_stuck) {
      // remember what the view looked like here, facing our new direction:
      calcLoS(binfromvec(dir),false);
   }
   if ((m_program->m_sel_type & AgentProgram::SEL_GIBSONIAN2) && !m_stuck) {
      calcLoS2(binfromvec(dir),false);
   }

   return dir;
}

Point2f Agent::onStandardLook(bool wholeisovist)
{
   int tarpixelate = -1;
   int vbin = m_program->m_vbin;
   if (wholeisovist || vbin == -1) {
      vbin = 16;
   }
   int directionbin = 32 + binfromvec(m_vector) - vbin;
   int choices = 0;
   // reset for getting list, check in range:
   vbin = vbin * 2 + 1;
   if (vbin > 32) {
      vbin = 32;
   }
   for (int i = 0; i < vbin; i++) {
      choices += m_pointmap->getPoint(m_node).getNode().bincount( (directionbin + i) % 32 );
   }
   if (choices == 0) {
      if (!wholeisovist) {
         return onStandardLook(true);
      }
      else {
         m_stuck = true;
         m_target = m_loc;
         m_target_pix = m_node;
         return Point2f(0,0);
      }
   }
   else {
      int chosen = pafrand() % choices;
      Node& node = m_pointmap->getPoint(m_node).getNode();
      for (; chosen >= node.bincount( directionbin % 32 ); directionbin++) {
         chosen -= node.bincount( directionbin % 32 );
      }
      Bin& bin = node.bin(directionbin % 32);
      bin.first();
      tarpixelate = bin.cursor();
      for (; chosen > 0; chosen--) {
         bin.next();
         tarpixelate = bin.cursor();
      }
   }

   m_target_pix = tarpixelate;
   m_target = m_pointmap->depixelate(tarpixelate);

   return (m_target - m_loc).normalise();
}

Point2f Agent::onWeightedLook(bool wholeisovist)
{
   if (wholeisovist) {
      // use standard targetted look instead:
      return onStandardLook(true);
   }
   int tarpixelate = -1;
   int vbin = m_program->m_vbin;
   if (vbin = -1) {
      vbin = 16;
   }
   int aheadbin = binfromvec(m_vector);
   int directionbin = 32 + binfromvec(m_vector) - vbin;
   prefvec<wpair> weightmap;
   double weight = 0.0;
   // reset for getting list, check in range:
   vbin = vbin * 2 + 1;
   if (vbin > 32) {
      vbin = 32;
   }
   for (int i = 0; i < vbin; i++) {
      Bin& bin = m_pointmap->getPoint(m_node).getNode().bin((directionbin + i) % 32);
      bin.first();
      
      // Quick mod - TV
#if defined(_WIN32)      
      int node = bin.is_tail() ? -1 : bin.cursor();
#else
      int node = bin.is_tail() ? -1 : bin.cursor().x;
#endif      
      while (node != -1) {
         weight += ((directionbin + i) % 32 == aheadbin) ? 5.0 : 1.0;
         weightmap.push_back(wpair(weight,node));
         bin.next();
	 // Quick mod - TV
#if defined(_WIN32)	          
         node = bin.is_tail() ? -1 : bin.cursor();
#else
         node = bin.is_tail() ? -1 : bin.cursor().x;
#endif         
      }
   }
   if (weightmap.size() == 0) {
      return onWeightedLook(true);
   }
   else {
      double chosen = prandomr() * weight;
      for (size_t i = 0; i < weightmap.size(); i++) {
         if (chosen < weightmap[i].weight) {
            tarpixelate = weightmap[i].node;
            break;
         }
      }
   }

   m_target_pix = tarpixelate;
   m_target = m_pointmap->depixelate(tarpixelate);

   return (m_target - m_loc).normalise();
}

Point2f Agent::onOcclusionLook(bool wholeisovist, int looktype)
{
   if (AgentProgram::SEL_OCC_MEMORY) {
      m_occ_memory.flip();
      m_occ_memory.a().clear();
   }

   if (wholeisovist) {
      // use standard targetted look instead:
      return onStandardLook(true);
   }
   PixelRef tarpixelate = NoPixel;
   int vbin = m_program->m_vbin;
   if (vbin = -1) {
      vbin = 16;
   }
   int directionbin = 32 + binfromvec(m_vector) - vbin;
   // reset for getting list, check in range:
   vbin = vbin * 2 + 1;
   if (vbin > 32) {
      vbin = 32;
   }
   if (looktype == AgentProgram::SEL_OCC_ALL) {
      int choices = 0;
      Node& node = m_pointmap->getPoint(m_node).getNode();
      for (int i = 0; i < vbin; i++) {
         if (node.m_occlusion_bins[(directionbin+i)%32].size()) {
            choices += node.m_occlusion_bins[(directionbin+i)%32].size();
         }
      }
      if (choices == 0) {
         if (!wholeisovist) {
            return onStandardLook(false);
         }
         else {
            m_stuck = true;
            m_target_pix = m_node;
            m_target = m_loc;
            return Point2f(0,0);
         }
      }
      else {
         size_t chosen = pafrand() % choices;
         for (; chosen >= node.m_occlusion_bins[ directionbin % 32 ].size(); directionbin++) {
            chosen -= node.m_occlusion_bins[ directionbin % 32 ].size();
         }
         tarpixelate = node.m_occlusion_bins[directionbin % 32].at(chosen);
      }
   }
   else {
      int subset = 1;
      if (looktype == AgentProgram::SEL_OCC_BIN45) {
         subset = 3;
      }
      else if (looktype == AgentProgram::SEL_OCC_BIN60) {
         subset = 5;
      }
      prefvec<wpair> weightmap;
      double weight = 0.0;
      Node& node = m_pointmap->getPoint(m_node).getNode();
      for (int i = 0; i < vbin; i += subset) {
         PixelRef nigpix;
         double fardist = -1.0;
         for (int k = 0; k < subset; k++) {
            for (size_t j = 0; j < node.m_occlusion_bins[(directionbin+i+k)%32].size(); j++) {
               PixelRef pix = node.m_occlusion_bins[(directionbin+i+k)%32].at(j);
               if (dist(pix,m_node) > fardist) {
                  fardist = dist(pix,m_node);
                  nigpix = pix;
               }  
            }
         }
         if (fardist != -1.0) {
            bool cont = true;
            if (looktype == AgentProgram::SEL_OCC_MEMORY) {
               depthmapX::addIfNotExists(m_occ_memory.a(), nigpix);
               // the turn chance (pafrand() % 2) may have to be modified later...
               if (!m_at_target && std::find(m_occ_memory.b().begin(), m_occ_memory.b().end() ,nigpix) != m_occ_memory.b().end()) {
                  cont = false;
               }
            }
            if (cont) {
               switch (looktype) {
               case AgentProgram::SEL_OCC_WEIGHT_DIST:
                  weight += fardist;
                  break;
               case AgentProgram::SEL_OCC_WEIGHT_ANG:
                  weight += (double(vbin-abs(i-vbin)));
                  break;
               case AgentProgram::SEL_OCC_WEIGHT_DIST_ANG:
                  weight += fardist * (double(vbin-abs(i-vbin)));
                  break;
               default:
                  weight += 1.0;
                  break;
               }
               weightmap.push_back(wpair(weight,nigpix));
            }
         }
      }
      if (weightmap.size() == 0) {
         if (!wholeisovist) {
            return onStandardLook(false);
         }
         else {
            m_stuck = true;
            m_target = m_loc;
            return Point2f(0,0);
         }
      }
      else {
         double chosen = prandomr() * weight;
         for (size_t i = 0; i < weightmap.size(); i++) {
            if (chosen < weightmap[i].weight) {
               tarpixelate = weightmap[i].node;
               break;
            }
         }
      }
   }

   m_target_pix = tarpixelate;
   m_target = m_pointmap->depixelate(tarpixelate);

   return (m_target - m_loc).normalise();
}

// note: LOS look uses similar weighted choice mechanism

Point2f Agent::onLoSLook(bool wholeisovist, int look_type)
{
   int bbin = -1;
   if (m_program->m_destination_directed) {
      Point2f vec2 = m_destination - m_loc;
      double test = vec2.length();
      vec2.normalise();
      bbin = binfromvec(vec2);
   }
   int targetbin = -1;
   int vbin = m_program->m_vbin;
   if (wholeisovist || vbin == -1) {
      vbin = 16;
   }
   int directionbin = 32 + binfromvec(m_vector) - vbin;
   prefvec<wpair> weightmap;
   double weight = 0.0;
   // reset for getting list, check in range:
   vbin = vbin * 2 + 1;
   if (vbin > 32) {
      vbin = 32;
   }
   for (int i = 0; i < vbin; i++) {
      double los = (look_type == AgentProgram::SEL_LOS) ?
         m_pointmap->getPoint(m_node).getNode().bindistance( (directionbin + i) % 32 ) :
         m_pointmap->getPoint(m_node).getNode().occdistance( (directionbin + i) % 32 );
      if (m_program->m_los_sqrd) {
         los *= los;
      }
      if (m_program->m_destination_directed) {
         los *= 1.0 - double(binsbetween( ((directionbin + i) % 32), bbin)) / 16.0;
      }
      weight += los;
      weightmap.push_back(wpair(weight,(directionbin + i) % 32));
   }
   if (weight == 0.0) {
      if (!wholeisovist) {
         return onLoSLook(true, look_type);
      }
      else {
         // oops!
         m_stuck = true;
         return Point2f(0,0);
      }
   }
   else {
      double chosen = prandomr() * weight;
      for (size_t i = 0; i < weightmap.size(); i++) {
         if (chosen < weightmap[i].weight) {
            targetbin = weightmap[i].node;
            break;
         }
      }
   }

   float angle = (float)anglefrombin2(targetbin);

   return Point2f( cosf(angle), sinf(angle) );
}

Point2f Agent::onDirectedLoSLook(bool wholeisovist, int look_type)
{
   int bbin = -1;
   Point2f vec2 = m_destination - m_loc;
   vec2.normalise();
   bbin = binfromvec(vec2);
   int targetbin = -1;
   int vbin = m_program->m_vbin;
   if (wholeisovist || vbin == -1) {
      vbin = 16;
   }
   int directionbin = 32 + binfromvec(vec2) - vbin;
   prefvec<wpair> weightmap;
   double weight = 0.0;
   // reset for getting list, check in range:
   vbin = vbin * 2 + 1;
   if (vbin > 32) {
      vbin = 32;
   }
   for (int i = 0; i < vbin; i++) {
      double los = (look_type == AgentProgram::SEL_LOS) ?
         m_pointmap->getPoint(m_node).getNode().bindistance( (directionbin + i) % 32 ) :
         m_pointmap->getPoint(m_node).getNode().occdistance( (directionbin + i) % 32 );
      if (m_program->m_los_sqrd) {
         los *= los;
      }
      weight += los;
      weightmap.push_back(wpair(weight,(directionbin + i) % 32));
   }
   if (weight == 0.0) {
      if (!wholeisovist) {
         return onLoSLook(true, look_type);
      }
      else {
         // oops!
         m_stuck = true;
         return Point2f(0,0);
      }
   }
   else {
      double chosen = prandomr() * weight;
      for (size_t i = 0; i < weightmap.size(); i++) {
         if (chosen < weightmap[i].weight) {
            targetbin = weightmap[i].node;
            break;
         }
      }
   }

   float angle = (float)anglefrombin2(targetbin);

   return Point2f( cosf(angle), sinf(angle) );
}


// Gibsonian agents record their last known information,
// and act according to their rules:

Point2f Agent::onGibsonianLook(bool wholeisovist)
{
   // at start, go in any direction:
   if (wholeisovist) {
      return onLoSLook(true, AgentProgram::SEL_LOS);
   }
   //
   calcLoS(binfromvec(m_vector),true);
   // now, choose action according to type of agent:
   int rule_choice = -1;
   int dir = 0;
   for (int k = 0; k < 4; k++) {
      dir = onGibsonianRule(m_program->m_rule_order[k]);
      if (dir != 0) {
         rule_choice = m_program->m_rule_order[k];
         break;
      }
   }

   float angle = 0.0;

   if (rule_choice != -1) {
      angle = (float)anglefrombin2((binfromvec(m_vector) + (2 * rule_choice + 1) * dir + 32) % 32);
   }

   // if no rule selection made, carry on in current direction
   return (rule_choice == -1) ? m_vector : Point2f( cosf(angle), sinf(angle) );
}

int Agent::onGibsonianRule(int rule)
{
   int option = 0;
   switch (m_program->m_sel_type) {
   case AgentProgram::SEL_LENGTH:
      // rule_threshold from 0 to 100m
      if (m_curr_los[rule + 1] > m_program->m_rule_threshold[rule]) {
         option = 0x01;
      }
      if (m_curr_los[rule + 5] > m_program->m_rule_threshold[rule]) {
         option |= 0x10;
      }
      break;
   case AgentProgram::SEL_OPTIC_FLOW:
      // rule_threshold reflects from 0x (0) to 5x (100.0)
      if ((m_curr_los[rule + 1] + 1) / (m_last_los[rule + 1] + 1) > m_program->m_rule_threshold[rule] / 20.0) {
         option = 0x01;
      }
      if ((m_curr_los[rule + 5] + 1) / (m_last_los[rule + 5] + 1) > m_program->m_rule_threshold[rule] / 20.0) {
         option |= 0x10;
      }
      break;
   case AgentProgram::SEL_COMPARATIVE_LENGTH:
      // rule_threshold reflects from 0x (0) to 10x (100.0)
      if ((m_curr_los[rule + 1] + 1) / (m_curr_los[0] + 1) > m_program->m_rule_threshold[rule] / 10.0) {
         option = 0x01;
      }
      if ((m_curr_los[rule + 5] + 1) / (m_curr_los[0] + 1) > m_program->m_rule_threshold[rule] / 10.0) {
         option |= 0x10;
      }
      break;
   case AgentProgram::SEL_COMPARATIVE_OPTIC_FLOW:
      // rule_threshold reflects from 0x (0) to 10x (100.0)
      if ((m_curr_los[rule + 1] * m_last_los[0] + 1) / (m_last_los[rule + 1] * m_curr_los[0] + 1) >
           m_program->m_rule_threshold[rule] / 10.0) {
         option = 0x01;
      }
      if ((m_curr_los[rule + 5] * m_last_los[0] + 1) / (m_last_los[rule + 5] * m_curr_los[0] + 1) >
           m_program->m_rule_threshold[rule] / 10.0) {
         option |= 0x10;
      }
      break;
   }
   int dir = 0;
   if (option == 0x01 && m_program->m_rule_probability[0] > prandomr()) {
      dir = -1;
   }
   else if (option == 0x10 && m_program->m_rule_probability[0] > prandomr()) {
      dir = +1;
   }
   else if (option == 0x11 && m_program->m_rule_probability[0] > prandomr() * prandomr()) {
      // note, use random * random event as there are two ways to do this
      dir = (rand() % 2) ? -1 : +1;
   }
   return dir;
}

Point2f Agent::onGibsonianLook2(bool wholeisovist)
{
   // at start, go in any direction:
   if (wholeisovist) {
      return onLoSLook(true, AgentProgram::SEL_LOS);
   }
   //
   calcLoS2(binfromvec(m_vector),true);
   int maxbin = 0;
   /*
   // first action: adjust to longest line of sight
   if (m_curr_los[3] > m_curr_los[0]) {
      maxbin = -m_program->m_vahead;
      if (m_curr_los[4] > m_curr_los[0] && (pafrand() % 2)) {
         maxbin = m_program->m_vahead;
      }
   }
   else if (m_curr_los[4] > m_curr_los[0]) {
      maxbin = m_program->m_vahead;
   }
   */
   // second action, apply feeler rule:
   char dir = 0x00;
   if ((m_curr_los[1]-m_last_los[1])/m_curr_los[1] > m_program->m_feeler_threshold) {
      dir |= 0x01;
   }
   if ((m_curr_los[2]-m_last_los[2])/m_curr_los[2] > m_program->m_feeler_threshold) {
      dir |= 0x10;
   }
   if (dir == 0x01 && m_program->m_feeler_probability > prandomr()) {
      maxbin = -m_program->m_vbin;
   }
   else if (dir == 0x10 && m_program->m_feeler_probability > prandomr()) {
      maxbin = m_program->m_vbin;
   }
   else if (dir == 0x11 && m_program->m_feeler_probability > prandomr() * prandomr()) {
      maxbin = (pafrand() % 2) ? m_program->m_vbin : -m_program->m_vbin;
   }
   // third action: detect heading for dead-end
   if (maxbin == 0 && (m_curr_los[0] / m_pointmap->getSpacing() < m_program->m_ahead_threshold)) {
      if (m_curr_los[1] >= m_curr_los[2]) {
         maxbin = -m_program->m_vbin;
      }
      else {
         maxbin = m_program->m_vbin;
      }
   }

   int bin = binfromvec(m_vector) + maxbin;
   float angle = (float)anglefrombin2(bin);

   return (maxbin == 0) ? m_vector : Point2f( cosf(angle), sinf(angle) );
}

void Agent::calcLoS(int directionbin, bool curr)
{
   float *los;
   if (curr) {
      los = m_curr_los;
   }
   else {
      los = m_last_los;
   }
   Node& node = m_pointmap->getPoint(m_node).getNode();
   // ahead
   los[0] = node.bindistance( directionbin % 32 );
   // directions:
   int count = 1;
   for (int i = 1; i <= 7; i += 2) {
      los[count] = node.bindistance((directionbin - i + 32) % 32);
      count++;
   }
   for (int j = 1; j <= 7; j += 2) {
      los[count] = node.bindistance((directionbin + j)  % 32);
      count++;
   }
}

void Agent::calcLoS2(int directionbin, bool curr)
{
   float *los;
   if (curr) {
      los = m_curr_los;
   }
   else {
      los = m_last_los;
   }
   Node& node = m_pointmap->getPoint(m_node).getNode();
   // ahead
   los[0] = node.bindistance(directionbin % 32);
   // directions:
   los[1] = node.bindistance( (directionbin - m_program->m_vbin + 32) % 32);
   los[2] = node.bindistance( (directionbin + m_program->m_vbin) % 32);
   //
   los[3] = node.bindistance( (directionbin - m_program->m_vahead + 32) % 32);
   los[4] = node.bindistance( (directionbin + m_program->m_vahead) % 32);
}
