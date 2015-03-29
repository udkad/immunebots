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
#include <vector>

using namespace std;

World::World( ImmunebotsSetup* is ):
        modcounter(0),
        CLOSED(false),
        worldtime(0.0)
{
	ibs = is;

	// Setup the world colours from the conf
	resetCellColours();

	// Copy over the other variables from settings (manual!)
	SUSCEPTIBLE_PERCENTAGE   = conf::SUSCEPTIBLE_PERCENTAGE;
	DEFAULT_NUM_CELLS_TO_ADD = conf::DEFAULT_NUM_CELLS_TO_ADD;

    // Initialise the patch (infected island) and CellShadow layers
    for (int x=0;x<conf::WIDTH;x++) {
    	patch.push_back( vector<bool>(conf::HEIGHT,false) );
    	CellShadow.push_back( vector<Cell*>(conf::HEIGHT,(Cell*)0) );
    }

    // Clear the patchVector as well
    patchV.clear();

    // Initialise the boundary box (to very wrong numbers)
    resetBoundingBox();
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
	for (int p=0; p<patchV.size(); p++) {
		checkBoundingBox(patchV[p]/conf::WIDTH,patchV[p]%conf::WIDTH);
	}

	// Loop through cells
	for (int c=0; c<cells.size(); c++) {
		checkBoundingBox(cells[c]->pos.x,cells[c]->pos.y);
	}
}

void World::checkBoundingBox(int x,int y) {
	int b = 50;
	if ( x - b < bounding_min.x ) bounding_min.x = max(0, x-b);
	if ( y - b < bounding_min.y ) bounding_min.y = max(0, y-b);
	if ( x + b > bounding_max.x ) bounding_max.x = min(x+b, conf::WIDTH);
	if ( y + b > bounding_max.y ) bounding_max.y = min(y+b, conf::HEIGHT);
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

void World::update() {

	modcounter++;

	// In each update cycle, do the following:
	//  1. Process time
	//  2. Process events (this may activate agents, introduce new agents, etc)
	//  3. Set inputs for the active agents list, i.e. provide each active agent with requested information, e.g. nearby agents, time
	//  4. Process actions for the active agents list, i.e. allow agents to move/update their position, produce offspring, die
    //  5. Tidy up (remove dead stuff)

    // 1. Process time, including recalculating the timestep
	// Update the worldtime by timestep seconds
	// TODO: The Timestep is determined by 1/conf::TIMESTEPS# of the the fastest rate
	float timestep = 1.0;
	worldtime += timestep;

	if (modcounter%1000==0) writeReport();

    if (modcounter>=10000) {
        modcounter=0;
    }

    // TODO: 2. Process events for this timestep

    // 3. Give input to every agent
    setInputs(timestep);

    // 4. Read output and process consequences of bots on environment
    processOutputs(timestep);

    // 5. Tidy up
    vector<AbstractAgent*>::iterator iter = agents.begin();
    while ( iter != agents.end() ) {

    	// Finally, if this agent is inactive, remove from the (and use the newly returned iterator)
    	if ( ((AbstractAgent*)(*iter))->isActive() ) {
    		++iter;
    	} else {
    		iter = agents.erase(iter);
    	}

    }

}

// This will clear all cells before placing them on the patches.
// The idea is that the setup mode is used to create the "world", and then the world is saved to a file.
// The saved world consists of patches and (N/S/I) cells, but no rates, CTL or virions (the first 2 are in the setup file).
// SHOULD include CTL as we can start the simulation with them!
void World::placeCells() {

	int r = conf::BOTRADIUS;
	cells.clear();
	resetBoundingBox(); // Bounding box may be determined by manually placed cells
	// We therefore reset the bounding box (so it includes only the patches) and then re-place the cells.

	// Step through the patch vector to find the last patch and first patch
	int lastPatch = 0;
	int firstPatch = conf::WIDTH*conf::HEIGHT;
	for ( int p=0; p<patchV.size(); p++ ) {
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
		case Cell::CELL_NOT_SUSCEPTIBLE: 	return(COLOUR_NOT_SUSCEPTIBLE);
		case Cell::CELL_SUSCEPTIBLE: 		return(COLOUR_SUSCEPTIBLE);
		case Cell::CELL_INFECTED: 			return(COLOUR_INFECTED);
		case Cell::CELL_CTL: 				return(COLOUR_CTL);
		case Cell::CELL_DEAD: 				return(COLOUR_DEAD);
	};
}

void World::setInputs(float timestep) {
	for (int i=0;i<agents.size();i++) {
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
	for (int i=0;i<agents.size();i++) {
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
	Cell * c = new Cell();
	// Put at required location, if x==-1 then leave as is
	if (x >= 0) {
		c->pos.x = x;
		c->pos.y = y;
	}
	// Check the bounding box
	checkBoundingBox(c->pos.x,c->pos.y);
	// Set probability that this cell is susceptible: Generate number between 0.00 and 100.00
	c->setSusceptible( SUSCEPTIBLE_PERCENTAGE );
	// Do the other required stuff (set ID, push onto Cell vector)
	cells.push_back(c);
    // Update the cell shadow layer with every point this cell takes up, with a call to setCellShadow(c.pos.x, c.pos.y, r)
    setCellShadow(c);
}

void World::resetShadowLayer() {
	// First, reset to 0
	for (int x=0;x<conf::WIDTH;x++) for (int y=0;y<conf::HEIGHT;y++) CellShadow[x][y] = 0;

	// Then, setup through the cells vector
	for (int i=0;i<cells.size();i++) {
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
	for (int i=0;i<cells.size();i++) {
		if ( cells[i]->cType != Cell::CELL_CTL || cells[i]->cType != Cell::CELL_INFECTED ) {
			// If not infected or CTL, then reset to 0 and invoke chance of susceptibility
			cells[i]->cType = Cell::CELL_NOT_SUSCEPTIBLE;
			cells[i]->setSusceptible( SUSCEPTIBLE_PERCENTAGE );
		}
	}
}

// When we clear cells, we need to remove all cells from the iterator, the selectedCell, inFocusCell AND reset the ShadowGrid
void World::clearAllCells() {
	agents.clear();
	cells.clear();
	resetShadowLayer();
	// Also reset the bounding box (depends on the patch vector)
	resetBoundingBox();
}

// Check if the given coord is over a cell, return true/false
bool World::isOverCell(int x, int y) {
	return( CellShadow[x][y] > 0 );
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

void World::writeReport() {

// For someone to fix..
//save all kinds of nice data stuff
//     int numherb=0;
//     int numcarn=0;
//     int topcarn=0;
//     int topherb=0;
//     for(int i=0;i<agents.size();i++){
//         if(agents[i].herbivore>0.5) numherb++;
//         else numcarn++;
//
//         if(agents[i].herbivore>0.5 && agents[i].gencount>topherb) topherb= agents[i].gencount;
//         if(agents[i].herbivore<0.5 && agents[i].gencount>topcarn) topcarn= agents[i].gencount;
//     }
//
//     FILE* fp = fopen("report.txt", "a");
//     fprintf(fp, "%i %i %i %i\n", numherb, numcarn, topcarn, topherb);
//     fclose(fp);
}


void World::reset() {
    for (int x=0;x<conf::WIDTH;x++) {
    	patch.push_back( vector<bool>(conf::HEIGHT,false) );
    	CellShadow.push_back( vector<Cell*>(conf::HEIGHT,(Cell*)0) );
    }
	patchV.clear();
	cells.clear();
    agents.clear();
}

void World::setClosed(bool close) {
    CLOSED = close;
}

bool World::isClosed() const {
    return CLOSED;
}

/* This gets called to draw the world (AIUI):
    1. Draws the food layer, if required
    2. Draws each agent in the agents vector
   Note: The "world update" is handled by the GLUT Idle function, which calls GLView::handleIdle().
   This, and also the GLUT display function, calls renderScene which calls World::draw().
 */


void World::drawBackground(View* view, bool simulationmode) {

	// Draw patches from patchV, but only if in setup mode
	if ( !simulationmode ) {
		for (int i=0; i<patchV.size(); i++) {
			view->drawDot( int(patchV[i]/conf::WIDTH), (patchV[i]%conf::WIDTH) );
		}
	}

	vector<Cell*>::const_iterator cit;
	for ( cit = cells.begin(); cit != cells.end(); ++cit) {
		view->drawCell(**cit);
	}

	// Draw the bounding box
	if ( bounding_max.x != -1 ) {
		glBegin(GL_LINES);
		glColor4f(0.5,0.5,0.5,0.5);
		glVertex3f(bounding_min.x,bounding_min.y,0);
		glVertex3f(bounding_max.x,bounding_min.y,0);

		glVertex3f(bounding_max.x,bounding_min.y,0);
		glVertex3f(bounding_max.x,bounding_max.y,0);

		glVertex3f(bounding_max.x,bounding_max.y,0);
		glVertex3f(bounding_min.x,bounding_max.y,0);

		glVertex3f(bounding_min.x,bounding_max.y,0);
		glVertex3f(bounding_min.x,bounding_min.y,0);
		glEnd();
	}
}


void World::drawForeground(View* view) {
    vector<AbstractAgent*>::const_iterator ait;
    for ( ait = agents.begin(); ait != agents.end(); ++ait) {
        view->drawAgent(*ait); // note: this just calls ait->drawAgent(view);
        //(*ait)->drawAgent((GLView)view);	// Skip the middle man, if I can figure out how
    }
}

int World::numAgents() const {
    return agents.size();
}

int World::numCells() const {
	return cells.size();
}

// Will save the patch and cell info as csv to a file (specified in name)
//TODO: Check if we overwrite an existing file
void World::saveLayout() {

	ofstream setupfile(ibs->layoutfilename);
	if (!setupfile.good()) {
		cout << "ERROR: Could not make setup file '"<< ibs->layoutfilename <<"'; setup save failed.\n";
		return;
	}

	// Ensure there is at least one agent which is an infected cell
	bool haveAnInfectedCell = false;
	for (int a=0; a<agents.size(); a++) {
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

    {	// save setup data, i.e. certain parts of the world class, to archive
        boost::archive::text_oarchive oa(setupfile);
        oa << (*this);
    } 	// archive and stream closed when destructors are called

}

// Will load a world setup from the given filename
void World::loadLayout() {

	// Create a link to the setup file and read it the contents
	string fn = ibs->layoutfilename;
	ifstream setupfile(fn);
	if (!setupfile.good()) {
		cout << "ERROR: Setupfile '"<< fn <<"' does not exist in working dir, setup failed.\n";
		return;
	}

	World wyrd(new ImmunebotsSetup()); // The wyrd world will temporarily store the world we saved from set up
	boost::archive::text_iarchive ia(setupfile);
	ia >> wyrd;
	// Now re-assign cells and patch (will overwrite cells and patches)
	cells = wyrd.cells;
	patchV = wyrd.patchV;
	bounding_min = wyrd.bounding_min;
	bounding_max = wyrd.bounding_max;

	cout << "Loaded cells and patch info\n";
	bounding_min = wyrd.bounding_min;
	// Add cells to active queue
	for (int i=0;i<cells.size();i++) {
		if ( cells[i]->isActive() ) {
			agents.push_back( cells[i] );
		}
	}

	// Reset the shadow layer
	resetShadowLayer();

	// Reset and regenerate the patch[x][y] matrix from the vector.
	for (int i=0;i<conf::WIDTH;i++) for (int j=0;j<conf::HEIGHT;j++) patch[i][j] = 0;
	for (int p=0;p<patchV.size();p++) {
		patch[int(patchV[p]/conf::WIDTH)][patchV[p]%conf::WIDTH] = true;
	}
}

