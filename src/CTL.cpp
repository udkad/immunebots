/*
 * CTL.cpp
 *
 *  Created on: Apr 26, 2011
 *      Author: ulrich
 */


#include "CTL.h"
#include "Cell.h"

// If we don't have an explicit list of CTL (as they're stored in the AbstractAgent pointer list, agents),
// then we need to explicitly export the CTL class.
// c.f. Infected cells, which are AbstractAgent->Cells but stored in the Cells pointer list, cells.
#ifndef IMMUNEBOTS_NOSERIALISATION
BOOST_CLASS_EXPORT_GUID(CTL, "CTL");
#endif

// Don't call this
CTL::CTL() { }

CTL::CTL(int x, int y, ImmunebotsSetup * ibs) {
	init(ibs);
	pos.x = x;
	pos.y = y;
}

void CTL::init(ImmunebotsSetup * ibs) {
	// CTL have a position, and a speed
    pos   = Vector2f(randf(0,ibs->getParm("WIDTH",conf::DefaultWidth)),randf(0,ibs->getParm("HEIGHT",conf::DefaultHeight)));
    angle = randf(-M_PI,M_PI);
    speed = ibs->getParm("r_ctlspeed",1.0f);
    radius = conf::BOTRADIUS;
    agent_type = AbstractAgent::AGENT_CTL;
    boundarycondition = ibs->getParm("ctl_boundary", BOUNDARY_BOUNCE);

    currentCell = 0;
    lastCell = 0;

    active = true;
    lifespan = 0.0;
    timer = 0.0;
    state = STATE_MOVE;

    drawable = true;
    draw_priority = 0.8; // high priority

    // Initialise the CTL area checker (note: radius is currently hardcoded as 5, but this might change in the future)
    float halfsinr = sqrt(0.5) * radius;

    float cellshadowinit[9][2] = {
    	{0,0},
    	{0,-radius}, {+halfsinr,-halfsinr},
    	{+radius,0}, {+halfsinr,+halfsinr},
    	{0,+radius}, {-halfsinr,+halfsinr},
    	{-radius,0}, {-halfsinr,-halfsinr}
	};

    memcpy( cellshadow, cellshadowinit, sizeof(cellshadowinit) );
    // How Very C! Probably should use a nested vector.
    // Also, there should only be one such vector per radius value, rather than one vector per CTL.

    for (int i=0; i<STATE_MAX; i++) { timekeeper[i] = 0.0f; }
}

CTL::~CTL() {
	// Auto-generated destructor stub
}

/* AbstractAgent functions */

// Draw the CTL
void CTL::drawAgent(GLView * v) {

#ifndef IMMUNEBOTS_NOGL

	if (!drawable) return;

	// 1. Draw the body (simple circle)
    glColor4f(conf::COLOUR_CTL[0],conf::COLOUR_CTL[1],conf::COLOUR_CTL[2], 1.0);
    v->drawCircle(pos.x, pos.y, draw_priority, radius, false);

    // 2. Show which state the CTL is in
    /*
		char buf[10];
		sprintf(buf, "%i", state);
		v->RenderString(pos.x-2.5, pos.y+2.5, GLUT_BITMAP_TIMES_ROMAN_10, buf, 1.0f, 1.0f, 1.0f);
		if (state != STATE_MOVE) {
			sprintf(buf, "(%.0f)", timer);
			v->RenderString(pos.x, pos.y, GLUT_BITMAP_TIMES_ROMAN_10, buf, 0.0f, 0.0f, 1.0f);
		}
     */

#endif
}


// CTL take input: check if they are on a cell (which may be infected)
void CTL::setInput(float dt, World * w) {
	lifespan += dt;
	timekeeper[state] += dt;

	// Is the CTL over a cell?
	for (int i=0; i<9; i++) {
		if (w->isOverCell(pos.x+cellshadow[i][0], pos.y+cellshadow[i][1])) {
			currentCell = w->getCell(pos.x+cellshadow[i][0], pos.y+cellshadow[i][1]);
			break;
		}
	}


	if ( currentCell == 0 && state != STATE_MOVE ) {
		cout << this << "CTL ERROR: Not on a cell but state is sense or kill ("<< state<< ")\n";
	}

	if (state == STATE_MOVE) {
		// Decrement the persistence length "timer" (in this case, timer is a ever-decreasing distance until a direction change happens)
		timer -= dt*speed;

		// If we're over a new cell, then "sense" it
		if (currentCell!=0 && currentCell!=lastCell) {
			state = STATE_SENSE;
			timer = w->ibs->getParm("t_ctlscan",25.0f); // 25s-60s for sensing this cell
		}

	} else if ( state == STATE_SENSE ) {
		// 1. Have we finished sensing the current cells? If so - start moving
		timer -= dt;
		if (currentCell != 0) {
			currentCell->setScanTime(w->worldtime);
		}

		if (timer <= 0) {
			// In scanning mode and have reached 0 (or less) on the timer (change states!):
			w->EventReporter(World::EVENT_CTL_SCANCOMPLETE);
			if (currentCell!=0 && currentCell->isInfected()) {
				// Start killing the cell
				state = STATE_KILL;
				timer = (w->ibs->getParm("t_ctlhandle",30.0f) + randf(w->ibs->getParm("t_ctlhandle_minus",-5.0f),w->ibs->getParm("t_ctlhandle_plus",5.0f)))*60; // Killing takes about 30 mins (*60=s)
			} else {
				state = STATE_MOVE;
				angle = randf(-M_PI,M_PI);
				timer = getPersistenceLength(w);
				lastCell = currentCell;
				currentCell = 0;
			}
		} // else do nothing.. just let timer tick down
	} else if ( state == STATE_KILL ) {
		timer -= dt;
		// Not sure when the CTL actually kills the cell: at some point during the 30mins!
		if (currentCell != 0 &&  !currentCell->isDead() ) {
			if ( randf(0,1) <= (1.0/timer)) {
				// Kill cell and notify the world event reporter
				currentCell->setKilled();
				w->EventReporter(World::EVENT_INFECTEDDEATH_LYSIS);
			}
		}
		if (timer <= 0) {
			// Cell must be dead by now.
			currentCell->setKilled();
			// CTL starts moving
			//cout << this << " CTL has killed cell ("<< currentCell << "); moving on\n";
			state = STATE_MOVE;
			angle = randf(-M_PI,M_PI);
			timer = getPersistenceLength(w);
			lastCell = currentCell;
			currentCell = 0;
		}
	}

}

float CTL::getPersistenceLength(World *w) {
	// TODO: Allow persistence lengths of 0 (i.e. immediate angle rotations)
	return (radius*w->ibs->getParm("r_ctlpersist", 5.0f) + randf(-1.0,1.0));
}

// CTL can do the following actions: sense cell (25s-1min), kill cell (30m), move.
void CTL::doOutput(float dt, World *w) {

	if ( state == STATE_MOVE ) {
		// Just move (for now)
		pos.x += dt*speed*sin(angle);
		pos.y += dt*speed*cos(angle);

		// Reset the angle once we reach the end of our persistence length
		if ( timer <= 0 ) {
			angle += randf(0.0, M_PI)-M_PI/2.0;
			timer = getPersistenceLength(w);
		}

		// Boundary conditions iff we're at a boundary
		bool exceedxmin = (pos.x < w->bounding_min.x);
		bool exceedxmax = (pos.x > w->bounding_max.x);
		bool exceedymin = (pos.y < w->bounding_min.y);
		bool exceedymax = (pos.y > w->bounding_max.y);

		if ( exceedxmin || exceedxmax || exceedymin || exceedymax ) {
			switch (boundarycondition) {
				case BOUNDARY_WRAP:
				    if      (exceedxmin) { pos.x = w->bounding_max.x; }
				    else if (exceedxmax) { pos.x = w->bounding_min.x; }
				    if      (exceedymin) { pos.y = w->bounding_max.y; }
				    else if (exceedymax) { pos.y = w->bounding_min.y; }
					break;
				case BOUNDARY_STAY:
				    if      (exceedxmin) { pos.x = w->bounding_min.x; }
				    else if (exceedxmax) { pos.x = w->bounding_max.x; }
				    if      (exceedymin) { pos.y = w->bounding_min.y; }
				    else if (exceedymax) { pos.y = w->bounding_max.y; }
					break;
				case BOUNDARY_BOUNCE:
					// Simple bounce: Virion bounces off in a random direction.
				    if      (exceedxmin) { pos.x = w->bounding_min.x; angle = randf(0, M_PI); }
				    else if (exceedxmax) { pos.x = w->bounding_max.x; angle = randf(-M_PI, 0); }
				    else if (exceedymin) { pos.y = w->bounding_min.y; angle = randf(-M_PI/2.0, M_PI/2.0); }
				    else if (exceedymax) { pos.y = w->bounding_max.y; angle = randf(M_PI/2.0,3*M_PI/2.0); if (angle>M_PI){angle=-2*M_PI+angle;} }
					break;
				case BOUNDARY_DIE:
					active   = false;
					drawable = false;
					break;
			}
		}

		// OFFSCREEN 3: Kill the CTL if it leaves the matrix (really? why?)
		//if ( pos.x < 0 || pos.x >= conf::WIDTH || pos.y < 0 || pos.y >= conf::HEIGHT ) {
		//	active = false;
		//}

		// Check if we should draw this CTL cell
		//drawable = true;//(pos.x < w->bounding_max.x && pos.x > w->bounding_min.x && pos.y < w->bounding_max.y && pos.y > w->bounding_min.y);
	} // else: does nothing (sits on cell)

}

// Returns a 3-element vector of the time spent in each of the 3 states since the last call
void CTL::getStateFraction( vector< vector<float> > &timefrac ) {
	// Work out the fraction spent in each state.
	float totaltime = 0.0;
	for (int i=0; i<STATE_MAX; i++) totaltime += timekeeper[i];

	// Make the fraction vector and reset timekeeper
	for (int i=0; i<STATE_MAX; i++) {
		float thisfrac = 0.0;
		if ( totaltime > 0 ) { thisfrac = timekeeper[i] / totaltime; }
		timefrac[i].push_back( thisfrac );
		timekeeper[i] = 0.0;
	}

}

void CTL::printInfo() {
	cout << this << " CTL ";
	printf( "(%.2fs)\n", lifespan);
}

/* Other functions */