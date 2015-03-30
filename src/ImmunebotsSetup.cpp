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
	endTime = 7*(24*60*60); // End at day 7 (note: integer & in seconds!)
	setupfilename  = string("");
	layoutfilename = string("Setup-Grant.sav");

	// Create (and initialise with defaults) the parameters (all floats) for:
	// - Virions:
	parameters["r_vinfect"]     = 0.1;			// arbitrary
	parameters["r_vproduction"] = 0.0023;		// = 200 * 1/(24*60*60); // 200 virions per day
    parameters["r_vspeed"] 		= 0.1;			// arbitrary
    // - Infected cells:
    parameters["r_ideath"]      = 0.000005787; 	// = 1/2*1/(24*60*60); // Infected cell death rate: Half life of approx 2 days
    // - CTL:
    parameters["r_ctlspeed"]    = 1.0;

    parameters["nc_susceptiblepc"] = 2.0;
}

void ImmunebotsSetup::setWorld(World * w) {
	world = w;
}

ImmunebotsSetup::~ImmunebotsSetup() {
	// Auto-generated destructor stub
}

// Setup files are processed here. A single setup file is specified on the commandline with "-s SETUPFILENAME".
void ImmunebotsSetup::processSetupFile() {
	ifstream sfile(setupfilename);

	if (!sfile.good()) {
		cout << "WARNING: Could not find the setup file ('"<<setupfilename<<"')\n";
		// Can't go any further, return
		return;
	}
	// Parse the setup file
	string line;
	// Lots of regex's (2-args, 3-args, Comment (#), Blank line (\s+))
	// Note: 2-args and 3-args now work with comments at the end of the line, eg. "CTL r_lysis 0.001 #arbitrary value"
	boost::regex re2("\\s*([^#][^\\s]+)\\s+([^\\s#]+)(\\s*#.*)?", boost::regex::perl|boost::regex::icase);
	boost::regex re3("\\s*([^#][^\\s]+)\\s+([^\\s]+)\\s+([^#]*?[^#\\s])(\\s*#.*)?", boost::regex::perl|boost::regex::icase);
	boost::regex reC("\\s*#.+", boost::regex::perl|boost::regex::icase);
	boost::regex reS("\\s*", boost::regex::perl|boost::regex::icase);
	boost::cmatch ma;

	while ( sfile.good() ) {
		// Get next line from sfile and put in "line" variable
		getline( sfile, line );
		if ( boost::regex_match(line.c_str(), ma, re2) ) {

			// Matches: Something Something
			string keyword = string(ma[1].first, ma[1].second);
			string value   = string(ma[2].first, ma[2].second);

			if (boost::iequals(keyword, "runid")) {
				// Do not over-write an id set by the command line
				if ( id.empty() ) {
					id = value;
					cout << " -- Setting Run ID as '"<<value<<"' from setupfile"<< endl;
				} else {
					cout << " WARNING: Run ID ('"<<value<<"') found in setupfile, ignoring as already set as '"<<id<<"'.\n";
				}
			} else if (boost::iequals(keyword, "endtime")) {
				setEndTime(value.c_str());
			} else if (boost::iequals(keyword, "layoutfile")) {
				layoutfilename = value;
			} else if ( keyword.at(0) == '!' )  {
				// Handle the commands
				handleCommands(keyword, value);
			} else {
				cout << "WARNING: Unrecognised line in setupfile (ignoring..): " << line << endl;
			}
		} else if ( boost::regex_match(line.c_str(), ma, re3) ) {

			// Matches: Something Something Something
			string keyword = string(ma[1].first, ma[1].second);
			string pname   = string(ma[2].first, ma[2].second);
			string pval    = string(ma[3].first, ma[3].second);

			boost::to_lower(pname);

			// Handles Rates and Cell placement variables.
			if (boost::iequals(keyword, "Rate")) {
				setParm(pname, pval);
			} else {
				// Handle Nonsusceptible/Susceptible/Infected/CTL keywords
				boost::to_lower(pname);

				// Catch the "placement" pname first (value is not an integer - so handle special)
				if ( boost::equals(pname,"placement") ) {
					if (boost::iequals(keyword, "Nonsusceptible")) {
						if ( boost::iequals(pval,"squaregrid") ) {
							setParm( string("nc_").append(pval), 1 );
						}
					} else if (boost::iequals(keyword, "Susceptible")) {
						if ( boost::iequals(pval,"random") ) {
							setParm( string("sc_").append(pval), 1 );
						}
					} // else ignore
				} else if (boost::iequals(keyword, "Nonsusceptible")) {
					setParm( string("nc_").append(pname), pval );
				} else if (boost::iequals(keyword, "Susceptible")) {
					setParm( string("sc_").append(pname), pval );
				} else if (boost::iequals(keyword, "Infected")) {
					setParm( string("ic_").append(pname), pval );
				} else if (boost::iequals(keyword, "CTL")) {
					setParm( string("ctl_").append(pname), pval );
				} else {
					cout << "WARNING: Unable to match 3-arg line: " << line << endl;
				}
			}

		} else if ( boost::regex_match(line.c_str(), ma, reC) || boost::regex_match(line.c_str(), ma, reS) ) {
			// Comment or blank line, ignore.
		} else {
			cout << "WARNING: Unrecognised line in setupfile (ignoring..): " << line << endl;
		}
	}
}

float ImmunebotsSetup::getParm(std::string parmName, float defaultParm) {
	if ( parameters.count(parmName)>0 ) {
		return(parameters[parmName]);
	}

	return(defaultParm);
}

int ImmunebotsSetup::getParm(std::string parmName, int defaultParm) {
	if ( parameters.count(parmName)>0 ) {
		return( floor(parameters[parmName]) );
	}

	return(defaultParm);
}

void ImmunebotsSetup::setParm(std::string parmName, float parmValue) {
	parameters[parmName] = parmValue;
}

// Overloaded function
void ImmunebotsSetup::setParm(std::string parmName, int parmValue) {
	setParm(parmName, (float)(parmValue));
}

// Setup file lines will always be converted into a float
void ImmunebotsSetup::setParm(std::string pname, std::string pval) {
	float pvalf = atof( pval.c_str() );
	cout << " -- Found rate "<< pname <<" with value "<< pvalf << ".\n";
	setParm( pname, pvalf );
}

void ImmunebotsSetup::handleCommands(std::string command, std::string value) {

	if (boost::iequals(command, "!place")) {
		// Can place: non-susceptible, infected, ctl
		if (boost::iequals(value,"nonsusceptible")){
			// Place non-susceptible (and by extension, susceptible cells) according to parms already set.
			cout << "[Setup] Placing non-susceptible cells\n";
			placeNonsusceptibleCells();
			cout << "Now there are " << world->numCells() << " cells in the world\n";
		} else if (boost::iequals(value,"infected")){
			// Infect x susceptible cells
			world->infectCells( getParm("ic_totalcount",1), true);
			cout << "[Setup] Infecting one susceptible cell\n";
			cout << "Now there are " << world->numAgents() << " agents in the world\n";
		} else if (boost::iequals(value,"ctl")){
			// TODO
		}
	}

}

// Will create a (square) grid of susceptible cells
void ImmunebotsSetup::placeNonsusceptibleCells() {

	int startx       = getParm("startx",100);
	int starty       = getParm("starty",100);
	int cell_spacing = getParm("nc_cellspacing", 5);
	int total_cells  = getParm("nc_totalcount", 10000);

	if ( getParm("nc_squaregrid", 0) == 1 ) {
		int square = (int)(sqrt(total_cells));

		for (int i=0; i<square; i++) {
			for (int j=0; j<square; j++) {
				world->addCell(
						startx+((int)(2*conf::BOTRADIUS)+cell_spacing)*i,
						starty+((int)(2*conf::BOTRADIUS)+cell_spacing)*j
				);
			}
		}
	} // else: other options tbd
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
