/*
 * ImmunebotsSetup.h
 *
 *  Created on: Apr 21, 2011
 *      Author: ulrich
 */

#ifndef IMMUNEBOTSSETUP_H_
#define IMMUNEBOTSSETUP_H_

#include <iostream>
#include <unordered_map>
#include "World.h"
#include "AbstractAgent.h"
#include "CTL.h"

using namespace std;

class ImmunebotsSetup {

public:
	ImmunebotsSetup();
	virtual ~ImmunebotsSetup();
	void setWorld(World *);
	float getWorldTime();

	/* The setup variables */
	bool useGlut;
	bool automaticRun;
	string id;
	int endTime;
	string setupfilename;
	string layoutfilename;
	string reportfilename; // aka log file aka stats
	string reportdir;

	/* The setup functions */
	void processSetupFile();
	bool hasReachedEnd();
	void setEndTime(const char*);

	float getParm(std::string, float);
	int   getParm(std::string, int);
	void  setParm(std::string, float);
	void  setParm(std::string, int);

	Vector2<int> getBounding( bool getMin );

private:

	World * world;

	void setParm(std::string, std::string);

	int getTimeInSeconds(const char*);

	void handleEvents(std::string, std::string);
	void handleCommands(std::string, std::string);
	void handleEndCondition(std::string, std::string);

	int Agent2Type( string );
	string Type2Agent( int );

	void placeNonsusceptibleCells();

	/* Rates and AbstractAgent parameters */
	std::unordered_map<std::string, float> parameters;

};

#endif /* IMMUNEBOTSSETUP_H_ */
