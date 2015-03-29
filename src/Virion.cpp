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

Virion::Virion(Cell *pCell) {

	// Set random direction
	angle = randf(-M_PI, M_PI);
	speed = 1.0; // TODO: we should be able to change this!

	pos 	= pCell->pos;
	active 	= true;
	dead 	= false;
	lifespan = 0.0;

	r_death  = 1/6 * 1/(60*60); 	/* 6 hours to clearance */
	r_infect = 1.0; 				/* No idea, arbitrary value (ensure infection when hitting a cell) */

	parentCell 	= pCell;
	currentCell = 0; // Although technically we're on the parent when we are created
}

Virion::~Virion() {
	// Auto-generated destructor stub
	cout << this << " Virion: destructor called!\n";
}


/* AbstractAgent functions */

// This should only be called from the view!
void Virion::drawAgent(GLView * v) {

	// We initially drew the virion as a green dot, but now we draw it as a green circle of radius 1 with a black border.
	// It still only infects if the center (pos.{x,y}) is over a susceptible cell.

	/*	// Draw a green dot
		glBegin(GL_POINTS);
		glColor3f(0,0,0);
		glVertex2f(pos.x,pos.y);
		glEnd();
	 */

	int r = 1;
	// 1. Draw the body (simple circle)
    glBegin(GL_POLYGON);
    glColor3f(0.0,1.0f,0.0);
    v->drawCircle(pos.x, pos.y, r);
    glEnd();

    // 2. Draw a black line around the circle
    glBegin(GL_LINES);
    glColor3f(0,0,0); // Black outline
    // Draw outline as set of 16 lines around the circumference
    float n;
    for (int k=0;k<17;k++) {
        n = k*(M_PI/8);
        glVertex3f(pos.x+r*sin(n), pos.y+r*cos(n), 0);
        n = (k+1)*(M_PI/8);
        glVertex3f(pos.x+r*sin(n), pos.y+r*cos(n), 0);
    }
    glEnd();

    /*	// This should be made optional. Draw green line from parent cell to virion.
		glBegin(GL_LINES);
		glColor3f(0,1,0);
		glVertex2f(parentCell->pos.x,parentCell->pos.y);
		glVertex2f(pos.x, pos.y);
		glEnd();
     */
}

void Virion::setInput(float dt, World *w) {

	if (dead) { return; } // This shouldn't hit, but it's best we check!

	// Check if on a susceptible cell, if so then chance of infection (during Output phase)
	lifespan += dt;
	if ( w->isOverCell(pos.x, pos.y) ) {
		currentCell = w->getCell(pos.x, pos.y);
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
	} else if ( currentCell!= 0
			&& !currentCell->isInfected()
			&&  currentCell->isSusceptible()
			&& (randf(0,1) <= r_infect*dt) ) {
		//cout << this << " Virion Trying to infect cell " << currentCell << endl;
		// Chance of infection: also add newly infected cell to ActiveAgents
		currentCell->setInfected(true);
		w->addAgent(currentCell);
		active = false;
		dead = true;
	} else {
		// Just move
		pos.x += dt*speed*sin(angle);
		pos.y += dt*speed*cos(angle);

		// Kill the virion if it goes off the screen
		if ( pos.x < 0 || pos.x >= conf::WIDTH || pos.y < 0 || pos.y >= conf::HEIGHT ) {
			active = false;
		}
	}
}

void Virion::printInfo() {
	cout << this << " Virion ";
	printf( "(%.2fs)\n", lifespan);
}
