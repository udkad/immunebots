/*
 * Virion.h
 *
 *  Created on: Apr 15, 2011
 *      Author: ulrich
 */

#ifndef VIRION_H_
#define VIRION_H_

#include "AbstractAgent.h"
#include "Cell.h"
#include "ImmunebotsSetup.h"

class Virion: public AbstractAgent {

public:
	Virion();
	Virion(Cell* const, ImmunebotsSetup*);
	virtual ~Virion();

	// AbstractAgent functions
	void drawAgent(GLView * const);
	void setInput(float, World*);
	void doOutput(float, World*);
	void printInfo();
	string getAgentType() { return string("Virion"); }

private:
	Cell *currentCell, *parentCell;

	/* Virion rates */
	float r_death;// = 1/6*1/(60*60); 	/* 6 hours to clearance */
	float r_infect;// = 1.0; 			/* No idea, arbitrary value (ensure infection when hitting a cell) */

};

#endif /* VIRION_H_ */
