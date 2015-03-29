/*
 * CTL.cpp
 *
 *  Created on: Apr 26, 2011
 *      Author: ulrich
 */

#include "CTL.h"
#include "Cell.h"

CTL::CTL() {
	init();
}

CTL::CTL(int x, int y) {
	init();
	pos.x = x;
	pos.y = y;
}

void CTL::init() {
	// CTL have a position, and a speed
    pos   = Vector2f(randf(0,conf::WIDTH),randf(0,conf::HEIGHT));
    angle = randf(-M_PI,M_PI);
    speed = 1.0;
    radius = conf::BOTRADIUS;

    currentCell = 0;
    lastCell = 0;

    // Rates (in secs)

    active = true;
    lifespan = 0.0;
    timer = 0.0;
    state = STATE_MOVE;
}

CTL::~CTL() {
	// Auto-generated destructor stub
}

/* AbstractAgent functions */

// Draw the CTL
void CTL::drawAgent(GLView * v) {

	// 1. Draw the body (simple circle)
    glBegin(GL_POLYGON);
    glColor3f(conf::COLOUR_CTL[0],conf::COLOUR_CTL[1],conf::COLOUR_CTL[2]);
    v->drawCircle(pos.x, pos.y, radius);
    glEnd();

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
}

// CTL take input: check if they are on a cell (which may be infected)
void CTL::setInput(float dt, World * w) {
	lifespan += dt;

	// Is the CTL over a cell?
	if ( w->isOverCell(pos.x, pos.y) ) {
		currentCell = w->getCell(pos.x, pos.y);
	}

	if ( currentCell == 0 && state != STATE_MOVE ) {
		cout << this << "CTL ERROR: Not on a cell but state is sense or kill ("<< state<< ")\n";
	}

	if (state == STATE_MOVE) {
		// Decrement the persistence length "timer"
		timer -= dt*speed;

		// If we're over a new cell, then "sense" it
		if (currentCell!=0 && currentCell!=lastCell) {
			state = STATE_SENSE;
			timer = randf(25,60); // 25s-60s for sensing this cell
		}

	} else if ( state == STATE_SENSE ) {
		// 1. Have we finished sensing the current cells? If so - start moving
		timer -= dt;
		if (timer <= 0) {
			if (currentCell!=0 && currentCell->isInfected()) {
				// Start killing the cell
				state = STATE_KILL;
				timer = (30.0 + randf(-5,5))*60; // Killing takes about 30 mins (*60=s)
			} else {
				state = STATE_MOVE;
				angle = randf(-M_PI,M_PI);
				timer = getPersistenceLength();
				lastCell = currentCell;
				currentCell = 0;
			}
		} // else do nothing.. just let timer tick down
	} else if ( state == STATE_KILL ) {
		timer -= dt;
		// Not sure when the CTL actually kills the cell: at some point during the 30mins!
		if (currentCell != 0 &&  !currentCell->isDead() ) {
			if ( randf(0,1) <= (1.0/timer)) currentCell->setKilled();
		}
		if (timer <= 0) {
			// Cell must be dead by now.
			currentCell->setKilled();
			// CTL starts moving
			cout << this << " CTL has killed cell ("<< currentCell << "); moving on\n";
			state = STATE_MOVE;
			angle = randf(-M_PI,M_PI);
			timer = getPersistenceLength();
			lastCell = currentCell;
			currentCell = 0;
		}
	}

}

float CTL::getPersistenceLength() {
	// TODO: Allow persistence lengths of 0 (i.e. immediate angle rotations)
	return (radius*5.0 + randf(-1.0,1.0));
}

// CTL can do the following actions: sense cell (25s-1min), kill cell (30m), move.
void CTL::doOutput(float dt, World *w) {

	if ( state == STATE_MOVE ) {
		// Just move (for now)
		pos.x += dt*speed*sin(angle);
		pos.y += dt*speed*cos(angle);

		// Reset the angle once we reach the end of our persistence length
		if ( timer <= 0 ) {
			angle = randf(-M_PI, M_PI);
			timer = getPersistenceLength();
		}

		// TODO: Change this (make it switchable: deactivate, bound, wrap)

		// OFFSCREEN 1: Torus wrap
	    if (pos.x<0) 		pos.x=1800+pos.x;
	    if (pos.x>=1800) 	pos.x=pos.x-1800;
	    if (pos.y<0) 		pos.y=1000+pos.y;
	    if (pos.y>=1000)	pos.y=pos.y-1000;

		// OFFSCREEN 2: Kill the CTL
		if ( pos.x < 0 || pos.x >= conf::WIDTH || pos.y < 0 || pos.y >= conf::HEIGHT ) {
			active = false;
		}
	} // else: does nothing (sits on cell)

}

void CTL::printInfo() {
	cout << this << " CTL ";
	printf( "(%.2fs)\n", lifespan);
}

/* Other functions */
