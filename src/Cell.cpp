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

#ifndef IMMUNEBOTS_NOSERIALISATION
BOOST_CLASS_EXPORT_GUID(Cell, "Cell");
#endif

// Don't use this..
Cell::Cell() {}

// These cells are AbstractAgents, even though they are only "active" if infected. They also don't move at all.
Cell::Cell(int x, int y, ImmunebotsSetup * _ibs) {
	ibs = _ibs;
	init(_ibs);
	pos.x = x;
	pos.y = y;
}

void Cell::init(ImmunebotsSetup * ibs){

	// Cells have a position, and a speed
    pos   = Vector2f(randf(0,ibs->getParm("WIDTH",0)),randf(0,ibs->getParm("HEIGHT",0)));
    angle = randf(-M_PI,M_PI);
    speed = 0.00001;

    //nearestPatch = Vector2f(conf::WIDTH/2, conf::HEIGHT/2);
    radius = conf::BOTRADIUS;
    agent_type = AGENT_NOT_SUSCEPTIBLE;
    //lastScanTime = 0.0;

    // Rates (in secs)
    // TODO: This should be set (overwritten perhaps!) by the virion.
    r_vproduction = ibs->getParm("r_vproduction", 0.0023f);// = 200 * 1/(24*60*60); // 200 virions per day
    r_death = ibs->getParm("r_ideath",0.000005787f); // = 1/2*1/(24*60*60); // Infected cell death rate: Half life of approx 2 days

    active = false; // Only active if infected, this is set later.
    lifespan = 0.0;

    // True by default: these cells do no move AND define the boundary box
    drawable = true;
    draw_priority = 0.01; // low priority

    dead = false;

    // Set up the infection shadow, for the "No Virions - Closest" scenario
    if ( ibs->getParm("v_boundary", AbstractAgent::BOUNDARY_NONE) == AbstractAgent::BOUNDARY_WRAP ) {
		int x_dist = (int)(conf::BOTRADIUS*2 + ibs->getParm("nc_cellspacing", 5));
		int infectionshadowinit[8][2] = {
			{-x_dist,-x_dist},
			{-x_dist,0},
			{-x_dist,+x_dist},
			{0,-x_dist},
			{0,+x_dist},
			{+x_dist,-x_dist},
			{+x_dist,0},
			{+x_dist,+x_dist}
		};
		memmove( infectionshadow, infectionshadowinit, sizeof(infectionshadowinit) );
    }

    lastScanTime = 0.0;

}

Cell::~Cell() {}

/* AbstractAgent functions */

// Cells are not drawn using this function: see GLView::drawCell()
void Cell::drawAgent(GLView * v) {
}

void Cell::setScanTime(float wt) {
	lastScanTime = wt;
}

// Infected cells take input, N and S do nothing
void Cell::setInput(float dt, World * w) {
	lifespan += dt;
}

// Infected cells produce virions
void Cell::doOutput(float dt, World *w) {

	if (agent_type == AGENT_INFECTED) {
		// Infected cells can die or produce virions
		if ( randf(0,1) <= r_death*dt ) {
			setKilled();
			// Notify the world event reporter of this virus-induced death
			w->EventReporter(World::EVENT_INFECTEDDEATH_VIRUS);
		} else if ( randf(0,1) <= r_vproduction*dt ) {
			Virion *v = new Virion(this, w->ibs);
			w->addAgent( v );
			w->stats->virion++;
		}
	} else if (agent_type == AGENT_INFECTED_NOVIRIONS) {
		if ( randf(0,1) <= r_death*dt ) {
			setKilled();
			// Notify the world event reporter of this virus-induced death
			w->EventReporter(World::EVENT_INFECTEDDEATH_VIRUS);
		} else if ( randf(0,1) <= w->ibs->getParm("r_vinfect",0.00005f)*dt ) { // else pick a susceptible cell at random and infect!
			// pick a susceptible cell at random and infected as AGENT_INFECTED_NOVIRIONS
			if ( w->stats->susceptible > 0 ) {
				w->infectCells(1, true, true); // Note: only infect susceptible cells, and always infect at a random location
				w->stats->infected++;
				w->stats->susceptible--;
			}
		}
	} else if (agent_type == AGENT_INFECTED_NOVIRIONS_CLOSEST) {
			if ( randf(0,1) <= r_death*dt ) {
				setKilled();
				// Notify the world event reporter of this virus-induced death
				w->EventReporter(World::EVENT_INFECTEDDEATH_VIRUS);
			} else if ( randf(0,1) <= w->ibs->getParm("r_vinfect",0.00005f)*dt ) {
				// Else, get all cells around it and infect a susceptible cell at random (as AGENT_INFECTED_NOVIRIONS_CLOSEST)
				if ( w->stats->susceptible > 0 ) {

					// Loop through all neighbours, check if they are susceptible
					int boundary = w->ibs->getParm("v_boundary", AbstractAgent::BOUNDARY_NONE);
					Cell * currentCell;
					std::vector<Cell*> infectees;

					for (int i=0; i<8; i++) {

						float xwrap = pos.x+infectionshadow[i][0];
						float ywrap = pos.y+infectionshadow[i][1];

						if (boundary == AbstractAgent::BOUNDARY_WRAP) {

							// If we're in the first or last row of cells:
							if ( xwrap <= w->bounding_min.x + w->ibs->getParm("nc_cellspacing", 5) ) {
								xwrap = w->bounding_max.x - conf::BOTRADIUS - w->ibs->getParm("nc_cellspacing", 5);
								//cout << " -- looking from (" << pos.x << "," << pos.y << ") to (" << xwrap << "," << ywrap << ")\n";
							} else if ( xwrap >= w->bounding_max.x - w->ibs->getParm("nc_cellspacing", 5) ) {
								xwrap = w->bounding_min.x + conf::BOTRADIUS + w->ibs->getParm("nc_cellspacing", 5);
								//cout << " -- looking from (" << pos.x << "," << pos.y << ") to (" << xwrap << "," << ywrap << ")\n";
							}

							// If we're in the first or last column of cells:
							if ( ywrap <= w->bounding_min.y + w->ibs->getParm("nc_cellspacing", 5) ) {
								ywrap = w->bounding_max.y - conf::BOTRADIUS - w->ibs->getParm("nc_cellspacing", 5);
								//cout << " -- looking from (" << pos.x << "," << pos.y << ") to (" << xwrap << "," << ywrap << ")\n";
							} else if ( ywrap >= w->bounding_max.y - w->ibs->getParm("nc_cellspacing", 5) ) {
								ywrap = w->bounding_min.y + conf::BOTRADIUS + w->ibs->getParm("nc_cellspacing", 5);
								//cout << " -- looking from (" << pos.x << "," << pos.y << ") to (" << xwrap << "," << ywrap << ")\n";
							}

						}

						if (w->isOverCell( xwrap, ywrap )) {
							currentCell = w->getCell(xwrap, ywrap);
							// Add to vector of potential infectees
							if (currentCell!=0 && currentCell->isSusceptible()) infectees.push_back(currentCell);
						}


					}

					// Check length of potential infectees, then choose one at random to infect. Shuffle, return first, infect.
					if ( infectees.size() > 0 ) {
						std::random_shuffle(infectees.begin(), infectees.end());
						w->toggleInfection(infectees[0]->pos.x, infectees[0]->pos.y);
						infectees[0]->setInfected(true, AbstractAgent::AGENT_INFECTED_NOVIRIONS_CLOSEST);
						w->stats->infected++;
						w->stats->susceptible--;
					}
				}
			}
		}

}

void Cell::printInfo() {
	cout << this << " Cell ";
	printf( "(%.2fs)\n", lifespan);
}

/* Other functions */

void Cell::setKilled() {
	agent_type = AGENT_DEADCELL;
	dead = true;
	active = false; // Will be removed from the ActiveAgents queue
}

void Cell::setCellType(int newType) {
	// If there is a change from cellType to newType
	if ( newType != agent_type ) {
		agent_type = newType;
	}
}

void Cell::setSusceptible(float sp) {
	if ( randf(0.0,1.0) < sp/100.0) {
		setCellType(AGENT_SUSCEPTIBLE);
	}
}

bool Cell::isInfected() {
	// This is where a bit mask would have been useful
	return ( agent_type == AGENT_INFECTED || agent_type == AGENT_INFECTED_NOVIRIONS || agent_type == AGENT_INFECTED_NOVIRIONS_CLOSEST );
}

bool Cell::isSusceptible() {
	return (agent_type == AGENT_SUSCEPTIBLE);
}

// This should only be called in a VIRION scenario. If called in any other scenario, then set the cell type to the correct infection.
// Should use setInfected(bool,int) whenever possible!
void Cell::setInfected(bool infected) {
	setInfected(infected, AGENT_INFECTED);
}

void Cell::setInfected(bool infected, int infection_type) {
	if (infected) {
		setCellType(infection_type);
	} else {
		setCellType(AGENT_NOT_SUSCEPTIBLE);
	}
	active = infected;
}

// If infected, then cell is an ActiveAgent
void Cell::toggleInfection() {
	setInfected( !(agent_type==AGENT_INFECTED) );
}

Vector2<int> Cell::getBounding(bool getMin) {
	return( ibs->getBounding(getMin) );
}
