/*
 * Cell.h
 *
 *  Created on: Mar 28, 2011
 *      Author: ulrich
 */

#ifndef CELL_H_
#define CELL_H_

#include "vmath.h"

#include <vector>
#include <string>

class Cell {

public:
	Cell();
	Cell(int x, int y);
	virtual ~Cell();

    void printSelf();
    //for drawing purposes
    void initEvent();

    Vector2f pos;
    float angle;
    float speed;
    int id;
    float radius;
    bool isSelected;
    bool isInFocus;
    bool isInfected;

	// This is one of the CELL_ types
	int cType;
	void setCellType(int newType);

	// Set Cell Type functions (really just wrappers for setCellType)
	void setInfected(bool isInfected);
	void setSusceptible(float sp);
	void toggleInfection();

	void toggleSelected();
	void setInFocus(bool f);

    // Keep a record of where we're trying to get to
    Vector2f nearestPatch;
    void setClosestPatch(int * patch);
    void checkClosestPatch(int x, int y);
    void chooseRandomPatch( const std::vector<int> patchVector );
    void setNewPatch(int x, int y, int patchNum);

    static const int CELL_NOT_SUSCEPTIBLE = 0;
    static const int CELL_SUSCEPTIBLE     = 1;
    static const int CELL_INFECTED		  = 2;
    static const int CELL_CTL			  = 3;
    static const int CELL_DEAD			  = 99;
    // Note: Ensure that Cell::setColourFromType() has a case for all of the above Cell Types


private:

};

#endif /* CELL_H_ */
