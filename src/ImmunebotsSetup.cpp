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
	reportfilename = string("log_default");
	reportdir      = ".";

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

	// Return if no setup file was set
	if ( setupfilename.empty() ) { return; }

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
					cout << "- Setting Run ID as '"<<value<<"' from setupfile"<< endl;
				} else {
					cout << "WARNING: Run ID ('"<<value<<"') found in setupfile, ignoring as already set as '"<<id<<"'.\n";
				}
			} else if (boost::iequals(keyword, "endtime")) {
				setEndTime(value.c_str());
			} else if (boost::iequals(keyword, "Stats")) {
				if (boost::iequals(value, "extra")) {
					setParm("stats_extra", 1);
				}
			} else if (boost::iequals(keyword, "layoutfile")) {
				layoutfilename = value;
			} else if (boost::iequals(keyword, "reporttime")) {
				setParm("report_time", getTimeInSeconds(value.c_str()));
			} else if (boost::iequals(keyword, "reportname")) {
				if (boost::iequals(value,"fromid")) {
					reportfilename = id;
				} else {
					reportfilename = value;
				}
			} else if (boost::iequals(keyword, "reportdir")) {
				reportdir = value;
			} else if (boost::iequals(keyword, "reportoverwrite")) {
				setParm("report_overwrite",atoi(value.c_str()));
			} else if ( keyword.at(0) == '!' )  {
				// Handle the commands (all start with '!', e.g. !PLACE)
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

			// Handles Rates, Times, End Conditions and Cell placement variables.
			if (boost::iequals(keyword, "Rate")) {
				setParm(pname, pval);
			} else if (boost::iequals(keyword, "Time")) {
				// Stick the rate/time on the parameters map
				setParm(pname, getTimeInSeconds(pval.c_str()));
			} else if (boost::iequals(keyword, "EndCondition")) {
				// Set up the end conditions, eg. endtime, or when a cell-count reaches a certain %/number
				handleEndCondition(pname, pval);
			} else {
				// Handle Nonsusceptible/Susceptible/Infected/CTL keywords
				boost::to_lower(pname);

				// Catch the "placement" pname first (value is not an integer - so handle special)
				if ( boost::iequals(pname,"PLACEMENT") ) {
					if (boost::iequals(keyword, "Nonsusceptible")) {
						if ( boost::iequals(pval,"squaregrid") ) {
							setParm( string("nc_").append(pval), 1 );
						}
					} else if (boost::iequals(keyword, "Susceptible")) {
						if ( boost::iequals(pval,"random") ) {
							setParm( string("sc_").append(pval), 1 );
						}
					} // else ignore
				} else if ( boost::iequals(pname,"TYPE") ) {
					// Handle the "type" pname (value is not an integer, so handle special)
					if (boost::iequals(keyword, "Infected")) {
						if ( boost::iequals(pval,"withvirions") || boost::iequals(pval,"virions") ) {
							// Use the Novirions agent instead of the (default) Infected cell type
							setParm( string("ic_novirions"), 0 );
						} else if ( boost::iequals(pval,"novirions") ) {
							// Use the Novirions agent instead of the (default) Infected cell type
							setParm( string("ic_novirions"), 1 );
						}
					} // else ignore other 'type' variables
				} else if ( boost::iequals(pname,"BOUNDARY") ) {
					// Catch the Boundary behaviours: wrap, stay, bounce, torus.
					int boundary = AbstractAgent::BOUNDARY_NONE;

					if (boost::iequals(pval,"none")) {
						boundary = AbstractAgent::BOUNDARY_NONE;
					} else if (boost::iequals(pval,"wrap")) {
						boundary = AbstractAgent::BOUNDARY_WRAP;
					} else if (boost::iequals(pval,"bounce")) {
						boundary = AbstractAgent::BOUNDARY_BOUNCE;
					} else if (boost::iequals(pval,"stay")) {
						boundary = AbstractAgent::BOUNDARY_STAY;
					} else if (boost::iequals(pval,"die")) {
						boundary = AbstractAgent::BOUNDARY_DIE;
					}

					if (boost::iequals(keyword, "Virion")) {
						setParm("v_boundary", boundary);
					} else if ( boost::iequals(keyword,"CTL") ) {
						setParm("ctl_boundary", boundary);
					} else {
						cout << "WARNING: Unrecognised boundary condition in setupfile (ignoring..): " << line << endl;
						return;
					}

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
	cout << " - adding parameter "<< parmName <<" ("<< parmValue << ")" << endl;
	parameters[parmName] = parmValue;
}

// Overloaded function
void ImmunebotsSetup::setParm(std::string parmName, int parmValue) {
	setParm(parmName, (float)(parmValue));
}

// Setup file lines will always be converted into a float
void ImmunebotsSetup::setParm(std::string pname, std::string pval) {
	float pvalf = atof( pval.c_str() );
	setParm( pname, pvalf );
}

// Check the EndCondition and stick on the map
void ImmunebotsSetup::handleEndCondition(string key, string value) {
	// Examples: endtime 7d; infectedpc 0.5.
	if ( boost::iequals(key,"endtime") ) {
		setEndTime(value.c_str());
		cout << " - EndCondition of 'World time >= "<< endTime <<"s' set" << endl;
	} else if ( boost::iequals(key,"infectedpc_eq") ) {
		setParm("end_infectedpc_eq",1);
		setParm("end_infectedpc_eqvalue", (float)atof(value.c_str()) );
		cout << " - EndCondition of 'Infected == "<< getParm("end_infectedpc_eqvalue", 0.0f)*100 <<"%' set" << endl;
	} else if ( boost::iequals(key,"infectedpc_ge") ) {
		setParm("end_infectedpc_ge",1);
		setParm("end_infectedpc_gevalue", (float)atof(value.c_str()) );
		cout << " - EndCondition of 'Infected >= "<< getParm("end_infectedpc_gevalue", 0.0f)*100 <<"%' set" << endl;
	} else {
		cout << "WARNING: Could not parse the EndCondition: " << key <<":"<<value << endl;
	}

}

void ImmunebotsSetup::handleCommands(std::string command, std::string value) {

	if (boost::iequals(command, "!place")) {
		// Can place: non-susceptible, infected, ctl
		if (boost::iequals(value,"nonsusceptible")){
			// Place non-susceptible (and by extension, susceptible cells) according to parms already set.
			cout << " - Placing "<< getParm("nc_totalcount", 10000) <<" non-susceptible cells" << endl;
			placeNonsusceptibleCells();
		} else if (boost::iequals(value,"infected")){
			// Infect x susceptible cells
			world->infectCells( getParm("ic_totalcount",1), true);
			cout << " - Infecting "<<getParm("ic_totalcount",1)<<" susceptible cell(s)" << endl;
		} else if (boost::iequals(value,"ctl")){
			world->dropCTL( getParm("ctl_totalcount",0) );
			cout << " - Placing "<< getParm("ctl_totalcount",0) << " CTL" << endl;
		}
	}
}

// Check to see if we have reached the end conditions specified in the setup
bool ImmunebotsSetup::hasReachedEnd() {

	// Check endtime, then any other conditions
	//bool hasEnded;
	if ( endTime!=-1 && world->worldtime >= endTime ) {
		cout << " - World time ("<<world->worldtime<<") is >= the required endTime of " << endTime << endl;
		return(true);
	} else {
		float infected_pc = (world->stats->infected / (float)(world->stats->infected + world->stats->susceptible)) ;
		if ( getParm("end_infectedpc_ge",0)==1 &&
			infected_pc >= getParm("end_infectedpc_gevalue",2.0f) ) {
			cout << " - End condition (Infected% >= "<<getParm("end_infectedpc_gevalue",2.0f)*100<<"%) has been reached" << endl;
			return(true);
		} else if ( getParm("end_infectedpc_eq",0)==1 &&
			infected_pc <= getParm("end_infectedpc_eqvalue",0.0f) ) {
			cout << " - End condition (Infected% == "<<getParm("end_infectedpc_eqvalue",0.0f)*100<<"%) has been reached" << endl;
			return( true );
		}
	}

	return(false);
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


void ImmunebotsSetup::setEndTime(const char* et) {
	endTime = getTimeInSeconds(et);
}

int ImmunebotsSetup::getTimeInSeconds(const char* et) {

	// End time can end with "s" (seconds) or "d" (days)
	boost::regex re("([-.\\d]+)(\\w)?", boost::regex::perl|boost::regex::icase);
	boost::cmatch ma;

	int tis = 0;

    if (boost::regex_match(et, ma, re)) {
    	char units = *ma[2].first; // OK as single character
    	int value = atoi(string(ma[1].first, ma[1].second).c_str()); // Convert string to integer

    	if (units == 'd') {
    		tis = value*24*60*60;
    	} else if (units == 's') {
    		tis = value;
    	} else if (units == 'm') {
    		tis = value*60;
    	} else if (units == 'w') {
    		tis = value*7*24*60*60;
    	} else if (units == 'h') {
    		tis = value*60*60;
    	} else {
    		tis = value; // Default: ignore units
    	}
    } else {
    	cout << "WARNING: could not parse time ("<<et<<") into anything meaningful\n";
    }

	return(tis);
}
