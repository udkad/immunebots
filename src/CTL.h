/*
 * CTL.h
 *
 *  Created on: Apr 26, 2011
 *      Author: ulrich
 */

#ifndef CTL_H_
#define CTL_H_

#include "vmath.h"
#include "World.h"
#include "AbstractAgent.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>

static int STATE_MOVE  = 0;
static int STATE_SENSE = 1;
static int STATE_KILL  = 2;

class CTL: public AbstractAgent {

public:
	CTL();
	CTL(int,int);
	virtual ~CTL();

    // The following are AbstractAgent classes, which we make concrete in this class
	void drawAgent(GLView * const);
	void setInput(float dt, World * const);
	void doOutput(float dt, World * const);
	void printInfo();
	string getAgentType() { return string("CTL"); }

private:

	void init();

	int state; // Current state of the CTL (e.g. move, sense, kill)
	Cell *currentCell;
	Cell *lastCell;
	float timer;

	/* Helper functions */
	float getPersistenceLength(); // returns a persistence length between (4.0,6.0)

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

        ar & state;
        ar & currentCell;
        ar & lastCell;
        ar & timer;

    }

};

#endif /* CTL_H_ */
