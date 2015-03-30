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

#ifndef IMMUNEBOTS_NOSERIALISATION
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#endif

class GLView;

class Cell: public AbstractAgent {

public:
	Cell();
	Cell(int x, int y, ImmunebotsSetup*);
	void init(ImmunebotsSetup*);
	virtual ~Cell();

    // The following are AbstractAgent classes, which we make concrete in this class
	void drawAgent(GLView * const);
	void setInput(float dt, World * const);
	void doOutput(float dt, World * const);
	void printInfo();

    // Variables to define the cell
    float radius;
	float lastScanTime; // last time this cell was scanned by CTL

	void setCellType(int newType);

	// Set Cell Type functions (really just wrappers for setCellType)
	void setInfected(bool isInfected);
	void setSusceptible(float sp);
	void setKilled();
	void setScanTime(float wt);
	void toggleInfection();
	bool isInfected();
	bool isSusceptible();

    // Note: Ensure that Cell::setColourFromType() has a case for all of the above Cell Types

private:

	/* Rate parameters, in seconds*/
	float r_vproduction; // Rate of virion production
	float r_death; // Infected cell death rate

#ifndef IMMUNEBOTS_NOSERIALISATION
	// For serialisation:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
    	ar & boost::serialization::base_object<AbstractAgent>(*this);

    	ar & pos;
        ar & angle;
        ar & speed;
        ar & radius;
        ar & agent_type;
        ar & active;
        ar & drawable;
        ar & draw_priority;
        ar & lifespan;

    }
#endif

};

#endif /* CELL_H_ */
