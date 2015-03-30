#ifndef HELPERS_H
#define HELPERS_H
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <iostream>

using namespace std;

// Statistics struct
typedef struct {   /* Our stats module - will be updated at an appropriate frequency! */
	bool force_refresh;					/* How often to update (in world-seconds) */
	/* Cell types */
	int total_cells;					/* Total number of ALL cells incl. CTL. Dead cells? */
	int notsusceptible, susceptible, infected, dead;
	int ctl;
	int virion;
	/* Area calculations */
	float ctl_per_mm2,  virion_per_mm2;	/* Density of CTL and virions per mm2 */
	float ctl_per_cell, virion_per_cell;/* Density of CTL and virions per cell */
	float ctl_per_infected;				/* E:T ratio */
	float area_mm2;						/* area (in um) of the bounding box SANS margin */
	float cell_area;
	char infected_str[255];
	/* Events */
	int infected_death, infected_lysis;	/* Death by virus-induced cytotoxicity or CTL lysis */
	float infected_death_average, infected_lysis_average;	/* Averaged (over 1 min) */
	int failed_infection;				/* Triggered when a virion goes over a cell but doesn't infect */
	/* Events: Average cells scanned by CTL each minute */
	int scan_complete;
	std::vector<int> scan;
	int lastCalled;
	float scan_average;
	// A 3-element vector of the fraction of time each CTL spent in each state.
	vector<float> ctlsf;
} Statistics;

// Event struct. This is an Event which happens in the future, it is held on the World::EventsList.
typedef struct {    /* Initialised with a setup line such as: "Event AddAbsolute 100CTL@1d" */
	int event;		/* Event type: add E:T ratio (EVENTSLIST_ETRATIO) or absolute number (EVENTSLIST_ADD) */
	float number;	/* Number of Agents to add */
	int agent;  	/* Agent type; uses AbstractAgent::AGENT_* numbering */
	int time; 		/* Time (actually minimum time) when the event triggers. Time of 0 = activated every second (until activated=true). */
	string agentpp; /* Pretty name for the agent, e.g. "CTL" instead of "ctl" or 4 */

	/* Extra variables for the AddWhen condition */
	int condition_number;
	int condition_type;

	/* Extra variables for persistent events */
	bool fired;

	// Don't like this, would rather overload "<<" and call "cout << EventE << endl" instead.
	// Don't know how to do that though.
	// Event KEYWORD <number><agentpp>@ [<time> || <condition_number><condition_type>]
	void prettyprint() {
		switch (event) {
			case 1:
				cout << "Event: Add " << number << " agents of type '" << agentpp << "' when " << "time is "<<time<<"s"; break;
			case 2:
				cout << "Event: Add " << number << " agents of type '" << agentpp << "' when " << "time is "<<time<<"s"; break;
			case 3:
				cout << "Event: Add " << number << " agents of type '" << agentpp << "' when there are " << condition_number << " of type " << condition_type; break;
			case 4:
				cout << "Event: End simulation when agents of type '" << agentpp << "' hits " << number << ", after we hit " <<condition_number<< " of type '"<< condition_type << "'"; break;
			case 5:
				cout << "Event: End simulation in "<< (number/24/60/60) <<" days after we hit "  <<condition_number<< " agents of type '"<< condition_type << "'"; break;
			default:
				cout << "Event: No description. Please fill in a useful sentence for event type '"<< event <<"'"; break;
		}
	}

} Event;

//uniform random in [a,b)
inline float randf(float a, float b){return ((b-a)*((float)rand()/RAND_MAX))+a;}

//uniform random int in [a,b)
inline int randi(int a, int b){return (rand()%(b-a))+a;}

//normalvariate random N(mu, sigma)
inline double randn(double mu, double sigma) {
	static bool deviateAvailable=false;	//	flag
	static float storedDeviate;			//	deviate from previous calculation
	double polar, rsquared, var1, var2;
	if (!deviateAvailable) {
		do {
			var1=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
			var2=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
			rsquared=var1*var1+var2*var2;
		} while ( rsquared>=1.0 || rsquared == 0.0);
		polar=sqrt(-2.0*log(rsquared)/rsquared);
		storedDeviate=var1*polar;
		deviateAvailable=true;
		return var2*polar*sigma + mu;
	}
	else {
		deviateAvailable=false;
		return storedDeviate*sigma + mu;
	}
}

//cap value between 0 and 1
inline float cap(float a){
	if (a<0) return 0;
	if (a>1) return 1;
	return a;
}
#endif
