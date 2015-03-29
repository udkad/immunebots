#include "World.h"

#include <ctime>
#include <stdio.h>
#include <iostream>
#include <unordered_set>

#include "settings.h"
#include "helpers.h"
#include "vmath.h"

using namespace std;

World::World():
        modcounter(0),
        current_epoch(0),
        idcounter(0),
        cidcounter(0),
        FW(conf::WIDTH/conf::CZ),
        FH(conf::HEIGHT/conf::CZ),
        CLOSED(false),
        selectedCell(0),
        inFocusCell(0),
        docelljitter(false),
        dobackgroundrefresh(true),
        cellJitterIteration(0)
{
	// Setup the world colours from the conf
	resetCellColours();

	// Copy over the other variables from settings (manual!)
	SUSCEPTIBLE_PERCENTAGE   = conf::SUSCEPTIBLE_PERCENTAGE;
	DEFAULT_NUM_CELLS_TO_ADD = conf::DEFAULT_NUM_CELLS_TO_ADD;

    addRandomBots(conf::NUMBOTS);

    // inititalize food layer
    for (int x=0;x<FW;x++) {
        for (int y=0;y<FH;y++) {
            food[x][y]= 0;
        }
    }

    // initialise the patch (infected island) and CellShadow layers
    for (int x=0;x<conf::WIDTH;x++) {
        for (int y=0;y<conf::HEIGHT;y++) {
            patch[x][y] = 0;
            CellShadow[x][y] = 0;
        }
    }

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

// This sets a '1' in the 2D patch array at co-ordinate (x,y) and tells all cells that there is a new patch
void World::setPatch(int x, int y) {
	patch[x][y] = 1;
    vector<Cell*>::const_iterator cit;
    for ( cit = cells.begin(); cit != cells.end(); ++cit) {
        //(*cit)->checkClosestPatch(x, y);
        // For each cell, allow a 1 in patchSize chance of choosing this new patch
        (*cit)->setNewPatch(x,y,patchVector.size());
    }
    // Store this new patch on the patch vector
    patchVector.push_back( x*conf::WIDTH + y );

    // Tell View that background has changed and needs to be refreshed
    dobackgroundrefresh = true;
}

void World::update() {

	modcounter++;

    //Process periodic events
    //Age goes up!
    if (modcounter%100==0) {
        for (int i=0;i<agents.size();i++) {
            agents[i].age+= 1;    //agents age...
        }
    }
    if (modcounter%1000==0) writeReport();
    if (modcounter>=10000) {
        modcounter=0;
        current_epoch++;
    }
    if (modcounter%conf::FOODADDFREQ==0) {
        fx=randi(0,FW);
        fy=randi(0,FH);
        food[fx][fy]= conf::FOODMAX;
    }


    /****************\
     * AGENT UPDATE *
    \****************/

    //give input to every agent. Sets in[] array
    setInputs();

    //brains tick. computes in[] -> out[]
    brainsTick();

    //read output and process consequences of bots on environment. requires out[]
    processOutputs();

    //process bots: health and deaths
    for (int i=0;i<agents.size();i++) {
        float baseloss= 0.0002 + 0.0001*(abs(agents[i].w1) + abs(agents[i].w2))/2;
        if (agents[i].w1<0.1 && agents[i].w2<0.1) baseloss=0.0001; //hibernation :p
        baseloss += 0.00005*agents[i].soundmul; //shouting costs energy. just a tiny bit

        if (agents[i].boost) {
            //boost carries its price, and it's pretty heavy!
            agents[i].health -= baseloss*conf::BOOSTSIZEMULT*2;
        } else {
            agents[i].health -= baseloss;
        }
    }

    //remove dead agents.

    vector<Agent>::iterator iter= agents.begin();
    while (iter != agents.end()) {
        if (iter->health <=0) {
            iter= agents.erase(iter);
        } else {
            ++iter;
        }
    }

    //handle reproduction
    for (int i=0;i<agents.size();i++) {
        if (agents[i].repcounter<0 && agents[i].health>0.65) { //agent is healthy and is ready to reproduce
            //agents[i].health= 0.8; //the agent is left vulnerable and weak, a bit
            reproduce(i, agents[i].MUTRATE1, agents[i].MUTRATE2); //this adds conf::BABIES new agents to agents[]
            agents[i].repcounter= agents[i].herbivore*randf(conf::REPRATEH-0.1,conf::REPRATEH+0.1) + (1-agents[i].herbivore)*randf(conf::REPRATEC-0.1,conf::REPRATEC+0.1);
        }
    }

    //environment tick
    for (int x=0;x<FW;x++) {
        for (int y=0;y<FH;y++) {
            food[x][y]+= conf::FOODGROWTH; //food grows
            if (food[x][y]>conf::FOODMAX)food[x][y]=conf::FOODMAX; //cap at conf::FOODMAX
        }
    }

    //add new agents, if environment isn't closed
    if (!CLOSED) {
        //make sure environment is always populated with at least NUMBOTS bots
        if (agents.size()<conf::NUMBOTS
           ) {
            //add new agent
            addRandomBots(1);
        }
        if (modcounter%200==0) {
            if (randf(0,1)<0.5)
                addRandomBots(1); //every now and then add random bots in
            else
                addNewByCrossover(); //or by crossover
        }
    }

    /***************\
     * CELL UPDATE *
    \***************/

    // If cells are in jitter mode, we iterate through each one and update their position
    if (docelljitter && cellJitterIteration<10 && cellJitterIteration!=-1) {
    	//cout << "Cell jitter i="<<cellJitterIteration<<"\n";
    	// Refresh the progress bar (ensure we have it BEFORE the first update
    	for (int i=0;i<cells.size();i++) {

    		// Is the cell within 2r of the target?
    		double dx = cells[i]->nearestPatch.x - cells[i]->pos.x;
    		double dy = cells[i]->nearestPatch.y - cells[i]->pos.y;
    		double dtarget = pow(dx, 2) + pow(dy, 2);
    		float a; // angle
    		float d = 1.0; // distance

    		// Store old position, as later we update the CellShadow with new position
    		Vector2f oldpos = Vector2f(cells[i]->pos.x, cells[i]->pos.y);

    		if ( dtarget < pow(cells[i]->radius*4,2) ) {
    			// Within 2r of target: choose random direction and move there
    			bool isFree = false;
    			int emergencyStop = 0;
    			while (!isFree && emergencyStop <10) {
    				a = randf(0,2*M_PI);
    				// Check if we can move into spot at angle a, remember the radius!
    				Cell * oc = CellShadow[ (int)(cells[i]->pos.x+cells[i]->radius*cos(a)) ][ (int)(cells[i]->pos.y+cells[i]->radius*sin(a)) ];
    				isFree = (oc == 0 || oc == cells[i]);
    				emergencyStop++;
    			}
    			// Too crowded, choose random location on grid and find nearest patch.
				if (emergencyStop == 10) {
					cells[i]->pos.x = randf(0, conf::WIDTH);
					cells[i]->pos.y = randf(0, conf::HEIGHT);
					cells[i]->setClosestPatch(&patch[0][0]);
				} else {
					cells[i]->pos.x += d*cos(a);
					cells[i]->pos.y += d*sin(a);
				}
    		} else {
    			// Too far away, we can choose one of two strategies:
    			// 1. Choose a random direction in a limited 10deg angle towards target and head there one step
    			// 2. Teleport to a random point within 4r of the target
    			int strategy = 2;
    			if ( strategy == 1 ) {
    				// Don't care if we cross paths with anything else (at this point, will care later)
    				a = atan2(dy,dx) + randf(-0.1*M_PI,0.1*M_PI);
					// Add a random jitter to this angle.. e.g. randf(-0.1*PI,0.1*PI) for +/-5% jitter
    	    		cells[i]->pos.x += d*cos(a);
    	    		cells[i]->pos.y += d*sin(a);
    			} else if ( strategy == 2 ) {
    				d = randf(0,cells[i]->radius*3);
    				a = randf(0,2*M_PI);
    				cells[i]->pos.x = cells[i]->nearestPatch.x + d*cos(a);
    				cells[i]->pos.y = cells[i]->nearestPatch.y + d*sin(a);
    			}
    		}

    		// Note: Alternative is to use vector maths: cannot fiddle with angle if you do it this way.
    		// 1 in 10 chance of resetting nearestPatch to a closer target
    		if ( randf(0,1) < 0.1 ) cells[i]->setClosestPatch(&patch[0][0]);
    		resetShadowLayer(cells[i], oldpos);

    		// Reset CellShadow for this cell only, and all nearby cells
    	}
    	cellJitterIteration++;

    	// Update the shadow: note if we need to know the shadow in the above update then we have to update it after very cell move!!
    	// 1. Reset, then 2. loop through all Cells
        //for (int x=0;x<conf::WIDTH;x++) for (int y=0;y<conf::HEIGHT;y++) CellShadow[x][y] = 0;
    	//for (int i=0;i<cells.size();i++) setCellShadow( cells[i] );
    } else if (docelljitter && cellJitterIteration >= 10) {
    	// Have reached the last cell jitter, reset and refresh background.
    	docelljitter = false;
    	dobackgroundrefresh = true;
    } else if (cellJitterIteration < 0) {
    	cellJitterIteration = 0;
    }
}

void World::resetShadowLayer(Cell * c, Vector2f oldpos) {
	// 1. Zero out the old x,y of Cell c
	// ... and find all cells surrounding the old x.y of Cell c
	int r = c->radius;
	float rsq1 = pow(r+1,2);
	float rsq2 = pow(c->radius,2);
	int max_x = min(conf::WIDTH, int(c->pos.x+c->radius));
	int max_y = min(conf::HEIGHT, int(c->pos.y+c->radius));

	unordered_set<Cell *> surroundingCells;

	// Loop through the bounding box and put the reference to the cell in CellShadow[][] if it's "within" the circle
	// a. Check if we have another cell potentially overlapping
	// b. Zero out the old cell radius
	for (int i=max(0,int(oldpos.x-c->radius));i<max_x; i++ ) {
		for (int j=max(0,int(oldpos.y-c->radius));j<max_y; j++ ) {
			//
			if ( ( pow(oldpos.x-i,2) + pow(oldpos.y-j,2) ) < rsq1 ) {
				// point is within the "danger zone" - check if we have a non-zero entry here
				if ( CellShadow[i][j] != 0 && CellShadow[i][j] != c ) {
					// If point in bounding box is not empty (0), this cell (c) [or already in the vector], then add it
					surroundingCells.insert( &(*CellShadow[i][j]) );
				}
				if ( ( pow(oldpos.x-i,2) + pow(oldpos.y-j,2) ) < rsq2 ) {
					// If the point is within the boundary of the cell, then reset to 0
					CellShadow[i][j] = 0;
				}
			}
		}
	}
	// 3. Reset the Shadow for all cells surrounding c
	//  - needs to iterate through the surroundingCells list and reset everything there
	unordered_set<Cell *>::iterator thisSurroundingCell;
	for( thisSurroundingCell = surroundingCells.begin(); thisSurroundingCell != surroundingCells.end(); thisSurroundingCell++ ) {
		setCellShadow( *thisSurroundingCell );
		//cout << "Surrounding cell is: " << thisSurroundingCell->first << "\n";
	}
	// 4. Reset the Shadow for c
	setCellShadow(c);
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

void World::brainsTick() {

#pragma omp parallel for

	for (int i=0;i<agents.size();i++) {
        agents[i].tick();
    }

}

void World::addRandomBots(int num) {

    for (int i=0;i<num;i++) {
        Agent a;
        a.id= idcounter;
        idcounter++;
        agents.push_back(a);
    }
}

void World::addRandomCells(int num) {

	// If num is 0 then use the World::DEFAULT_NUM_CELLS_TO_ADD
	if (num==0)
		num = DEFAULT_NUM_CELLS_TO_ADD;

    for (int i=0;i<num;i++) {
        Cell * c = new Cell();
        // Set probability that this cell is susceptible
        c->setSusceptible( SUSCEPTIBLE_PERCENTAGE );
        // Set cell ID and push onto the cells array
        c->id = cidcounter;
        cidcounter++;
        cells.push_back(c);
        // Update the cell shadow layer with every point this cell takes up, with a call to setCellShadow(c.pos.x, c.pos.y, r)
        setCellShadow(c);
        // Randomly choose a patch to home into
        //c->setClosestPatch(&patch[0][0]);
        c->chooseRandomPatch( patchVector );
    }

    // Tell View that background has changed and needs to be refreshed
    dobackgroundrefresh = true;
}


void World::addCell(int x, int y) {
	Cell * c = new Cell();
	// Put at required location
	//cout << "[DEBUG] placing cell ("<< c << ") at (" << x << "," << y << ")\n";
	c->pos.x = x;
	c->pos.y = y;
	// Set probability that this cell is susceptible: Generate number between 0.00 and 100.00
	c->setSusceptible( SUSCEPTIBLE_PERCENTAGE );
	// Do the other required stuff (set ID, push onto Cell vector)
	c->id = cidcounter;
	cidcounter++;
	cells.push_back(c);
    // Update the cell shadow layer with every point this cell takes up, with a call to setCellShadow(c.pos.x, c.pos.y, r)
    setCellShadow(c);
    // Select a random patch to home into
    c->chooseRandomPatch( patchVector );
    //c->setClosestPatch(&patch[0][0]);
    // Tell View that background has changed and needs to be refreshed
    dobackgroundrefresh = true;
}

void World::setCellShadow(Cell *c) {
	_setCellShadow( c, c );
}

void World::resetCellShadow(Cell *c) {
	_setCellShadow( c, 0 );
}

// Given a Cell*, sets the address of the Cell in the 2D "World" shadow, CellShadow[][].
void World::_setCellShadow(Cell *c, Cell * value) {
	// Some handy variables to speed stuff up
	float rsq = pow(c->radius,2);
	int max_x = min(conf::WIDTH, int(c->pos.x+c->radius));
	int max_y = min(conf::HEIGHT, int(c->pos.y+c->radius));

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
    // Tell View that background has changed and needs to be refreshed
	dobackgroundrefresh = true;
}

// When we clear cells, we need to remove all cells from the iterator, the selectedCell, inFocusCell AND reset the ShadowGrid
void World::clearAllCells() {
	cells.clear();
	selectedCell = 0;
	inFocusCell  = 0;
    for (int x=0;x<conf::WIDTH;x++) {
        for (int y=0;y<conf::HEIGHT;y++) {
            CellShadow[x][y] = 0;
        }
    }
    // Tell View that background has changed and needs to be refreshed
    dobackgroundrefresh = true;
}

void World::startJitter() {
	if ( !docelljitter ) {
		docelljitter = true;
		cellJitterIteration = -1;
	}
}

// Check if the given coord is over a cell, return true/false
bool World::isOverCell(int x, int y) {
	return( CellShadow[x][y] > 0 );
}

// Return the cell at coord x,y (0 if nothing is there)
Cell * World::getCell(int x, int y) {
	return( (Cell*)CellShadow[x][y] );
}

// If mouse was clicked over a cell, set that cell as "selected"
// Also gets rid of existing selected cell.
void World::setSelectOnCell(int x, int y) {
	if (isOverCell(x,y)) {
		Cell * newSelectedCell = getCell(x,y);
		cout << "SelectedCell is " << newSelectedCell << "\n";
		if (selectedCell != 0 && selectedCell != newSelectedCell) {
			selectedCell->toggleSelected();
		}
		selectedCell = newSelectedCell;
		selectedCell->toggleSelected();
	}
}

// If we click on a non-susceptible or susceptible cell, we infect it
// If we click on an infected cell, we remove the infection
void World::setInfection(int x, int y) {
	if (isOverCell(x,y)) {
		Cell * c = getCell(x,y);
		c->toggleInfection();
	}
    // Tell View that background has changed and needs to be refreshed
	dobackgroundrefresh = true;
}

// If mouse is over a cell, set that cell as "in focus"
// Else remove the focus from the existing cell
void World::setFocusOnCell(int x,int y) {
	if (isOverCell(x,y)) {
		inFocusCell = getCell(x,y);
		inFocusCell->setInFocus(true);
	} else if ( inFocusCell != 0 ) {
		inFocusCell->setInFocus(false);
		inFocusCell = 0;
	}
}

void World::addNewByCrossover() {

    //find two success cases
    int i1= randi(0, agents.size());
    int i2= randi(0, agents.size());
    for (int i=0;i<agents.size();i++) {
        if (agents[i].age > agents[i1].age && randf(0,1)<0.1) {
            i1= i;
        }
        if (agents[i].age > agents[i2].age && randf(0,1)<0.1 && i!=i1) {
            i2= i;
        }
    }

    Agent* a1= &agents[i1];
    Agent* a2= &agents[i2];


    //cross brains
    Agent anew = a1->crossover(*a2);


    //maybe do mutation here? I dont know. So far its only crossover
    anew.id= idcounter;
    idcounter++;
    agents.push_back(anew);
}

void World::reproduce(int ai, float MR, float MR2) {

    if (randf(0,1)<0.04) MR= MR*randf(1, 10);
    if (randf(0,1)<0.04) MR2= MR2*randf(1, 10);

    agents[ai].initEvent(30,0,0.8,0); //green event means agent reproduced.
    for (int i=0;i<conf::BABIES;i++) {

        Agent a2 = agents[ai].reproduce(MR,MR2);
        a2.id= idcounter;
        idcounter++;
        agents.push_back(a2);

        // For someone to fix recording
        //record this
        //FILE* fp = fopen("log.txt", "a");
        //fprintf(fp, "%i %i %i\n", 1, this->id, a2.id); //1 marks the event: child is born
        //fclose(fp);
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
    agents.clear();
    addRandomBots(conf::NUMBOTS);
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


void World::drawBackground(View* view, bool drawfood) {

	//cout << "Drawing the background from scratch..\n";

    if(drawfood) {
        for(int i=0;i<FW;i++) {
            for(int j=0;j<FH;j++) {
                float f= 0.5*food[i][j]/conf::FOODMAX;
                view->drawFood(i,j,f);
            }
        }
    }

	// Draw patches from patch
	for(int i=0;i<conf::WIDTH;i++) {
		for(int j=0;j<conf::HEIGHT;j++) {
			if ( patch[i][j] == 1) {
				view->drawDot(i,j);
			}
		}
	}

	vector<Cell*>::const_iterator cit;
	for ( cit = cells.begin(); cit != cells.end(); ++cit) {
		view->drawCell(**cit);
	}


	// Finally: draw the Selected and InFocus cells (if any), and reset InFocus :)
	if ( selectedCell != 0 ) view->drawCell( *selectedCell );
	if ( inFocusCell  != 0 ) view->drawCell( *inFocusCell );

    // Reset the background refresh flag
	dobackgroundrefresh = false;
}

bool World::doBackgroundRefresh() {
	return dobackgroundrefresh;
}

void World::setBackgroundRefresh(bool b) {
	dobackgroundrefresh = b;
}

void World::drawForeground(View* view) {

    vector<Agent>::const_iterator ait;
    for ( ait = agents.begin(); ait != agents.end(); ++ait) {
        view->drawAgent(*ait);
    }

	if (docelljitter) {
		// Update with progress
		view->drawProgressBar(cellJitterIteration/10.0);
	}

}

std::pair< int,int > World::numHerbCarnivores() const {
    int numherb=0;
    int numcarn=0;
    for (int i=0;i<agents.size();i++) {
        if (agents[i].herbivore>0.5) numherb++;
        else numcarn++;
    }

    return std::pair<int,int>(numherb,numcarn);
}

int World::numAgents() const {
    return agents.size();
}

int World::numCells() const {
	return cells.size();
}

int World::epoch() const {
    return current_epoch;
}
