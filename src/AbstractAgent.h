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

#ifndef IMMUNEBOTS_NOSERIALISATION
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#endif

#include "World.h"
#include "GLView.h"

using namespace std;

class AbstractAgent {

protected:

	bool active;
	bool dead;
	float lifespan;
	int boundarycondition;

public:

    // Allowed values for agent_type:
    static const int AGENT_NOT_SUSCEPTIBLE 	= 1;
    static const int AGENT_SUSCEPTIBLE     	= 2;
    static const int AGENT_INFECTED			= 3;
    static const int AGENT_CTL			  	= 4;
    static const int AGENT_VIRION		  	= 5;
    static const int AGENT_INFECTED_NOVIRIONS = 6; // static cell which doesn't produce any virions
    static const int AGENT_DEADCELL			= 99;

    // Boundary conditions
    static const int BOUNDARY_NONE			= 0;
    static const int BOUNDARY_WRAP			= 1;
    static const int BOUNDARY_BOUNCE		= 2;
    static const int BOUNDARY_STAY			= 3;
    static const int BOUNDARY_DIE			= 4;

	bool isActive() {return active;} // If T, remove from ActiveAgents queue
	bool isDead()   {return dead;} // If T, remove from world

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

	int agent_type;
	int getAgentType() { return(agent_type); }

	// Overload these
	virtual void printInfo() { cout << this << " AbstractAgent [I need to be defined]\n"; }; // Change to logoutput?

	// Extension functions:
	// - Predeterminable actions?
	// - a 'register' function? (for actions, options)

private:

#ifndef IMMUNEBOTS_NOSERIALISATION
	// For serialisation:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {}
#endif

};

#ifndef IMMUNEBOTS_NOSERIALISATION
BOOST_SERIALIZATION_ASSUME_ABSTRACT(AbstractAgent);
#endif

#endif /* ABSTRACTAGENT_H_ */
