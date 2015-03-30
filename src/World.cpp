#include "World.h"

#include <ctime>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>


#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

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

World::World(  ) {
	worldtime = 0.0;

	// Generate a unique ID for this world
	UID = randi(0,RAND_MAX);

	// Setup the world colours from the conf
	resetCellColours();

	// Copy over the other variables from settings (manual!)
	CTL_DENSITY              = conf::CTL_DENSITY;
	DEFAULT_NUM_CELLS_TO_ADD = conf::DEFAULT_NUM_CELLS_TO_ADD;

    // Initialise the patch (infected island) and CellShadow layers
    for (int x=0;x<conf::WIDTH;x++) {
    	patch.push_back( vector<bool>(conf::HEIGHT,false) );
    	CellShadow.push_back( vector<Cell*>(conf::HEIGHT,(Cell*)0) );
    }

    // Clear the patchVector as well
    patchV.clear();

    // Create the statistics module
	stats = (Statistics*) malloc( sizeof( Statistics ) );
	updateStats(true);

    // Initialise the boundary box (to very wrong numbers)
    resetBoundingBox();
}

void World::setImmunebotsSetup(ImmunebotsSetup* is) {
	ibs = is;
}

World::~World() {
	cout << "Destroying a world. Bye bye\n";
}

// Horrible clunky solution: copies all values from Conf to the World colour "store"
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
	if ( x<0 || y<0 || x>conf::WIDTH || y>conf::HEIGHT ) return;

	// If there isn't a patch at x,y already, put one there and set the bounding box
	if (!patch[x][y]) {
		patchV.push_back( (x-1)*conf::WIDTH + y );
		patch[x][y] = true;
		// Check if the bounding box needs resetting
		checkBoundingBox(x,y);
	}
}

void World::resetBoundingBox() {
	bounding_min = Vector2f(conf::WIDTH,conf::HEIGHT);
	bounding_max = Vector2f(0,0);

	// Loop through patches
	for (int p=0; p<(int)patchV.size(); p++) {
		checkBoundingBox(patchV[p]/conf::WIDTH,patchV[p]%conf::WIDTH);
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
	if ( x + b > bounding_max.x ) bounding_max.x = min(x+b, conf::WIDTH);
	if ( y + b > bounding_max.y ) bounding_max.y = min(y+b, conf::HEIGHT);

	// Set bounding box area (Note: margin is hard-coded as 15um, but as a cell is "detected" at its origin, we take 5um (radius) off).
	// Note: This is fairly specific to ALL cells having a radius of 5um.
	if ( bounding_max.x > bounding_min.x ) {
		stats->area_mm2 = (float)((bounding_max.x - bounding_min.x - 2*(b-5)) * (bounding_max.y - bounding_min.y - 2*(b-5))) / 1000000.0;
	}

}

// Overload the above function (single int, converted to an x,y coord)
void World::setPatch(int i) {
	int x = int(i/conf::WIDTH);
	int y = i%conf::WIDTH;
	return setPatch(x,y);
}

float World::getWorldTime() {
	return worldtime;
}

void World::update(bool writeReport) {

	// In each update cycle, do the following:
	//  1. Process time
	//  2. Process events (this may activate agents, introduce new agents, etc)
	//  3. Set inputs for the active agents list, i.e. provide each active agent with requested information, e.g. nearby agents, time
	//  4. Process actions for the active agents list, i.e. allow agents to move/update their position, produce offspring, die
    //  5. Tidy up (remove dead stuff)

	// Write the report at time 0 and every 60 (? seems to be 50) thereafter.
	if (writeReport || worldtime==0.0) {
		this->writeReport(worldtime==0.0);
	}

    // 1. Process time, including recalculating the timestep
	// Update the worldtime by timestep seconds
	// TODO: The Timestep should be determined by 1/conf::TIMESTEPS# of the the fastest rate
	float timestep = 1.0;
	worldtime += timestep;

	// TODO: 2. Process events for this timestep in the (currently non-existent) EventsQueue

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
			break;
		case EVENT_INFECTEDDEATH_VIRUS:
			stats->infected_death++;
			break;
		case EVENT_FAILEDINFECTION:
			stats->failed_infection++;
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
	int firstPatch = conf::WIDTH*conf::HEIGHT;
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
		case AbstractAgent::AGENT_NOT_SUSCEPTIBLE: 	return(COLOUR_NOT_SUSCEPTIBLE);
		case AbstractAgent::AGENT_SUSCEPTIBLE: 		return(COLOUR_SUSCEPTIBLE);
		case AbstractAgent::AGENT_INFECTED: 		return(COLOUR_INFECTED);
		case AbstractAgent::AGENT_CTL: 				return(COLOUR_CTL);
		case AbstractAgent::AGENT_DEADCELL: 		return(COLOUR_DEAD);
	};

	return(0);
}

void World::setInputs(float timestep) {
	for (int i=0;i<(int)agents.size();i++) {
		agents[i]->setInput(timestep, this);
	}
}

/* Scriptbots version: DEPRECATED
void World::setInputs() {

    //P1 R1 G1 B1 FOOD P2 R2 G2 B2 SOUND SMELL HEALTH P3 R3 G3 B3 CLOCK1 CLOCK 2 HEARING     BLOOD_SENSOR
    //0   1  2  3  4   5   6  7 8   9     10     11   12 13 14 15 16       17      18           19

    float PI8=M_PI/8/2; //pi/8/2
    float PI38= 3*PI8; //3pi/8/2
    for (int i=0;i<agents.size();i++) {
        Agent* a= &agents[i];

        //HEALTH
        a->in[11]= cap(a->health/2); //divide by 2 since health is in [0,2]

        //FOOD
        int cx= (int) a->pos.x/conf::CZ;
        int cy= (int) a->pos.y/conf::CZ;
        a->in[4]= food[cx][cy]/conf::FOODMAX;

        //SOUND SMELL EYES
        float p1,r1,g1,b1,p2,r2,g2,b2,p3,r3,g3,b3;
        p1=0;
        r1=0;
        g1=0;
        b1=0;
        p2=0;
        r2=0;
        g2=0;
        b2=0;
        p3=0;
        r3=0;
        g3=0;
        b3=0;
        float soaccum=0;
        float smaccum=0;
        float hearaccum=0;

        //BLOOD ESTIMATOReclipse
        float blood= 0;

        for (int j=0;j<agents.size();j++) {
            if (i==j) continue;
            Agent* a2= &agents[j];

            if (a->pos.x<a2->pos.x-conf::DIST || a->pos.x>a2->pos.x+conf::DIST
                    || a->pos.y>a2->pos.y+conf::DIST || a->pos.y<a2->pos.y-conf::DIST) continue;

            float d= (a->pos-a2->pos).length();

            if (d<conf::DIST) {

                //smell
                smaccum+= 0.3*(conf::DIST-d)/conf::DIST;

                //sound
                soaccum+= 0.4*(conf::DIST-d)/conf::DIST*(max(fabs(a2->w1),fabs(a2->w2)));

                //hearing. Listening to other agents
                hearaccum+= a2->soundmul*(conf::DIST-d)/conf::DIST;

                float ang= (a2->pos- a->pos).get_angle(); //current angle between bots

                //left and right eyes
                float leyeangle= a->angle - PI8;
                float reyeangle= a->angle + PI8;
                float backangle= a->angle + M_PI;
                float forwangle= a->angle;
                if (leyeangle<-M_PI) leyeangle+= 2*M_PI;
                if (reyeangle>M_PI) reyeangle-= 2*M_PI;
                if (backangle>M_PI) backangle-= 2*M_PI;
                float diff1= leyeangle- ang;
                if (fabs(diff1)>M_PI) diff1= 2*M_PI- fabs(diff1);
                diff1= fabs(diff1);
                float diff2= reyeangle- ang;
                if (fabs(diff2)>M_PI) diff2= 2*M_PI- fabs(diff2);
                diff2= fabs(diff2);
                float diff3= backangle- ang;
                if (fabs(diff3)>M_PI) diff3= 2*M_PI- fabs(diff3);
                diff3= fabs(diff3);
                float diff4= forwangle- ang;
                if (fabs(forwangle)>M_PI) diff4= 2*M_PI- fabs(forwangle);
                diff4= fabs(diff4);

                if (diff1<PI38) {
                    //we see this agent with left eye. Accumulate info
                    float mul1= conf::EYE_SENSITIVITY*((PI38-diff1)/PI38)*((conf::DIST-d)/conf::DIST);
                    //float mul1= 100*((conf::DIST-d)/conf::DIST);
                    p1 += mul1*(d/conf::DIST);
                    r1 += mul1*a2->red;
                    g1 += mul1*a2->gre;
                    b1 += mul1*a2->blu;
                }

                if (diff2<PI38) {
                    //we see this agent with left eye. Accumulate info
                    float mul2= conf::EYE_SENSITIVITY*((PI38-diff2)/PI38)*((conf::DIST-d)/conf::DIST);
                    //float mul2= 100*((conf::DIST-d)/conf::DIST);
                    p2 += mul2*(d/conf::DIST);
                    r2 += mul2*a2->red;
                    g2 += mul2*a2->gre;
                    b2 += mul2*a2->blu;
                }

                if (diff3<PI38) {
                    //we see this agent with back eye. Accumulate info
                    float mul3= conf::EYE_SENSITIVITY*((PI38-diff3)/PI38)*((conf::DIST-d)/conf::DIST);
                    //float mul2= 100*((conf::DIST-d)/conf::DIST);
                    p3 += mul3*(d/conf::DIST);
                    r3 += mul3*a2->red;
                    g3 += mul3*a2->gre;
                    b3 += mul3*a2->blu;
                }

                if (diff4<PI38) {
                    float mul4= conf::BLOOD_SENSITIVITY*((PI38-diff4)/PI38)*((conf::DIST-d)/conf::DIST);
                    //if we can see an agent close with both eyes in front of us
                    blood+= mul4*(1-agents[j].health/2); //remember: health is in [0 2]
                    //agents with high life dont bleed. low life makes them bleed more
                }
            }
        }

        a->in[0]= cap(p1);
        a->in[1]= cap(r1);
        a->in[2]= cap(g1);
        a->in[3]= cap(b1);
        a->in[5]= cap(p2);
        a->in[6]= cap(r2);
        a->in[7]= cap(g2);
        a->in[8]= cap(b2);
        a->in[9]= cap(soaccum);
        a->in[10]= cap(smaccum);
        a->in[12]= cap(p3);
        a->in[13]= cap(r3);
        a->in[14]= cap(g3);
        a->in[15]= cap(b3);
        a->in[16]= abs(sin(modcounter/a->clockf1));
        a->in[17]= abs(sin(modcounter/a->clockf2));
        a->in[18]= cap(hearaccum);
        a->in[19]= cap(blood);
    }
}
*/

void World::processOutputs(float timestep) {
	// Loop through all active agents
	for (int i=0;i<(int)agents.size();i++) {
		agents[i]->doOutput(timestep, this);
	}
}

/* Scriptbots version: DEPRECATED
void World::processOutputs() {

    //assign meaning
    //LEFT RIGHT R G B SPIKE BOOST SOUND_MULTIPLIER GIVING
    // 0    1    2 3 4   5     6         7             8
    for (int i=0;i<agents.size();i++) {
        Agent* a= &agents[i];

        a->red= a->out[2];
        a->gre= a->out[3];
        a->blu= a->out[4];
        a->w1= a->out[0]; //-(2*a->out[0]-1);
        a->w2= a->out[1]; //-(2*a->out[1]-1);
        a->boost= a->out[6]>0.5;
        a->soundmul= a->out[7];
        a->give= a->out[8];

        //spike length should slowly tend towards out[5]
        float g= a->out[5];
        if (a->spikeLength<g)
            a->spikeLength+=conf::SPIKESPEED;
        else if (a->spikeLength>g)
            a->spikeLength= g; //its easy to retract spike, just hard to put it up
    }

    //move bots
    //#pragma omp parallel for
    for (int i=0;i<agents.size();i++) {
        Agent* a= &agents[i];

        Vector2f v(conf::BOTRADIUS/2, 0);
        v.rotate(a->angle + M_PI/2);

        Vector2f w1p= a->pos+ v; //wheel positions
        Vector2f w2p= a->pos- v;

        float BW1= conf::BOTSPEED*a->w1;
        float BW2= conf::BOTSPEED*a->w2;
        if (a->boost) {
            BW1=BW1*conf::BOOSTSIZEMULT;
        }
        if (a->boost) {
            BW2=BW2*conf::BOOSTSIZEMULT;
        }

        //move bots
        Vector2f vv= w2p- a->pos;
        vv.rotate(-BW1);
        a->pos= w2p-vv;
        a->angle -= BW1;
        if (a->angle<-M_PI) a->angle= M_PI - (-M_PI-a->angle);
        vv= a->pos - w1p;
        vv.rotate(BW2);
        a->pos= w1p+vv;
        a->angle += BW2;
        if (a->angle>M_PI) a->angle= -M_PI + (a->angle-M_PI);

        //wrap around the map
        if (a->pos.x<0) a->pos.x= conf::WIDTH+a->pos.x;
        if (a->pos.x>=conf::WIDTH) a->pos.x= a->pos.x-conf::WIDTH;
        if (a->pos.y<0) a->pos.y= conf::HEIGHT+a->pos.y;
        if (a->pos.y>=conf::HEIGHT) a->pos.y= a->pos.y-conf::HEIGHT;
    }

    //process food intake for herbivors
    for (int i=0;i<agents.size();i++) {

        int cx= (int) agents[i].pos.x/conf::CZ;
        int cy= (int) agents[i].pos.y/conf::CZ;
        float f= food[cx][cy];
        if (f>0 && agents[i].health<2) {
            //agent eats the food
            float itk=min(f,conf::FOODINTAKE);
            float speedmul= (1-(abs(agents[i].w1)+abs(agents[i].w2))/2)/2 + 0.5;
            itk= itk*agents[i].herbivore*agents[i].herbivore*speedmul; //herbivores gain more from ground food
            agents[i].health+= itk;
            agents[i].repcounter -= 3*itk;
            food[cx][cy]-= min(f,conf::FOODWASTE);
        }
    }

    //process giving and receiving of food
    for (int i=0;i<agents.size();i++) {
        agents[i].dfood=0;
    }
    for (int i=0;i<agents.size();i++) {
        if (agents[i].give>0.5) {
            for (int j=0;j<agents.size();j++) {
                float d= (agents[i].pos-agents[j].pos).length();
                if (d<conf::FOOD_SHARING_DISTANCE) {
                    //initiate transfer
                    if (agents[j].health<2) agents[j].health += conf::FOODTRANSFER;
                    agents[i].health -= conf::FOODTRANSFER;
                    agents[j].dfood += conf::FOODTRANSFER; //only for drawing
                    agents[i].dfood -= conf::FOODTRANSFER;
                }
            }
        }
    }

    //process spike dynamics for carnivors
    if (modcounter%2==0) { //we dont need to do this TOO often. can save efficiency here since this is n^2 op in #agents
        for (int i=0;i<agents.size();i++) {
            for (int j=0;j<agents.size();j++) {
                if (i==j || agents[i].spikeLength<0.2 || agents[i].w1<0.3 || agents[i].w2<0.3) continue;
                float d= (agents[i].pos-agents[j].pos).length();

                if (d<2*conf::BOTRADIUS) {
                    //these two are in collision and agent i has extended spike and is going decent fast!
                    Vector2f v(1,0);
                    v.rotate(agents[i].angle);
                    float diff= v.angle_between(agents[j].pos-agents[i].pos);
                    if (fabs(diff)<M_PI/8) {
                        //bot i is also properly aligned!!! that's a hit
                        float mult=1;
                        if (agents[i].boost) mult= conf::BOOSTSIZEMULT;
                        float DMG= conf::SPIKEMULT*agents[i].spikeLength*max(fabs(agents[i].w1),fabs(agents[i].w2))*conf::BOOSTSIZEMULT;

                        agents[j].health-= DMG;

                        if (agents[i].health>2) agents[i].health=2; //cap health at 2
                        agents[i].spikeLength= 0; //retract spike back down

                        agents[i].initEvent(40*DMG,1,1,0); //yellow event means bot has spiked other bot. nice!

                        Vector2f v2(1,0);
                        v2.rotate(agents[j].angle);
                        float adiff= v.angle_between(v2);
                        if (fabs(adiff)<M_PI/2) {
                            //this was attack from the back. Retract spike of the other agent (startle!)
                            //this is done so that the other agent cant right away "by accident" attack this agent
                            agents[j].spikeLength= 0;
                        }
                    }
                }
            }
        }
    }
}

*/

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

void World::resetShadowLayer() {
	// First, reset to 0
	for (int x=0;x<conf::WIDTH;x++) for (int y=0;y<conf::HEIGHT;y++) CellShadow[x][y] = 0;

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

// When called by the user through the GUI, will iterate through all cells and reset the susceptibility chance
void World::dropCTL() {
	// Add a number of CTL within the bounding box
	int number_ctl = round(CTL_DENSITY * stats->area_mm2);
	//CTL_DENSITY * stats->area_mm2
	int x,y;
	for (int i=0; i<number_ctl; i++) {
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

// Check if the given coord is over a cell, return true/false
bool World::isOverCell(int x, int y) {
	if (x>0 && x<conf::WIDTH && y>0 && y<conf::HEIGHT) {
		return( CellShadow[x][y] > 0 );
	}
	return(false);
}

// Return the cell at coord x,y (0 if nothing is there)
Cell* World::getCell(int x, int y) {
	return( (Cell*)CellShadow[x][y] );
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
				agents.push_back(*cell);
				infectedcount++;
			}
		}
		cell++;
	}
}

// Saves most of the Stats module to a csv
void World::writeReport(bool firstTime) {

	char buffer[255];
	// TODO: Check that logs/ exist
	sprintf(buffer, "logs/log_%i.csv", UID);
	FILE* fp = fopen(buffer, "a");
	if (firstTime) {
		// Write out the header
		fprintf(fp, "time,infected,virions,ctl\n");
	}
	fprintf(fp, "%f,%i,%i,%i\n", worldtime, stats->infected, stats->virion, stats->ctl);
	fclose(fp);

}


void World::reset() {

    for (int x=0;x<conf::WIDTH;x++) {
    	patch.push_back( vector<bool>(conf::HEIGHT,false) );
    	CellShadow.push_back( vector<Cell*>(conf::HEIGHT,(Cell*)0) );
    }

	patchV.clear();
	cells.clear();
    agents.clear();

    worldtime = 0.0;

	bounding_min = Vector2f(-1,-1);
	bounding_max = Vector2f(-1,-1);

	updateStats(true);

}

/* This gets called to draw the background things of the world: bounding box, "worldspace" and patches (latter only if in setup mode) */
void World::drawBackground(View* view, bool simulationmode) {

	// 1. Draw the setup/simulaton specific stuff
	// If in setup mode, draw the patches and entire worldspace (cyan)
	// If in simulation mode, only draw the background of the bounding box (and not the whole matrix)
	if ( !simulationmode ) {
		// Setup mode
		// a. Draw the worldspace in cyan
		glBegin(GL_QUADS);
		glColor4f(0.1,0.9,1.0,0.5);
		glVertex3f(0,0,-conf::FAR_PLANE);
		glVertex3f(0,conf::HEIGHT,-conf::FAR_PLANE);
		glVertex3f(conf::WIDTH,conf::HEIGHT,-conf::FAR_PLANE);
		glVertex3f(conf::WIDTH,0,-conf::FAR_PLANE);
		glEnd();

		// b. Draw the patches
		for (int i=0; i<(int)patchV.size(); i++) {
			view->drawDot( int(patchV[i]/conf::WIDTH), (patchV[i]%conf::WIDTH), (-conf::FAR_PLANE+1.0) );
		}
	} else {
		// Simulation mode
		// a. Draw the bounding box quad in cyan
		glBegin(GL_QUADS);
		glColor4f(0.1,0.9,1.0,0.5);
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


void World::drawForeground(View* view) {



	// 2. Iterate through the CTL vector and draw each one
    vector<AbstractAgent*>::const_iterator ait;
    for ( ait = agents.begin(); ait != agents.end(); ++ait) {
    	// Only draws the agent if it's within the world bounding box
    	view->drawAgent(*ait); // note: this just calls ait->drawAgent(view);
        //(*ait)->drawAgent((GLView)view);	// Skip the middle man, if I can figure out how
    }

	// 1. Iterate through the cells vector and draw each one
	vector<Cell*>::iterator cit;
	for ( cit = cells.begin(); cit != cells.end(); ++cit) {
		view->drawCell(*cit);
	}
}

int World::numAgents() const {
    return agents.size();
}

int World::numCells() const {
	return cells.size();
}

// Will save the patch, cell and agent info to a file.
// TODO: Check if we overwrite an existing file
void World::saveLayout() {

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


}

// Will load a world setup from the given filename
// Note: probably unecessary to pass in the view pointer, as view *calls* this function.
void World::loadLayout() {

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

}

void World::updateStats(bool resetEvents) {

	int total_cells = 0, ctl = 0, notsusceptible = 0, infected = 0, susceptible = 0, virion = 0;

	if (resetEvents) {
		stats->infected_death = 0;
		stats->infected_lysis = 0;
	}

	// Update the cell-related stats
	vector<Cell*>::iterator cit;
	for ( cit = cells.begin(); cit != cells.end(); ++cit) {
		// Increment the right counter
		switch ( ((AbstractAgent*)(*cit))->getAgentType() ) {
			case AbstractAgent::AGENT_SUSCEPTIBLE:
				susceptible++;
				break;
			case AbstractAgent::AGENT_INFECTED:
				infected++;
				break;
			default: break;
		}
		total_cells++;
	}

	// Update the ctl & virion stats
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
	stats->ctl = ctl;
	stats->total_cells = total_cells;
	stats->notsusceptible = notsusceptible;
	stats->susceptible = susceptible;
	stats->infected = infected;
	stats->virion = virion;

	// Can't update the events here

	// Update the density calculations
	stats->ctl_per_mm2    = 0.0;
	stats->virion_per_mm2 = 0.0;
	if (stats->area_mm2>0.0) {
		stats->ctl_per_mm2    = ctl/stats->area_mm2;
		stats->virion_per_mm2 = virion/stats->area_mm2;
	}

	stats->ctl_per_cell = 0.0;
	stats->virion_per_cell = 0.0;
	int nonctlcells = total_cells - ctl;
	if (nonctlcells>0) {
		stats->ctl_per_cell = ctl/(float)nonctlcells;
		stats->virion_per_cell = virion/(float)nonctlcells;
	}

	stats->ctl_per_infected = 0.0;
	if (infected>0) {
		stats->ctl_per_infected = ctl/(float)infected;
	}

	stats->cell_area = nonctlcells * M_PI * pow(conf::BOTRADIUS,2) / 1000000.0;
}

