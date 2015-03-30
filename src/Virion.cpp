/*
 * Virion.cpp
 *
 *  Created on: Apr 15, 2011
 *      Author: ulrich
 */

#include "Virion.h"
#include "helpers.h"

Virion::Virion() {
	// Auto-generated constructor stub
}

Virion::Virion(Cell *pCell, ImmunebotsSetup *ibs) {

	// Set random direction
	angle = randf(0, 2.0*M_PI);
	speed = ibs->getParm("r_vspeed", 0.1f);

	pos 	= pCell->pos;
	active 	= true;
	dead 	= false;
	lifespan = 0.0;
    agent_type = AGENT_VIRION;
    boundarycondition = ibs->getParm("v_boundary", BOUNDARY_DIE);

	r_death  = 1/6 * 1/(60*60); 	/* 6 hours to clearance */
	r_infect = ibs->getParm("r_vinfect", 0.1f); /* No idea, arbitrary value (ensure infection when hitting a cell) */

	parentCell 	= pCell;
	currentCell = 0; // Although technically we're on the parent when we are created

	drawable=true;
	draw_priority = 2.0; // between ctl and cell priority (0.1 and 0.9 respectively)
}

Virion::~Virion() {
	// Auto-generated destructor stub
	cout << this << " Virion: destructor called!\n";
}


/* AbstractAgent functions */

// This should only be called from the view!
void Virion::drawAgent(GLView * v) {
#ifndef IMMUNEBOTS_NOGL

	if (!drawable) return;

	// We initially drew the virion as a green dot, but now we draw it as a green circle of radius 1 with a black border.
	// It still only infects if the center (pos.{x,y}) is over a susceptible cell.

	// Draw a point or a line
	bool drawPoint = true;

	if (drawPoint) {

		// Draw a green dot
		glBegin(GL_POINTS);
		glColor3f(0.0,0.0,0.0);
		glVertex3f(pos.x, pos.y, draw_priority);
		glEnd();

	} else {

		int r = 1;
		// 1. Draw the body (simple circle)
		glColor4f(0.0,1.0f,0.0,1.0);
		v->drawCircle(pos.x, pos.y, draw_priority, r, true);
	}

    	// This should be made optional. Draw green line from parent cell to virion.
		glBegin(GL_LINES);
		glColor3f(0,1,0);
		glVertex2f(parentCell->pos.x,parentCell->pos.y);
		glVertex2f(pos.x, pos.y);
		glEnd();

#endif
}

void Virion::setInput(float dt, World *w) {

	if (dead) { return; } // This shouldn't hit, but it's best we check!

	// Check if on a susceptible cell, if so then chance of infection (during Output phase)
	lifespan += dt;
	if ( w->isOverCell(pos.x, pos.y) ) {
		Cell * newCell = w->getCell(pos.x, pos.y);

		if (newCell != currentCell){
			if (currentCell!=0 && currentCell->isSusceptible()) {
				// Last cell was a susceptible cell - failed to infect so log "attempt"
				w->EventReporter(World::EVENT_FAILEDINFECTION);
			}
		}
		currentCell = newCell;

	} else if (currentCell != 0) {
		// Now not on a cell, and didn't manage to infect the previous cell
		if (currentCell->isSusceptible()) {
			// Last cell was a susceptible cell - failed to infect so log "attempt"
			w->EventReporter(World::EVENT_FAILEDINFECTION);
		}
		currentCell = 0;
	}
}

void Virion::doOutput(float dt, World *w) {

	if (dead) { return; } // This shouldn't hit, but it's best we check!

	// Maybe move, maybe infect, maybe die
	// First: chance of death
	//cout << this << " Virion CurrentCell is " << currentCell << endl;
	if ( randf(0,1) <= r_death*dt ) {
		cout << this << " Virion DEAD!\n";
		dead = true;
		active = false; // Will be removed from the ActiveAgents queue (I think this is the only reference to the virion!)
	} else if ( currentCell != 0
			&& !currentCell->isInfected()
			&&  currentCell->isSusceptible()
			&&  randf(0,1) <= r_infect*dt ) {
				// Chance of infection: also add newly infected cell to ActiveAgents
				currentCell->setInfected(true);
				w->addAgent(currentCell);
				active = false;
				dead = true;
	} else {
		// Just move
		pos.x += dt*speed*sin(angle);
		pos.y += dt*speed*cos(angle);

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

		// Kill the virion if it goes off the screen
		//if ( pos.x < 0 || pos.x >= conf::WIDTH || pos.y < 0 || pos.y >= conf::HEIGHT ) {
		//	active = false;
		//}

		// Also kill the virion if it goes out of the bounding box (probably won't infect any more cells)
		// Check if we should draw this virion
		//if (pos.x < w->bounding_max.x && pos.x > w->bounding_min.x && pos.y < w->bounding_max.y && pos.y > w->bounding_min.y) {
		//	active = true;
		//	drawable = true;
		//} else {
		//	active   = false;
		//	drawable = false;
		//}
	}
}

void Virion::printInfo() {
	cout << this << " Virion ";
	printf( "(%.2fs)\n", lifespan);
}
