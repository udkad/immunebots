/*
 * ImmunebotsSetup.cpp
 *
 *  Created on: Apr 21, 2011
 *      Author: ulrich
 */

#include "ImmunebotsSetup.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>

using namespace std;

ImmunebotsSetup::ImmunebotsSetup() {
	// Fill out the default values for all the bools etc
	useGlut = true;
	automaticRun = false;
	endTime = 7*(24*60*60); // End at day 7 (note: in seconds!)
	setupfilename = string("");
	layoutfilename = string("Setup-Grant.sav");
}

ImmunebotsSetup::~ImmunebotsSetup() {
	// Auto-generated destructor stub
}

void ImmunebotsSetup::processSetupFile() {
	ifstream sfile(setupfilename);

	if (!sfile.good()) {
		cout << "WARNING: Could not find the setup file ('"<<setupfilename<<"')\n";
		// Can't go any further, return
		return;
	}
	// Parse the setup file
	string line;
	// Lots of regex's (2-args, 3-args, C [??], S [??])
	boost::regex re2("\\s*([^#][^\\s]+)\\s+([^\\s]+)", boost::regex::perl|boost::regex::icase);
	boost::regex re3("\\s*([^#][^\\s]+)\\s+([^\\s]+)\\s+(.+)", boost::regex::perl|boost::regex::icase);
	boost::regex reC("\\s*#.+", boost::regex::perl|boost::regex::icase);
	boost::regex reS("\\s*", boost::regex::perl|boost::regex::icase);
	boost::cmatch ma;

	while ( sfile.good() ) {
		getline( sfile, line );
		if ( boost::regex_match(line.c_str(), ma, re2) ) {
			// Matches: Something Something
			string keyword = string(ma[1].first, ma[1].second);
			string value = string(ma[2].first, ma[2].second);

			if (boost::iequals(keyword, "runid")) {
				// Do not over-write an id set by the command line
				if ( id.empty() ) {
					id = value;
				}
			} else if (boost::iequals(keyword, "endtime")) {
				setEndTime(value.c_str());
			} else if (boost::iequals(keyword, "layoutfile")) {
				layoutfilename = value;
			} else {
				cout << "WARNING: Unrecognised line in setupfile (ignoring..): " << line << endl;
			}
		} else if ( boost::regex_match(line.c_str(), ma, re3) ) {
			// Matches: Something Something Something
			string agent = string(ma[1].first, ma[1].second);
			cout << " -- Found rate for agent '"<< agent << "'\n";
		} else if ( boost::regex_match(line.c_str(), ma, reC) || boost::regex_match(line.c_str(), ma, reS) ) {
			// Comment or blank line, ignore
		} else {
			cout << "WARNING: Unrecognised line in setupfile (ignoring..): " << line << endl;
		}
	}
}

void ImmunebotsSetup::setEndTime(const char* et){
	// End time can end with "s" (seconds) or "d" (days)
	boost::regex re("(\\d+)(\\w)?", boost::regex::perl|boost::regex::icase);
	boost::cmatch ma;

    if (boost::regex_match(et, ma, re)) {
    	char units = *ma[2].first; // OK as single character
    	int value = atoi(string(ma[1].first, ma[1].second).c_str()); // Convert string to integer

    	if (units == 'd') {
    		endTime = value*24*60*60;
    	} else if (units == 's') {
    		endTime = value;
    	} else if (units == 'w') {
    		endTime = value*7*24*60*60;
    	} else if (units == 'h') {
    		endTime = value*60*60;
    	} else {
    		endTime = value; // Default: ignore units
    	}
    	cout << "End time set to: "<< endTime << "s\n";
    } else {
    	cout << "WARNING: detected an end time (-e) option but could not parse it ("<<et<<")\n";
    }

}
