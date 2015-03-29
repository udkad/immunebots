/*
 * ImmunebotsSetup.h
 *
 *  Created on: Apr 21, 2011
 *      Author: ulrich
 */

#ifndef IMMUNEBOTSSETUP_H_
#define IMMUNEBOTSSETUP_H_

#include <iostream>

using namespace std;

class ImmunebotsSetup {

public:
	ImmunebotsSetup();
	virtual ~ImmunebotsSetup();

	/* The setup variables */
	bool useGlut;
	int endTime;
	string setupfilename;
	string id;
	string layoutfilename;

	/* The setup functions */
	void processSetupFile();
	void setEndTime(const char*);

};

#endif /* IMMUNEBOTSSETUP_H_ */
