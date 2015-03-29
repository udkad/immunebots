/*
 * Cell.cpp
 *
 *  Created on: Mar 28, 2011
 *      Author: ulrich
 */

#include "Cell.h"

#include "settings.h"
#include "helpers.h"
#include <stdio.h>
#include <iostream>
#include <string>

using namespace std;

// Cells are like Agents, but they can't move (much)
Cell::Cell() {

	// Cells have a position, and a speed
    pos   = Vector2f(randf(0,conf::WIDTH),randf(0,conf::HEIGHT));
    angle = randf(-M_PI,M_PI);
    speed = 0.00001;
    id    = 0;
    nearestPatch = Vector2f(conf::WIDTH/2, conf::HEIGHT/2);
    radius = conf::BOTRADIUS;
    cType = CELL_NOT_SUSCEPTIBLE;
    isSelected = false;
    isInFocus = false;
    isInfected = false;

    //in.resize(INPUTSIZE, 0);
    //out.resize(OUTPUTSIZE, 0);
}

Cell::~Cell() {
}


void Cell::setCellType(int newType) {
	// If there is a change from cellType to newType
	if ( newType != cType ) {
		cType = newType;
	}
}

void Cell::setSusceptible(float sp) {
	if ((rand() % 10001)/100.0 <= sp) {
		setCellType(CELL_SUSCEPTIBLE);
	}
}

void Cell::setInfected(bool i) {
	isInfected = i;
	if (isInfected) {
		setCellType(CELL_INFECTED);
	} else {
		setCellType(CELL_NOT_SUSCEPTIBLE);
	}
}

void Cell::toggleSelected() {
	isSelected = !isSelected;
}

void Cell::setInFocus(bool f) {
	isInFocus = f;
}

void Cell::toggleInfection() {
	setInfected(!isInfected);
}

// Allow a 1 in patchNum chance of setting this (new) patch as the Cell's preferred home"
void Cell::setNewPatch(int x, int y, int patchNum) {
	//cout<< "Will I choose new patch at ("<<x<<","<<y<<")? 1 in "<< 1.0/patchNum << " chance.\n";
	if ( randf(0,1) < 1.0/patchNum ) {
		nearestPatch = Vector2f( x, y );
	}
}

// Given a vector of patches, choose one at random, get the patch location and set as "nearest" patch
void Cell::chooseRandomPatch( std::vector<int> patchVector ) {
	// Randomly select an int between 0 and patchVector.size()
	int randomPatchI = randi(0,patchVector.size());
	//randomPatchI
	int patchI = patchVector[randomPatchI];
	nearestPatch = Vector2f( int(patchI / conf::WIDTH), patchI % conf::WIDTH );
}

// TODO: This needs to be renamed I think
void Cell::setClosestPatch(int * patch) {

	// Find nearest patch to home into..
	float nearest = -1; // Very big number to start with
	int nearestX = 0;
	int nearestY = 0;

	// Exhaustive search - loop through entire patch directory until we find the closest dot.
	for (int i=0; i<conf::WIDTH; i++) {
		for (int j=0; j<conf::HEIGHT; j++) {
			if ( *(patch + conf::HEIGHT*i + j) == 1 ) {
				// Calculate the distance from patch (i,j) to x,y
				float newdistance = pow((i-pos.x),2)+pow((j-pos.y),2);
				if (newdistance<nearest || nearest == -1) {
					nearestX = i;
					nearestY = j;
					nearest = newdistance;
				}
			}
		}
	}
	nearestPatch = Vector2f( nearestX, nearestY );
}

// Given a new patch location, check if it is closer than nearestPatch (and set new nearestPatch if not)
void Cell::checkClosestPatch(int x, int y) {
	float d1 = pow((pos.x-nearestPatch.x),2) + pow((pos.y-nearestPatch.y),2);
	float d2 = pow((pos.x-x),2) + pow((pos.y-y),2);
	if ( d1 > d2 ) {
		nearestPatch = Vector2f( x, y );
	}
}

/* UNUSED FUNCTIONS */

void Cell::printSelf() {
    printf("I am a cell");
}

void Cell::initEvent() {

}


