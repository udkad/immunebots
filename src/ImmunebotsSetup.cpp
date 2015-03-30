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

    // Stats and reporting time: default is 5 mins
    parameters["report_time"]  = 300.0; /* Can be overridden by the user in the setup file */
    parameters["update_stats"] = 300.0; /* Will be overridden if there is an EndCondition or Event */
    //cout << " [DEBUG] update stats was just init - got " << parameters["update_stats"] << "s. Count is "<< parameters.count("update_stats") << "." << endl;

    // Legacy defaults:
    parameters["ctl_immediatekill"] = 0.0; // CTL kill immediately after scanning; keyword is: CTL LYSIS [start|end].
	parameters["ctl_chemotaxis_version"] = 0.0; // default to chemotaxis version 0, i.e. no chemotaxis (same as v1&&0%). For v2 set rho via ctl_chemotaxis_pc.

    // Initial starting dimensions of the world.
    parameters["WIDTH"]  = conf::DefaultWidth;
    parameters["HEIGHT"] = conf::DefaultHeight;
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
	boost::regex reC("\\s*#.*", boost::regex::perl|boost::regex::icase);
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
					cout << " - turning on extra stats output" << endl;
					setParm("stats_extra", 1);
				} else if (boost::iequals(value, "ctl_r")) {
					cout << " - turning on extra stats output for CTL (chemotaxis v2 only)" << endl;
					setParm("stats_ctl_r", 1);
				}
			} else if (boost::iequals(keyword, "layoutfile")) {
				layoutfilename = value;
			} else if (boost::iequals(keyword, "reporttime")) {
				/* Set both the report writing and stats updating to the same time */
				setParm("report_time",  getTimeInSeconds(value.c_str()));
				setParm("stats_update", getParm("report_time", 300));
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
				// Handle the commands (all start with '!', e.g. !PLACE or !ADD)
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
			} else if (boost::iequals(keyword, "Event")) {
				handleEvents( pname, pval );
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

				} else if ( boost::iequals(pname,"LYSIS") ) {
					// This determines when a E-I conjugate lyses the cells: either at the beginning of the handling time, or at the end.
					if ( boost::iequals(keyword,"CTL") ) {
						if ( boost::iequals(pval,"start") ) {
							setParm("ctl_immediatekill",1);
						} else if ( boost::iequals(pval,"end") ) {
							setParm("ctl_immediatekill",0);
						}
					}

				} else if ( boost::iequals(pname,"CHEMOTAXIS_PC") ) {
					// If any chemotaxis percentage over 0, then later we set ctl_chemotaxis.
					setParm( string("ctl_chemotaxis_pc"), pval );
				}  else if ( boost::iequals(pname,"CHEMOTAXIS_V") ) {
					// Default is chemotaxis version 0 (i.e. no chemot)
					setParm( string("ctl_chemotaxis_version"), pval );
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

	// Finished looping through the setup file. Do some cleanup now.

	// Chemotaxis cleanup: set ctl_chemotaxis if v1&pc>0 or v2
	if ( (getParm("ctl_chemotaxis_version",0)==1 && getParm("ctl_chemotaxis_pc",0.0f)> 0.0) || getParm("ctl_chemotaxis_version",0)==2 ) {
		setParm("ctl_chemotaxis",1);
	} else { setParm("ctl_chemotaxis",0); }

	// Finished with setup, so do stats
	world->updateStatsFull();

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
		setParm("update_stats", 1); /* Reset stats to be updated every second of worldtime */
		cout << " - EndCondition of 'Infected == "<< getParm("end_infectedpc_eqvalue", 0.0f)*100 <<"%' set" << endl;
	} else if ( boost::iequals(key,"infectedpc_ge") ) {
		setParm("end_infectedpc_ge",1);
		setParm("end_infectedpc_gevalue", (float)atof(value.c_str()) );
		setParm("update_stats", 1); /* Reset stats to be updated every second of worldtime */
		cout << " - EndCondition of 'Infected >= "<< getParm("end_infectedpc_gevalue", 0.0f)*100 <<"%' set" << endl;
	} else {
		cout << "WARNING: Could not parse the EndCondition: " << key <<":"<<value << endl;
	}

}

void ImmunebotsSetup::handleCommands(std::string command, std::string value) {

	if (boost::iequals(command, "!PLACE")) {
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

// Handler for "Event AddAbsolute 50CTL@5d" config lines
void ImmunebotsSetup::handleEvents( std::string type, std::string value ) {

	Event e;
	// Standard inits:
	e.fired = false;

	/* TYPE: ADD E:T RATIO, ADD ABSOLUTE, ADD WHEN, END AFTER <CONDITION> */

	if ( boost::iequals(type, "addetratio") || boost::iequals(type, "etratio") ) {
		// This is an event which will trigger in the future. E:T CTL will be added at time t
		e.event = World::EVENTSLIST_ADDETRATIO;
		//world->addEvent(World::EVENTSLIST_ADDETRATIO, value);
	} else if ( boost::iequals(type, "addabsolute") || boost::iequals(type, "add") ) {
		// This is an event which will trigger in the future. X CTL will be added at time t
		e.event = World::EVENTSLIST_ADDABSOLUTE;
		//world->addEvent(World::EVENTSLIST_ADDABSOLUTE, value);
	} else if ( boost::iequals(type, "addwhen") ) {
		// specialised add: Event AddWhen 10CTL@100Infected => add 10 CTL when Infected count is 100.
		e.event = World::EVENTSLIST_ADDWHEN;
		setParm("update_stats", 1); /* Reset stats to be updated every second of worldtime */
	} else if ( boost::iequals(type, "endafter") ) {
		// specialised add: Event AddWhen 10CTL@100Infected => add 10 CTL when Infected count is 100.
		e.event = World::EVENTSLIST_ENDAFTER;
		setParm("update_stats", 1); /* Reset stats to be updated every second of worldtime */
	} else if ( boost::iequals(type, "endtime") ) {
		// specialised end time: resets the world endtime to worldtime+specifiedtime upon firing.
		e.event = World::EVENTSLIST_ENDTIME;
		setParm("update_stats", 1); /* Reset stats to be updated every second of worldtime */
	}

	/* VALUE: Can be 5CTL@100s or 10CTL@100Infected, i.e. <Number of Agent to add>@<Trigger>
	 *  	or, for EndAfter: 10Infected@100Infected, i.e. Exit when we hit 10 Infected AFTER hitting 100 Infected.
	 *      or, for EndAfter: 13d@100Infected, i.e. Exit 3 days AFTER hitting 100 Infected.
	 * */

	// Start decoding the value string, syntax is "5CTL@8d" =~ /([\d.]+)([^@]+)@(.+)/i
	// Or: (in the case of EndTime) "3d@100I" =~ /([^@]+)@(.+)/i
	boost::regex eventstr("\\s*([\\d.]+)([^@]+)@(.+)(\\s*#.*)?", boost::regex::perl|boost::regex::icase);
	boost::cmatch em;

	if ( boost::regex_match(value.c_str(), em, eventstr) ) {

		if (e.event != World::EVENTSLIST_ENDTIME) {
			// Number is easy - just turn string into char[] and then convert to float.
			e.number  = atof( string(em[1].first, em[1].second).c_str() );

			// Agent is harder - need to match to an AbstractAgent::AGENT_*. Skip for ENDTIME
			e.agentpp = string(em[2].first, em[2].second);
			e.agent   = Agent2Type( e.agentpp );
		} else if (e.event == World::EVENTSLIST_ENDTIME) {
			// This bit picks out the time to end the simulation at, and stores it in e.number
			e.number = getTimeInSeconds( string(em[1].first, em[1].second).append(string(em[2].first, em[2].second)).c_str() );
		}

		// Now pick out the trigger, e.g. 1000I or 5d.
		string trigger = string(em[3].first, em[3].second);
		// After the '@' is either a TIME or an Infected Cell Count, e.g. '@5d' or '@100Infected'
		if ( e.event == World::EVENTSLIST_ADDETRATIO || e.event == World::EVENTSLIST_ADDABSOLUTE ) {
			// Time is OK - need to turn a string into a time (in seconds) using getTimeInSeconds().
			e.time = getTimeInSeconds( trigger.c_str() );
		} else if (e.event == World::EVENTSLIST_ADDWHEN  ||
				   e.event == World::EVENTSLIST_ENDAFTER ||
				   e.event == World::EVENTSLIST_ENDTIME ) {
			// Not an explicit time, but rather a condition which triggers the event (usually adding x CTL once the Infected cell count reaches y).
			boost::regex condstr("(\\d+)(\\w+)", boost::regex::perl|boost::regex::icase);
			boost::cmatch cm;
			if ( boost::regex_match(trigger.c_str(), cm, condstr) ) {
				e.condition_number = floor( atof( string(cm[1].first, cm[1].second).c_str() ) );
				e.condition_type   = Agent2Type(  string(cm[2].first, cm[2].second) );
				e.time             = 0;
				if (e.condition_type == 0) {
					// Unable to parse the trigger!
					cout << "Unable to parse the trigger part of 'Event  "<< type << " " << value << "'. Invalidating this event." << endl;
					e.fired = true;
					e.event = 0;
				}
			}
		} else {
			// Could not decipher '@' part of the string.. error.
			cout << "ERROR Could not match the 'time' part of the event string: '"<< value <<"'. Returning prematurely."<<endl;
			return;
		}


	} else {
		cout << "ERROR Could not match Event string: '"<< value <<"'. Returning prematurely."<<endl;
		return;
	}

	world->addEvent(e);
}

// Translate pretty string to agent type, e.g "CTL" to 4
int ImmunebotsSetup::Agent2Type( string agentpp ) {
	if ( boost::iequals(agentpp, "CTL") )              		{ return(AbstractAgent::AGENT_CTL);                }
	else if ( boost::iequals(agentpp, "Susceptible") ) 		{ return(AbstractAgent::AGENT_SUSCEPTIBLE);        }
	else if ( boost::iequals(agentpp, "Virion") )      		{ return(AbstractAgent::AGENT_VIRION);             }
	else if ( boost::iequals(agentpp, "Infected") )    		{ return(AbstractAgent::AGENT_INFECTED);           }
	else if ( boost::iequals(agentpp, "InfectedNoVirions") ){ return(AbstractAgent::AGENT_INFECTED_NOVIRIONS); }
	else {
		cout << "[WARNING] Unable to parse agent (" << agentpp << ") in config file. CTL, Susceptible, Virion and Infected[NoVirions] are the only valid options." << endl;
		return(0);
	}
}

// Translate pretty agent type to string, e.g 4 to "CTL"
string ImmunebotsSetup::Type2Agent( int agentnum ) {
	string agentpp = "";
	switch (agentnum) {
		case AbstractAgent::AGENT_CTL: 			agentpp = "CTL"; break;
		case AbstractAgent::AGENT_SUSCEPTIBLE: 	agentpp = "Susceptible"; break;
		case AbstractAgent::AGENT_VIRION: 		agentpp = "Virion"; break;
		case AbstractAgent::AGENT_INFECTED: 	agentpp = "Infected"; break;
		case AbstractAgent::AGENT_INFECTED_NOVIRIONS: agentpp = "InfectedNoVirions"; break;
		default: cout << "[WARNING] Unable to parse agent number (" << agentnum << ") to anything meaningful!" << endl;
	}
	return(agentpp);
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
		} else if ( getParm("end_infected_le",0)==1 &&
			world->stats->infected <= getParm("end_infected_levalue",2) ) {
			cout << " - End condition (Infected <= "<<getParm("end_infected_levalue",2)<<") has been reached" << endl;
			return(true);
		}
	}

	return(false);
}


// Will create a (square) grid of non-susceptible and susceptible cells.
void ImmunebotsSetup::placeNonsusceptibleCells() {

	int startx       = getParm("startx",100);
	int starty       = getParm("starty",100);
	int cell_spacing = getParm("nc_cellspacing", 5);
	int total_cells  = getParm("nc_totalcount", 10000);

	// Check we can place this number of cells! Conservative check.
	int cells_per_row      = floor(sqrt(total_cells)) + 1;
	int max_cells_per_row  = ( conf::DefaultWidth - 20 ) / (2*conf::BOTRADIUS + cell_spacing);
	if ( cells_per_row > max_cells_per_row ) {
		cout << "ERROR: Too many cells are being placed. Max number of cells is: " << pow(max_cells_per_row,2) << endl;
		cout << "Quitting program.." << endl;
		exit(EXIT_SUCCESS);
	}

	if ( getParm("nc_squaregrid", 0) == 1 ) {
		int squaroot = (int)(sqrt(total_cells));
		int clength = (int)(2*conf::BOTRADIUS) + cell_spacing;

		// Check that the total number of cells to place does not exceed the world size
		if ( startx+clength*squaroot > getParm("WIDTH",0) ) {
			// World not wide OR high enough!
			int MAX_CELLS = (getParm("WIDTH",0)-2*startx)/clength;
			MAX_CELLS = MAX_CELLS * MAX_CELLS;
			cout << "ERROR: Config file specifies " << total_cells << " cells to be placed. However, system max is: " << MAX_CELLS << ". Exiting.\n";
			exit(EXIT_FAILURE);
		}

		for (int i=0; i<squaroot; i++) {
			for (int j=0; j<squaroot; j++) {
				world->addCell(
						startx+clength*i,
						starty+clength*j
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
