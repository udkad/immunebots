/*
 * Cell.cpp
 *
 *  Created on: Mar 28, 2011
 *      Author: ulrich
 */

#include "Cell.h"
#include "World.h"
#include "Virion.h"

#include "settings.h"
#include "helpers.h"

#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;

BOOST_CLASS_EXPORT_GUID(Cell, "Cell");

Cell::Cell() {
	init();
}

// These cells are AbstractAgents, even though they are only "active" if infected. They also don't move at all.
Cell::Cell(int x, int y) {
	init();
	pos.x = x;
	pos.y = y;
}

void Cell::init(){

	id = rand();

	// Cells have a position, and a speed
    pos   = Vector2f(randf(0,conf::WIDTH),randf(0,conf::HEIGHT));
    angle = randf(-M_PI,M_PI);
    speed = 0.00001;

    //nearestPatch = Vector2f(conf::WIDTH/2, conf::HEIGHT/2);
    radius = conf::BOTRADIUS;
    cType = CELL_NOT_SUSCEPTIBLE;
    lastScanTime = 0.0;

    // Rates (in secs)
    // TODO: This should be set by the virion!
    r_vproduction = 0.0023;// = 200 * 1/(24*60*60); // 200 virions per day
    r_death = 0.000005787; // = 1/2*1/(24*60*60); // Infected cell death rate: Half life of approx 2 days

    active = false;
    lifespan = 0.0;

    // True by default: these cells do no move AND define the boundary box
    drawable = true;
    draw_priority = 0.01; // low priority

    dead = false;

}

Cell::~Cell() {}

/* AbstractAgent functions */

// Cells are not drawn using this function
void Cell::drawAgent(GLView * v) {
}

void Cell::setScanTime(float wt) {
	lastScanTime=wt;
}

// Infected cells take input, N and S do nothing
void Cell::setInput(float dt, World * w) {
	lifespan += dt;
}

// Infected cells produce virions
void Cell::doOutput(float dt, World *w) {

	if (cType == CELL_INFECTED) {
		// Infected cells can die or produce virions
		if ( randf(0,1) <= r_death*dt ) {
			setKilled();
		} else if ( randf(0,1) <= r_vproduction*dt ) {
			Virion *v = new Virion(this);
			w->addAgent( v );
		}
	}

}

void Cell::printInfo() {
	cout << this << " Cell ";
	printf( "(%.2fs)\n", lifespan);
}

/* Other functions */

void Cell::setKilled() {
	cType = CELL_DEAD;
	dead = true;
	active = false; // Will be removed from the ActiveAgents queue
}

void Cell::setCellType(int newType) {
	// If there is a change from cellType to newType
	if ( newType != cType ) {
		cType = newType;
	}
}

void Cell::setSusceptible(float sp) {
	if ( randf(0.0,1.0) < sp/100.0) {
		setCellType(CELL_SUSCEPTIBLE);
	}
}

bool Cell::isInfected() {
	return (cType == CELL_INFECTED);
}

bool Cell::isSusceptible() {
	return (cType == CELL_SUSCEPTIBLE);
}

void Cell::setInfected(bool infected) {
	if (infected) {
		setCellType(CELL_INFECTED);
	} else {
		setCellType(CELL_NOT_SUSCEPTIBLE);
	}
	active = infected;
}

// If infected, then cell is an ActiveAgent
void Cell::toggleInfection() {
	setInfected( !(cType==CELL_INFECTED) );
}

