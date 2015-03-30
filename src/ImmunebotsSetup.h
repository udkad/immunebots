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

using namespace std;

class ImmunebotsSetup {

public:
	ImmunebotsSetup();
	virtual ~ImmunebotsSetup();
	void setWorld(World *);

	/* The setup variables */
	bool useGlut;
	bool automaticRun;
	string id;
	int endTime;
	string setupfilename;
	string layoutfilename;

	/* The setup functions */
	void processSetupFile();
	void setEndTime(const char*);

	float getParm(std::string, float);
	int   getParm(std::string, int);
	void  setParm(std::string, float);
	void  setParm(std::string, int);

private:

	World * world;

	void setParm(std::string, std::string);
	void handleCommands(std::string command, std::string value);

	void placeNonsusceptibleCells();

	/* Rates and AbstractAgent parameters */
	std::unordered_map<std::string, float> parameters;

};

#endif /* IMMUNEBOTSSETUP_H_ */
