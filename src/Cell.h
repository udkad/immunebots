/*
 * Cell.h
 *
 *  Created on: Mar 28, 2011
 *      Author: ulrich
 */

#ifndef CELL_H_
#define CELL_H_

#include "vmath.h"
#include "World.h"
#include "GLView.h"
#include "AbstractAgent.h"

#include <vector>
#include <string>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>

class GLView;

class Cell: public AbstractAgent {

public:
	Cell();
	Cell(int x, int y);
	virtual ~Cell();

    // The following are AbstractAgent classes, which we make concrete in this class
	void drawAgent(GLView * const);
	void setInput(float dt, World * const);
	void doOutput(float dt, World * const);
	void printInfo();
	string getAgentType() { return string("Cell"); }

    // Variables to define the cell
    float radius;
	int cType; 	// This is one of the CELL_ types (should enumerate?)

	void setCellType(int newType);

	// Set Cell Type functions (really just wrappers for setCellType)
	void setInfected(bool isInfected);
	void setSusceptible(float sp);
	void setKilled();
	void toggleInfection();
	bool isInfected();
	bool isSusceptible();

    // Keep a record of where we're trying to get to
    //Vector2f nearestPatch;
    //void setClosestPatch(int * patch);
    //void checkClosestPatch(int x, int y);
    //void chooseRandomPatch( const std::vector<int> patchVector );
    //void setNewPatch(int x, int y, int patchNum);

    // Allowed values for cType:
    static const int CELL_NOT_SUSCEPTIBLE = 0;
    static const int CELL_SUSCEPTIBLE     = 1;
    static const int CELL_INFECTED		  = 2;
    static const int CELL_CTL			  = 3;
    static const int CELL_DEAD			  = 99;
    // Note: Ensure that Cell::setColourFromType() has a case for all of the above Cell Types

private:

	/* Rate parameters, in seconds*/
	float r_vproduction; // Rate of virion production
	float r_death; // Infected cell death rate

	// For serialisation:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<AbstractAgent>(*this);

    	ar & pos;
        ar & angle;
        ar & speed;
        ar & radius;
        ar & active;
        ar & drawable;
        ar & lifespan;

        ar & cType;
    }


};

#endif /* CELL_H_ */
