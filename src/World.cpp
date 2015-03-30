#include "World.h"

#include <ctime>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>

#ifndef IMMUNEBOTS_NOSERIALISATION
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#endif

#include "settings.h"
#include "helpers.h"
#include "vmath.h"

#include "ImmunebotsSetup.h"
#include "View.h"
#include "AbstractAgent.h"
#include "Cell.h"
#include "CTL.h"
#include <vector>

using namespace std;

// Simple sorter for the events (sorts by execution time).
static bool sortEvents( Event first, Event second ) {
	return( first.time < second.time );
}


World::World( ImmunebotsSetup *is ) {
	worldtime = 0.0;
	ibs = is;

	// Generate a unique ID for this world
	UID = randi(0,RAND_MAX);

	// Setup the world colours from the conf
	resetCellColours();

	// Copy over the other variables from settings (manual!)
	CTL_DENSITY              = conf::CTL_DENSITY;
	DEFAULT_NUM_CELLS_TO_ADD = conf::DEFAULT_NUM_CELLS_TO_ADD;

    // Initialise the patch (infected island) and CellShadow layers
	//CellShadowBool = new bool[is->getParm("WIDTH",conf::DefaultWidth)*is->getParm("HEIGHT",conf::DefaultHeight)];
	setWorldDimensions(is->getParm("WIDTH",conf::DefaultWidth), is->getParm("HEIGHT",conf::DefaultHeight));

    // Create the statistics module
	stats = (Statistics*) malloc( sizeof( Statistics ) );
	stats->scan.reserve(5);
	stats->force_refresh = false;

	// Initialise the boundary box (to very wrong numbers)
    resetBoundingBox();
}

World::~World() {
	cout << "Destroying a world. Bye bye.\n";
}

// Horrible clunky solution: copies all values from Conf to the World colour "store"
// Also somewhat deprecated. Not sure this works anymore.
void World::resetCellColours() {
	copyColourArray(COLOUR_NOT_SUSCEPTIBLE, conf::COLOUR_NOT_SUSCEPTIBLE);
	copyColourArray(COLOUR_SUSCEPTIBLE, 	conf::COLOUR_SUSCEPTIBLE);
	copyColourArray(COLOUR_INFECTED, 		conf::COLOUR_INFECTED);
	copyColourArray(COLOUR_CTL, 			conf::COLOUR_CTL);
	copyColourArray(COLOUR_DEAD, 			conf::COLOUR_DEAD);
}

void World::copyColourArray(float wc[], const float cc[]) {
	for (int i=0; i<3; i++)
		wc[i]=cc[i];
}

// This sets a 'true' in the 2D patch array at co-ordinate x,y, and inserts a 1d coord into patchV
// Karen says: I would probably use a map<Pos, bool> for the lot and write my own Position class
void World::setPatch(int x, int y) {
	// We need to maintain two containers
	// Boundary of canvas exceeded - return! (or risk overruns)
	if ( x<0 || y<0 || x>ibs->getParm("WIDTH",0) || y>ibs->getParm("HEIGHT",0) ) return;

	// If there isn't a patch at x,y already, put one there and set the bounding box
	if (!patch[x][y]) {
		patchV.push_back( (x-1)*ibs->getParm("WIDTH",0) + y );
		patch[x][y] = true;
		// Check if the bounding box needs resetting
		checkBoundingBox(x,y);
	}
}

// Given a width and height, sets up the patch and cellshadow datat structures.
// Note: everything in previous patch and cellshadow is overwritten.
void World::setWorldDimensions(int w, int h) {
    for (int x=0;x<w;x++) {
    	patch.push_back( vector<bool>(h,false) );
    	CellShadow.push_back( vector<Cell*>(h,(Cell*)0) );
    	CellShadowBool.push_back( vector<bool>(h,false) );
    }
    // Allocate and clear the CellShadow Vector
    //if (CellShadowBool!=0) delete [] CellShadowBool;
    //CellShadowBool = new bool[w*h];
    //for (int i=0; i<w*h; i++) CellShadowBool[i] = false;
    //CellShadowBool.clear();
    // Clear the patchVector as well
    patchV.clear();
}

void World::resetBoundingBox() {
	bounding_min = Vector2f(ibs->getParm("WIDTH",0),ibs->getParm("HEIGHT",0));
	bounding_max = Vector2f(0,0);

	// Loop through patches
	for (int p=0; p<(int)patchV.size(); p++) {
		checkBoundingBox(patchV[p]/ibs->getParm("WIDTH",0),patchV[p]%ibs->getParm("WIDTH",0));
	}

	// Loop through cells
	for (int c=0; c<(int)cells.size(); c++) {
		checkBoundingBox(cells[c]->pos.x,cells[c]->pos.y);
	}
}

void World::checkBoundingBox(int x,int y) {
	int b = 15;
	if ( x - b < bounding_min.x ) bounding_min.x = max(0, x-b);
	if ( y - b < bounding_min.y ) bounding_min.y = max(0, y-b);
	if ( x + b > bounding_max.x ) bounding_max.x = min(x+b, ibs->getParm("WIDTH", 0));
	if ( y + b > bounding_max.y ) bounding_max.y = min(y+b, ibs->getParm("HEIGHT",0));

	// Set bounding box area (Note: margin is hard-coded as 15um, but as a cell is "detected" at its origin, we take 5um (radius) off).
	// Note: This is fairly specific to ALL cells having a radius of 5um.
	if ( bounding_max.x > bounding_min.x ) {
		stats->area_mm2 = (float)((bounding_max.x - bounding_min.x - 2*(b-5)) * (bounding_max.y - bounding_min.y - 2*(b-5))) / 1000000.0;
	}

}

// Overload the above function (single int, converted to an x,y coord)
void World::setPatch(int i) {
	int x = int(i/ibs->getParm("WIDTH",0));
	int y = i%ibs->getParm("WIDTH",0);
	return setPatch(x,y);
}

float World::getWorldTime() {
	return worldtime;
}

void World::update() {

	// In each update cycle, do the following:
	//  0. Do stats
	//  1. Process time
	//  2. Process events (this may activate agents, introduce new agents, etc)
	//  3. Set inputs for the active agents list, i.e. provide each active agent with requested information, e.g. nearby agents, time
	//  4. Process actions for the active agents list, i.e. allow agents to move/update their position, produce offspring, die
    //  5. Tidy up (remove dead stuff)

	// 0. If time 0, then update stats (should have already been done) and write the report for the first time.
	if (worldtime==0.0) {
		this->updateStats(true);
		this->writeReport(true);
	} else if ( stats->force_refresh || ( int(worldtime) % ibs->getParm("update_stats", 300) ) == 0 ) {
		this->updateStats(false);
		if ( stats->force_refresh || ( int(worldtime) % ibs->getParm("report_time", 300) ) == 0 ) {
			this->writeReport(false);
			stats->force_refresh = false;
		}
	}

    // 1. Process time, including recalculating the timestep
	// Update the worldtime by timestep seconds
	// TODO: The Timestep should be determined by 1/conf::TIMESTEPS# of the the fastest rate
	float timestep = 1.0;
	worldtime += timestep;

	// 2. Process events for this timestep in the EventsList
	int events = checkEvents(worldtime);
	bool doReSort = false;
	while (events>0) {
		// Get current Event, activate and remove from list! If persistent, stick on BACK of list.
		Event currentEvent = EventsList.front();
		activateEvent(&currentEvent);
		EventsList.pop_front();
		// If the current event has not been "fired", then add to back of the list and re-sort later.
		if (!currentEvent.fired) {
			EventsList.push_back( currentEvent );
			doReSort = true;
		}
		events--;
	}
	// Will be true if we stick an event at the end of the events queue
	if (doReSort) { EventsList.sort(sortEvents); }

    // 3. Give input to every agent
    setInputs(timestep);

    // 4. Read output and process consequences of bots on environment
    processOutputs(timestep);

    // 5. Tidy Up
    vector<AbstractAgent*>::iterator iter = agents.begin();
    while ( iter != agents.end() ) {
    	// If this agent is inactive, remove from the vector (and use the newly returned iterator)
    	if ( ((AbstractAgent*)(*iter))->isActive() ) {
    		++iter;
    	} else {
    		iter = agents.erase(iter);
    	}
    }
}

// Must be called by Abstract Agents upon (CTL) lysis or (Cell) becoming infected,
void World::EventReporter(int event_type) {
	switch (event_type) {
		case EVENT_INFECTEDDEATH_LYSIS:
			stats->infected_lysis++;
			stats->infected--;
			stats->dead++;
			break;
		case EVENT_INFECTEDDEATH_VIRUS:
			stats->infected_death++;
			stats->infected--;
			stats->dead++;
			break;
		case EVENT_FAILEDINFECTION:
			stats->failed_infection++;
			break;
		case EVENT_CTL_SCANCOMPLETE:
			stats->scan_complete++;
			break;
		case EVENT_INFECTED_ID:
			stats->iscan_complete++;
			break;

		default: break;
	}
}

// This will clear all cells before placing them on the patches.
// The idea is that the setup mode is used to create the "world", and then the world is saved to a file.
// The saved world consists of patches and (N/S/I) cells, but no rates, CTL or virions (the first 2 are in the setup file).
// SHOULD include CTL as we can start the simulation with them!
void World::placeCells() {

	int r = conf::BOTRADIUS;
	cells.clear();
	// Also clear the "infected" cells from the agents list
	vector<AbstractAgent*>::iterator iter = agents.begin();
	while ( iter != agents.end() ) {
		if ( ((AbstractAgent*)(*iter))->getAgentType() == AbstractAgent::AGENT_INFECTED ) {
			iter = agents.erase(iter);
		} else {
			++iter;
		}
	}

	resetBoundingBox(); // Bounding box may be determined by manually placed cells
	// We therefore reset the bounding box (so it includes only the patches) and then re-place the cells.

	// Step through the patch vector to find the last patch and first patch
	int lastPatch = 0;
	int firstPatch = ibs->getParm("WIDTH",0)*ibs->getParm("HEIGHT",0);
	for ( int p=0; p<(int)patchV.size(); p++ ) {
		if (patchV[p]>lastPatch)  lastPatch  = patchV[p];
		if (patchV[p]<firstPatch) firstPatch = patchV[p];
	}

	// Start placing the cells
	int packoffset = 0;
	for ( int j=r; j<bounding_max.y-r+1; j+=2*r-1) {
		packoffset = ((j-5)%2==0)?0:r; // add this to i to find the x-coord
		for ( int i=r; i<bounding_max.x-r; i+=2*r ) {
			// Is there a patch nearby? For now just check i,j coord itself
			if ( isNearPatch(i+packoffset,j) ) {
				// Drop a new cell here.
				addCell(i+packoffset,j);
			}
		}
	}

	// Update the shadow: have to do it here as there might be cells that were not placed near a patch before (if #patch<#cells).
	resetShadowLayer();
	resetBoundingBox();
}

/*
void World::placeCellsOld() {

	int r = conf::BOTRADIUS;

	// Set up the cell iterator (this releases cells when popped)
	// Need to be careful, as an empty cells will crash things (I think?)
	std::random_shuffle(cells.begin(), cells.end());
	vector<Cell *>::iterator cellsIt = cells.begin();
	bool outOfCells = cells.empty();

	// If in jitter mode, then step through the world
	int packoffset = 0;
	for ( int j=r+1; j<conf::HEIGHT-r+1; j+=2*r-1) {
		packoffset = ((j-5)/10%2)?0:r; // add this to i to find the x-coord
		for ( int i=r; i<conf::WIDTH-r; i+=2*r ) {
			// Is there a patch nearby? For now just check i,j coord itself
			if ( isNearPatch(i+packoffset,j) ) {
				// Drop a cell here.
				// If there are no more cells in cells and automanage is on, then create new. Else return.
				if ( !outOfCells && cellsIt != cells.end() ) {
					// Place current cell here
					(*cellsIt)->pos.x = (i+packoffset);
					(*cellsIt)->pos.y = j;
					++cellsIt;
				} else {
					outOfCells = true;
					// No more cells! Either create new or end the jitter
					if ( autoManageCells ) {
						// Create new cell, pushback onto cells
						//cout << "--- creating new cell!\n";
						addCell(i+packoffset,j);
					} else {
						// No more cells :-( Finito. Need to use a GOTO in order to break out of the double for loop. FACT.
						goto NoMoreCells;
					}
				}
			}
		}
	}

	// For all the other cells (if not placed), stick at random on the world
	// This prevents multiple Jitters from layering all the cells on the patch
	// Arguably, these cells are surplus and should be destroyed.
	if ( !outOfCells ) {
		while ( cellsIt!=cells.end() ) {
			(*cellsIt)->pos.x = randf(0,conf::WIDTH);
			(*cellsIt)->pos.y = randf(0,conf::HEIGHT);
			++cellsIt;
		}
	}

	NoMoreCells:

	// Update the shadow: have to do it here as there might be cells that were not placed near a patch before (if #patch<#cells).
	resetShadowLayer();
}
*/

// Checks if there is a patch within 4 pixels of (x,y)
bool World::isNearPatch(int x, int y) {
	int r = conf::BOTRADIUS;
	for (int i=max(0,x-4*r); i<min(bounding_max.x, x+4*r); i++) {
		for (int j=max(0,y-4*r); j<min(bounding_max.y, y+4*r); j++) {
			if ( patch[i][j] == 1 ) return true;
		}
	}
	return false;
}

// Given a cell type, returns the colour array
float * World::getCellColourFromType(int ct) {
	switch(ct) {
		case AbstractAgent::AGENT_NOT_SUSCEPTIBLE: 		return(COLOUR_NOT_SUSCEPTIBLE);
		case AbstractAgent::AGENT_SUSCEPTIBLE: 			return(COLOUR_SUSCEPTIBLE);
		case AbstractAgent::AGENT_INFECTED: 			return(COLOUR_INFECTED);
		case AbstractAgent::AGENT_INFECTED_NOVIRIONS:	return(COLOUR_INFECTED);
		case AbstractAgent::AGENT_CTL: 					return(COLOUR_CTL);
		case AbstractAgent::AGENT_DEADCELL: 			return(COLOUR_DEAD);
	};

	return(0);
}

void World::setInputs(float timestep) {
	for (int i=0;i<(int)agents.size();i++) {
		agents[i]->setInput(timestep, this);
	}
}

void World::processOutputs(float timestep) {
	// Loop through all active agents
	for (int i=0;i<(int)agents.size();i++) {
		agents[i]->doOutput(timestep, this);
	}
}

//#pragma omp parallel for

void World::addRandomCells(int num) {

	// If num is 0 then use the World::DEFAULT_NUM_CELLS_TO_ADD
	if (num==0)
		num = DEFAULT_NUM_CELLS_TO_ADD;

    for (int i=0;i<num;i++) {
    	addCell(-1,-1);
    }

}

// Add an agent to the world
void World::addAgent( AbstractAgent *a ) {
	agents.push_back( a );
}


void World::addCell(int x, int y) {
	Cell * c = new Cell(x,y,ibs);
	// Check the bounding box
	checkBoundingBox(c->pos.x,c->pos.y);
	// Set probability that this cell is susceptible: Generate number between 0.00 and 100.00
	c->setSusceptible( ibs->getParm("nc_susceptiblepc", 2.0f) );
	// Do the other required stuff (set ID, push onto Cell vector)
	cells.push_back(c);
    // Update the cell shadow layer with every point this cell takes up, with a call to setCellShadow(c.pos.x, c.pos.y, r)
    setCellShadow(c);
}

void World::addCTL(int x, int y) {
	//cout << " - Placing CTL at ("<<x<<","<<y<<")\n";
	// Add the CTL cell at the required place
	CTL *ctl = new CTL(x,y, ibs);
	this->addAgent( ctl );
}

void World::addEvent(Event e) {
	EventsList.push_back(e);
	EventsList.sort( sortEvents );

	// Print it out
	cout << " - adding ";
	e.prettyprint();
	cout << endl;
}

// Given a time, return the number of events hanging on that time
int World::checkEvents(int t) {
	int numEvents = 0;
	list<Event>::iterator it;
	for ( it = EventsList.begin(); it != EventsList.end(); it++ ) {
		if ( (*it).time <= t ) {
			numEvents++;
		} else {
			// First element happens after t, so break and return 0
			break;
		}
	}
	return numEvents;
}

// Given an event, run it now! Should really be "event.activate()", but oh well.
void World::activateEvent(Event *e) {

	// 1. Figure out how many of the desired type (sually CTL or Infected) to add (either absolute or ratio)
	int number = 0;
	int currentnumber = 0;

	switch (e->event) {
		case EVENTSLIST_ADDETRATIO:
			// E:T depends on the number of Infected cells
			//this->updateStats(false); // force update: NB. don't need full update, only Infected cells
			currentnumber = getCurrentPopulation(AbstractAgent::AGENT_INFECTED) + getCurrentPopulation(AbstractAgent::AGENT_INFECTED_NOVIRIONS);
			number = floor(currentnumber * e->number + 0.5); // Get ratio and round to nearest whole int
			break;
		case EVENTSLIST_ADDABSOLUTE:
			number = floor(e->number);
			break;
		case EVENTSLIST_ADDWHEN:
			// This only activates if the condition is true (usually "Infected cell count is x")
			// Return if the condition is reached, else continue, drop the required number of agents and mark event as fired.
			//this->updateStats(false); // force update: don't need full update, only <condition_type> cells
			currentnumber = getCurrentPopulation(e->condition_type);
			switch (e->condition_type) {
				case AbstractAgent::AGENT_CTL: 					if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_INFECTED:				if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_INFECTED_NOVIRIONS:	if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_NOT_SUSCEPTIBLE:		if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_SUSCEPTIBLE:			if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_VIRION:				if (currentnumber < e->condition_number) {return;} else {break;}
				default: return;
			}
			number = floor(e->number);
			break;
		case EVENTSLIST_ENDAFTER:
			// This, like ADDWHEN, will be checked every second, but only activates if the condition is true.
			// Deliberate fallthrough!
		case EVENTSLIST_ENDTIME:
			currentnumber = getCurrentPopulation(e->condition_type);
			switch (e->condition_type) {
				case AbstractAgent::AGENT_CTL: 					if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_INFECTED:				if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_INFECTED_NOVIRIONS:	if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_NOT_SUSCEPTIBLE:		if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_SUSCEPTIBLE:			if (currentnumber < e->condition_number) {return;} else {break;}
				case AbstractAgent::AGENT_VIRION:				if (currentnumber < e->condition_number) {return;} else {break;}
				default: return;
			}
			break;
	}

	// 2. Do the required action. If it's an ADD event, then add X of the appropriate type.
	// If it's the EndWhen event, then stick the correct trigger in ibs

	if (e->event == EVENTSLIST_ENDAFTER) {
		switch (e->agent) {
			case AbstractAgent::AGENT_INFECTED: //deliberate fall-through
			case AbstractAgent::AGENT_INFECTED_NOVIRIONS:
				ibs->setParm("end_infected_le",1);
				ibs->setParm("end_infected_levalue", e->number );
				ibs->setParm("update_stats", 1);
				break;
		}
	} else if (e->event == EVENTSLIST_ENDTIME) {
		// Change end time to new end time (note: overwrites old endtime)
		ibs->endTime = worldtime + e->number;
		//cout << "";
	} else if (e->event != EVENTSLIST_ENDAFTER) {
		switch (e->agent) {
			case AbstractAgent::AGENT_CTL:
				this->dropCTL(number);
				stats->ctl = stats->ctl + number;
				break;
			case AbstractAgent::AGENT_SUSCEPTIBLE:
				cout << "Add susceptible cells doesn't currently work! Fix this!" << endl;
				stats->susceptible = stats->susceptible + number;
				break;
			default:
				cout << "ERROR: Cannot add required cell type ("<< e->agent <<")! Fix this!" << endl;
				break;
		}
	}

	// If we've reached here then the event has been activated.
	e->fired = true;
	stats->force_refresh = true; /* force a stats and report refresh as an event has triggered */

	// Print it out..
	cout << " - ["<<worldtime<<"] Activating "; e->prettyprint(); cout << endl;
}

int World::getCurrentPopulation(int agent_type) {

	int population = 0;

	vector<Cell*>::iterator cit;
	vector<AbstractAgent*>::iterator agent;

	// Magic switch - treat infected as infected_novirions
	if (ibs->getParm("ic_novirions", 0) & agent_type==AbstractAgent::AGENT_INFECTED) { agent_type = AbstractAgent::AGENT_INFECTED_NOVIRIONS; }

	switch (agent_type) {

		// Fall through #1: The cells vector for all cells (Dead, NS, S and I)
		case AbstractAgent::AGENT_DEADCELL:
		case AbstractAgent::AGENT_NOT_SUSCEPTIBLE:
		case AbstractAgent::AGENT_INFECTED:
		case AbstractAgent::AGENT_INFECTED_NOVIRIONS:
			for ( cit = cells.begin(); cit != cells.end(); ++cit) {
				if ( ((AbstractAgent*)(*cit))->getAgentType() == agent_type ) population++;
			}
			break;

		// Fall through #2: The agents vector for ctl and virions
		case AbstractAgent::AGENT_CTL:
		case AbstractAgent::AGENT_VIRION:
			for ( agent = agents.begin(); agent != agents.end(); ++agent) {
				// Increment the right counter
				switch ( ((AbstractAgent*)(*agent))->getAgentType() == agent_type ) population++;
			}
			break;
	}

	return(population);
}

void World::resetShadowLayer() {
	// First, reset to 0
	for (int x=0;x<ibs->getParm("WIDTH",0);x++) {
		for (int y=0;y<ibs->getParm("HEIGHT",0);y++) {
			CellShadow[x][y] = 0;
			CellShadowBool[x][y] = false;
		}
	}
	//for (int i=0; i<ibs->getParm("WIDTH",0)*ibs->getParm("HEIGHT",0); i++) CellShadowBool[i] = false;

	// Then, setup through the cells vector
	for (int i=0;i<(int)cells.size();i++) {
		setCellShadow( cells[i] );
	}
}

void World::setCellShadow( Cell * c ) {
	_setCellShadow(c,c);
}

// Given a Cell*, sets the address of the Cell in the 2D "World" shadow, CellShadow[][].
void World::_setCellShadow(Cell *c, Cell * value) {
	// Some handy variables to speed stuff up
	float rsq = pow(c->radius,2);
	int max_x = min(bounding_max.x, int(c->pos.x+c->radius));
	int max_y = min(bounding_max.y, int(c->pos.y+c->radius));

	// Loop through the bounding box and put the reference to the cell in CellShadow[][] if it's "within" the circle
	for (int x=max(0,int(c->pos.x-c->radius));x<max_x; x++ ) {
		for (int y=max(0,int(c->pos.y-c->radius));y<max_y; y++ ) {
			if ( ( pow(c->pos.x-x,2) + pow(c->pos.y-y,2) ) < rsq ) {
				// Now add c at CellShadow[x][y]
				CellShadow[x][y] = value;
				CellShadowBool[x][y] = true;
				//CellShadowBool[(x-1)*ibs->getParm("HEIGHT",0)+(y-1)] = true;
			}
		}
	}
}

// When called by the user through the GUI, will iterate through all cells and reset the susceptibility chance
void World::resetCellSusceptibility() {
	for (int i=0;i<(int)cells.size();i++) {
		if ( cells[i]->agent_type != AbstractAgent::AGENT_CTL || cells[i]->agent_type != AbstractAgent::AGENT_INFECTED ) {
			// If not infected or CTL, then reset to 0 and invoke chance of susceptibility
			cells[i]->agent_type = AbstractAgent::AGENT_NOT_SUSCEPTIBLE;
			cells[i]->setSusceptible( ibs->getParm("nc_susceptiblepc", 2.0f) );
		}
	}
}

// When called by the user through the GUI, will drop the required density (ctl/mm2)
void World::dropCTL() {
	// Add a number of CTL within the bounding box
	int number_ctl = round(CTL_DENSITY * stats->area_mm2);
	dropCTL(number_ctl);
}

void World::dropCTL(int ctl_number) {
	int x,y;
	for (int i=0; i<ctl_number; i++) {
		x = randi(bounding_min.x,bounding_max.x);
		y = randi(bounding_min.y,bounding_max.y);
		addCTL(x, y);
	}
}

// When we clear cells, we need to remove all cells from the iterator, the selectedCell, inFocusCell AND reset the ShadowGrid
void World::clearAllCells() {
	agents.clear();
	cells.clear();
	resetShadowLayer();
	worldtime = 0.0;
	// Also reset the bounding box (depends on the patch vector)
	resetBoundingBox();
}

// Check if the given coord is over a cell, return false (0) if not, the cell pointer if so.
bool World::isOverCell(int x, int y) {
	if (x>0 && x<ibs->getParm("WIDTH",0) && y>0 && y<ibs->getParm("HEIGHT",0)) {
		return( CellShadowBool[x][y] );
		//return( CellShadowBool[(x-1)*ibs->getParm("HEIGHT",0)+(y-1)] );
	}
	return(false);
}

Cell * World::getCell(int x, int y) {
	return( CellShadow[x][y] );
}

// If we click on a non-susceptible or susceptible cell, we infect it
// If we click on an infected cell, we remove the infection
void World::toggleInfection(int x, int y) {
	if (isOverCell(x,y)) {
		Cell * c = getCell(x,y);
		c->toggleInfection();
		if ( c->isInfected() ) {
			// Add to ActiveAgent queue
			agents.push_back(c);
		}
	}
}

// 2nd parameter "onlySusceptible" relates to whether we add new infected cells, or turn susceptible cells to infected status.
void World::infectCells(int total, bool onlySusceptible) {
	int infectedcount = 0;

	vector<Cell*>::iterator cell;

	random_shuffle(cells.begin(), cells.end());

	cell = cells.begin();
	while (infectedcount<total && cell != cells.end() ) {
		if (!(*cell)->isInfected()) {
			if ((onlySusceptible && (*cell)->isSusceptible()) || !onlySusceptible) {
				(*cell)->toggleInfection();
				// Handle special "ODE-style" infected cells
				if (ibs->getParm("ic_novirions",0)==1) {
					(*cell)->setCellType(AbstractAgent::AGENT_INFECTED_NOVIRIONS);
				}
				agents.push_back(*cell);
				infectedcount++;
			}
		}
		cell++;
	}
}

// Saves most of the Stats module to a csv
void World::writeReport(bool firstTime) {

	//cout << "   - writeReport also called ["<<worldtime<<"s]"<<endl;

	// Bit nasty and inelegant having this code here, but it's only needed when we write out to csv.
	if ( ibs->getParm("stats_extra",0) ) {
		// Empty the 'fraction of time spent in each state' vector
		stats->ctlsf.assign(3,0);

		// Don't continue unless we have any CTL
		if ( stats->ctl > 0 ) {

			// Create a 3-rowed 2D vector (note: empty).
			vector< vector<float> > ctlsfull( CTL::STATE_MAX, vector<float>() );

			vector<AbstractAgent*>::iterator agent;
			for ( agent = agents.begin(); agent != agents.end(); ++agent) {
				if ( ((AbstractAgent*)(*agent))->getAgentType() == AbstractAgent::AGENT_CTL ) {
				// Work out the fraction of time each CTL spent in each of the 3 states.
					((CTL*)(*agent))->getStateFraction( ctlsfull );
				}
			}

			// Aggregate the CTL fractional state vector (ctlsfull) into ctlsf
			vector<float> localsf(CTL::STATE_MAX, 0);

			for (int i=0; i < CTL::STATE_MAX; i++ ) {
				for( int j = 0 ; j < ctlsfull[i].size() ; j++ ) {
					localsf[i] += ctlsfull[i][j];
				}
			}

			// And now divide through by ctlsfull.size()
			for (int i=0; i<CTL::STATE_MAX; i++) stats->ctlsf[i] = localsf[i] / ctlsfull[i].size();
		}
	}


	char buffer[255];
	// TODO: Check that logs/ exist
	sprintf(buffer, "%s/%s.csv", ibs->reportdir.c_str(), ibs->reportfilename.c_str());

	ofstream logfile;

	if (firstTime) {
		// Check if the file already exists
		cout << " - Report file is located at: " << buffer << endl;
		ifstream testfile(buffer);
		if ( testfile ) {
			if ( ibs->getParm("report_overwrite",0)==0  ) {
				testfile.close();
				cout << "ERROR: Logfile ("<<buffer<<") already exists and overwrite bit (OverwriteReport) not set in setupfile. Exiting." << endl;
				exit(EXIT_SUCCESS);
			}
		// Otherwise, just close the testfile - writing the header will overwrite the file.
		testfile.close();
		}

		// Open file normally (will overwrite everything)
		logfile.open(buffer);
		// Write out the header
		logfile << "time,nonsusceptible,susceptible,infected,dead,virions,ctl";
		if (ibs->getParm("stats_extra",0)) {
			// Add any other stats output headers to the logfile
			logfile << ",scan_avg,ctl_sm,ctl_ss,ctl_sl";
		}
		logfile << endl;
	} else {
		// Just open the (pre-existing) file for appending.
		logfile.open(buffer, fstream::app);
	}

	// Write-out the stats (currently write out everything)
	logfile << worldtime <<","<< stats->notsusceptible << "," << stats->susceptible <<","<< stats->infected <<","<< stats->dead <<","<< stats->virion <<","<< stats->ctl;
	if ( ibs->getParm("stats_extra",0) ) {
		// Add any other stats output headers to the logfile
		logfile << "," << stats->scan_average;
		for (int i=0; i<CTL::STATE_MAX;i++) logfile << "," << stats->ctlsf[i];
	}
	logfile << endl;

	logfile.close();

}


void World::reset() {
	setWorldDimensions(ibs->getParm("WIDTH",0),ibs->getParm("HEIGHT",0));

	cells.clear();
    agents.clear();

    worldtime = 0.0;

	bounding_min = Vector2f(-1,-1);
	bounding_max = Vector2f(-1,-1);

	updateStats(true);

}

/* This gets called to draw the background things of the world: bounding box, "worldspace" and patches (latter only if in setup mode) */
#ifndef IMMUNEBOTS_NOGL
void World::drawBackground(View* view, bool simulationmode) {

	// 1. Draw the setup/simulaton specific stuff
	// If in setup mode, draw the patches and entire worldspace (cyan)
	// If in simulation mode, only draw the background of the bounding box (and not the whole matrix)
	if ( !simulationmode ) {
		// Setup mode
		// a. Draw the worldspace in cyan
		glBegin(GL_QUADS);
		//glColor4f(0.1,0.9,1.0,0.5);
		glColor4f(0,0,0,0.5);
		glVertex3f(0,0,-conf::FAR_PLANE);
		glVertex3f(0,ibs->getParm("HEIGHT",0),-conf::FAR_PLANE);
		glVertex3f(ibs->getParm("WIDTH",0),ibs->getParm("HEIGHT",0),-conf::FAR_PLANE);
		glVertex3f(ibs->getParm("WIDTH",0),0,-conf::FAR_PLANE);
		glEnd();

		// b. Draw the patches
		for (int i=0; i<(int)patchV.size(); i++) {
			view->drawDot( int(patchV[i]/ibs->getParm("WIDTH",0)), (patchV[i]%ibs->getParm("WIDTH",0)), (-conf::FAR_PLANE+1.0) );
		}
	} else {
		// Simulation mode
		// a. Draw the bounding box quad in cyan
		glBegin(GL_QUADS);
		//glColor4f(0.1,0.9,1.0,0.5);
		glColor4f(1,1,1,0.5);
		glVertex3f(bounding_min.x,bounding_min.y,-conf::FAR_PLANE);
		glVertex3f(bounding_min.x,bounding_max.y,-conf::FAR_PLANE);
		glVertex3f(bounding_max.x,bounding_max.y,-conf::FAR_PLANE);
		glVertex3f(bounding_max.x,bounding_min.y,-conf::FAR_PLANE);
		glEnd();
	}

	// 2. Draw the bounding box outline
	if ( bounding_max.x > 0 ) {
		glBegin(GL_LINES);
		glColor4f(0.5,0.5,0.5,0.5);
		glVertex3f(bounding_min.x,bounding_min.y,-conf::FAR_PLANE+1.0);
		glVertex3f(bounding_max.x,bounding_min.y,-conf::FAR_PLANE+1.0);

		glVertex3f(bounding_max.x,bounding_min.y,-conf::FAR_PLANE+1.0);
		glVertex3f(bounding_max.x,bounding_max.y,-conf::FAR_PLANE+1.0);

		glVertex3f(bounding_max.x,bounding_max.y,-conf::FAR_PLANE+1.0);
		glVertex3f(bounding_min.x,bounding_max.y,-conf::FAR_PLANE+1.0);

		glVertex3f(bounding_min.x,bounding_max.y,-conf::FAR_PLANE+1.0);
		glVertex3f(bounding_min.x,bounding_min.y,-conf::FAR_PLANE+1.0);
		glEnd();
	}
}
#endif

#ifndef IMMUNEBOTS_NOGL
void World::drawForeground(View* view) {

	// 1. Iterate through the cells vector and draw each one
	vector<Cell*>::iterator cit;
	for ( cit = cells.begin(); cit != cells.end(); ++cit) {
		view->drawCell(*cit);
	}

	// 2. Iterate through the CTL vector and draw each one
    vector<AbstractAgent*>::const_iterator ait;
    for ( ait = agents.begin(); ait != agents.end(); ++ait) {
    	// Only draws the agent if it's within the world bounding box
    	view->drawAgent(*ait); // note: this just calls ait->drawAgent(view);
    }

}
#endif

int World::numAgents() const {
    return agents.size();
}

int World::numCells() const {
	return cells.size();
}

// Will save the patch, cell and agent info to a file.
// TODO: Check if we overwrite an existing file
void World::saveLayout() {

#ifdef IMMUNEBOTS_NOSERIALISATION
	cout << "WARNING: Tried to save layout, but boost::serialisation has been removed. Ignoring." << endl;
#endif

#ifndef IMMUNEBOTS_NOSERIALISATION
	ofstream setupfile(ibs->layoutfilename);
	if (!setupfile.good()) {
		cout << "ERROR: Could not make setup file '"<< ibs->layoutfilename <<"'; setup save failed.\n";
		return;
	}

	/*
	// Ensure there is at least one agent which is an infected cell
	// Can't remember why I even check for this, infected cells are currently lost between saves.
	bool haveAnInfectedCell = false;
	for (int a=0; a<(int)agents.size(); a++) {
		if ( agents[a]->getAgentType().compare("Cell") == 0 ) {
			// Cast to a Cell and check if infective
			if ( ((Cell*)(agents[a]))->isInfected() ) {
				// Break out of loop
				haveAnInfectedCell = true;
				break;
			}
		}
	}
	if (!haveAnInfectedCell) cout << "WARNING: No infected cells in the layout.\n";

	*/

	// Explicit registration of AbstractAgent, CTL and Cell

    {	// save setup data, i.e. certain parts of the world class, to archive (parts which are saved are in the header file)
		boost::archive::text_oarchive oa(setupfile);
		oa << (*this);
    } 	// archive and stream closed when destructors are called
#endif

}

// Will load a world setup from the given filename
// Note: probably unecessary to pass in the view pointer, as view *calls* this function.
void World::loadLayout() {

#ifdef IMMUNEBOTS_NOSERIALISATION
	cout << "WARNING: Tried to load layout, but boost::serialisation has been removed. Ignoring." << endl;
#endif

#ifndef IMMUNEBOTS_NOSERIALISATION
	// Create a link to the setup file and read it the contents
	string fn = ibs->layoutfilename;
	ifstream setupfile(fn);
	if (!setupfile.good()) {
		cout << "ERROR: Setupfile '"<< fn <<"' does not exist in working dir, setup failed.\n";
		return;
	}

	World wyrd; // The wyrd world will temporarily store the world we saved from set up
	// We don't need to set the ImmunebotsSetup pointer

	{
		boost::archive::text_iarchive ia(setupfile);
		ia >> wyrd;
	}

	// Now re-assign cells and patch (will overwrite cells, agents and patches)
	cells  = wyrd.cells;
	patchV = wyrd.patchV;
	agents = wyrd.agents;

	//bounding_min = wyrd.bounding_min;
	//bounding_max = wyrd.bounding_max;
	resetBoundingBox();

	// Reset the shadow layer
	resetShadowLayer();

	// TODO: This takes too long! But I don't know how to speed it up.
	// Reset and regenerate the patch[x][y] matrix from the vector.
	for (int i=0;i<conf::WIDTH;i++) for (int j=0;j<conf::HEIGHT;j++) patch[i][j] = 0;
	for (int p=0;p<(int)patchV.size();p++) {
		patch[int(patchV[p]/conf::WIDTH)][patchV[p]%conf::WIDTH] = true;
	}

	worldtime = 0.0;
	this->updateStats(true);
#endif
}

/* updateStatsFull:
 *
 * This  function counts the number of agents in the system by type,
 * and then calls updateStats (with reset enabled). updateStats works
 * out the cell densities and the 5-min averaged scan rate.
 *
 */
void World::updateStatsFull() {

	int total_cells = 0, ctl = 0, notsusceptible = 0, infected = 0, susceptible = 0, dead = 0, virion = 0;

	/* CELLS: INFECTED, SUSCEPTIBLE AND DEADCELLS */
	vector<Cell*>::iterator cit;
	for ( cit = cells.begin(); cit != cells.end(); ++cit) {
		// Increment the right counter
		switch ( ((AbstractAgent*)(*cit))->getAgentType() ) {
			case AbstractAgent::AGENT_NOT_SUSCEPTIBLE:
				notsusceptible++;
				break;
			case AbstractAgent::AGENT_SUSCEPTIBLE:
				susceptible++;
				break;
			case AbstractAgent::AGENT_INFECTED: // intentional fall through
			case AbstractAgent::AGENT_INFECTED_NOVIRIONS:
				infected++;
				break;
			case AbstractAgent::AGENT_DEADCELL:
				dead++;
				break;
			default: break;
		}
		total_cells++;
	}

	/* CTL AND VIRIONS */
	vector<AbstractAgent*>::iterator agent;
	for ( agent = agents.begin(); agent != agents.end(); ++agent) {
		// Increment the right counter
		switch ( ((AbstractAgent*)(*agent))->getAgentType() ) {
			case AbstractAgent::AGENT_CTL:
				ctl++;
				total_cells++;
				break;
			case AbstractAgent::AGENT_VIRION:
				virion++;
				break;
			default: break;
		}
	}

	// Update the absolute cell/virion numbers
	stats->ctl 			  = ctl;
	stats->total_cells 	  = total_cells;
	stats->notsusceptible = notsusceptible;
	stats->susceptible 	  = susceptible;
	stats->infected 	  = infected;
	stats->dead 		  = dead;
	stats->virion		  = virion;

	World::updateStats(true);

}

/*
 * Called from:
 *  - update() when the reporting interval is reached [writeReport is subsequently called]
 *  - some event checks which rely on exact numbers of infected cells
 *  - when *something* sets stats->force_refresh to TRUE
 *
 * updateStats works out the cell densities and the 5-min averaged scan rate.
 *
 */
void World::updateStats(bool resetEvents) {

	//cout << " [DEBUG] UpdateStats called at "<<worldtime<<"s. Intervals are: " << ibs->getParm("update_stats",300) <<"s (stats) and " << ibs->getParm("report_time",300) << "s (report)."<< endl;

	// This should only be called at the very beginning of the simulation.
	if (resetEvents) {
		stats->infected_death = 0;
		stats->infected_lysis = 0;
		stats->scan_complete  = 0;
		stats->scan_average   = 0.0;
		stats->lastCalled     = worldtime;
		stats->scan.assign(5,-1);
	}

	// Every time update is called, calculate the avg cell scan rate per ctl. Smooth over reporting period * 5.
	// Limit this to once every minute, at least!
	if ( stats->lastCalled + 60 <= worldtime ) {
		int fiveminscan = 0; int fivemindiv = 1;
		for (int i=4; i>0; i--) {
			stats->scan[i] = stats->scan[i-1];
			if (stats->scan[i]>=0) {
				fivemindiv++;
				fiveminscan += stats->scan[i];
			}
		}
		stats->scan[0] = stats->scan_complete;
		fiveminscan   += stats->scan_complete;
		//cout << "[DEBUG:"<<worldtime<<"] For this reporting interval, "<< stats->scan_complete << " cells were scanned (";
		stats->scan_complete = 0;
		// Now calculate the (reporting_time*5) smoothed average
		stats->scan_average = fiveminscan / fivemindiv;

		// Calculate the average number of cells scanned every minute, reset every 5 mins.
		if ( stats->ctl == 0 ) {
			stats->scan_average = 0;
		} else {
			stats->scan_average = stats->scan_average * 60.0 / (worldtime - stats->lastCalled) / float(stats->ctl);
		}

		stats->lastCalled = worldtime;
	}

	// Can't update the events here

	// Update the density calculations
	stats->ctl_per_mm2    = 0.0;
	stats->virion_per_mm2 = 0.0;
	if (stats->area_mm2>0.0) {
		stats->ctl_per_mm2    = stats->ctl/stats->area_mm2;
		stats->virion_per_mm2 = stats->virion/stats->area_mm2;
	}

	stats->ctl_per_cell = 0.0;
	stats->virion_per_cell = 0.0;
	int nonctlcells = stats->total_cells - stats->ctl;
	if (nonctlcells>0) {
		stats->ctl_per_cell    = stats->ctl/(float)nonctlcells;
		stats->virion_per_cell = stats->virion/(float)nonctlcells;
	}

	stats->ctl_per_infected = 0.0;
	if (stats->infected > 0) {
		stats->ctl_per_infected = stats->ctl/(float)stats->infected;
	}

	stats->cell_area = nonctlcells * M_PI * pow(conf::BOTRADIUS,2) / 1000000.0;

}

