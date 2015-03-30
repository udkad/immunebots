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
#include "ImmunebotsSetup.h"
//#include <>

#ifndef IMMUNEBOTS_NOSERIALISATION
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/export.hpp>
#endif

class CTL: public AbstractAgent {

public:

	static const int STATE_MAX   = 3;
	static const int STATE_MOVE  = 0;
	static const int STATE_SENSE = 1;
	static const int STATE_KILL  = 2;

	CTL();
	CTL(int,int,ImmunebotsSetup*);
	virtual ~CTL();

    // The following are AbstractAgent classes, which we make concrete in this class
	void drawAgent(GLView * const);
	void setInput(float dt, World * const);
	void doOutput(float dt, World * const);

	// Pass in a 2D vector of dim 3x(CTL number). CTL pushes on the fraction of time spent in each of the 3 states.
	void getStateFraction( vector< vector<float> >& );
	void printInfo();

	float pull;

private:

	void init(ImmunebotsSetup*);

	int state; // Current state of the CTL (e.g. move, sense, kill)
	Cell *currentCell;
	Cell *lastCell;
	Cell *nearestCell;
	float timer;
	float cellshadow[9][2];
	int chemotaxis_type;

	// Temporary variable which colours the CTL differently depending on the current direction mode.
	int chemo_state;

	// Time spent in each state
	float timekeeper[STATE_MAX];
	vector<float> timefrac;

	/* Helper functions */
	float getPersistenceLength(World *); // returns a persistence length between (4.0,6.0)
	float getAngle(World *);

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

        ar & state;
        ar & currentCell;
        ar & lastCell;
        ar & timer;

    }
#endif

};

#endif /* CTL_H_ */
