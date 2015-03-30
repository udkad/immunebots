/*
 * AbstractAgent.h
 *
 *  Created on: Apr 15, 2011
 *      Author: ulrich
 */

#ifndef ABSTRACTAGENT_H_
#define ABSTRACTAGENT_H_

/*
 * AbstractAgent defines the necessary functions and variables for a class to be an ActiveAgent,
 *   as well as transition from inactive <-> active in both directions.
 */

#include "vmath.h"

#include <iostream>
#include <GL/glut.h> // Required for the drawing utilities

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "World.h"
#include "GLView.h"

using namespace std;

class AbstractAgent {

protected:

	bool active;
	bool dead;
	float lifespan;

public:

	bool isActive() {return active;}; // If T, remove from ActiveAgents queue
	bool isDead() {return dead;}; // If T, remove from world

	// Position-related
	Vector2f pos;
    float angle;
    float speed;
    float radius;

	virtual void drawAgent(GLView * const) {};
	bool drawable;
	float draw_priority; // If drawable, this needs to be set in the range 0-1, with 1 being high priority.

	virtual void setInput(float, World* const) {};
	virtual void doOutput(float, World* const) { cout << "\t"<<this<<" [doOutput not defined]";};

	// Overload these
	virtual void printInfo() { cout << this << " AbstractAgent [I need to be defined]\n"; }; // Change to logoutput?
	virtual string getAgentType() {return "AbstractAgent";}; // Previously just "=0"

	// Extension functions:
	// - Predeterminable actions?
	// - a 'register' function? (for actions, options)

private:


	// For serialisation:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {}

};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(AbstractAgent);

#endif /* ABSTRACTAGENT_H_ */
