#ifndef HELPERS_H
#define HELPERS_H
#include <stdlib.h>
#include <math.h>
#include <vector>

using namespace std;

typedef struct {   /* Our stats module - gets updated every second */
	int total_cells;					/* Total number of ALL cells incl. CTL  */
	int notsusceptible, susceptible, infected, dead;   		/* Cell types */
	int ctl;							/* CTL numbers */
	int virion;
	float ctl_per_mm2,  virion_per_mm2;	 /* Density of CTL and virions per mm2 */
	float ctl_per_cell, virion_per_cell; /* Density of CTL and virions per cell */
	float ctl_per_infected;				 /* E:T ratio */
	float area_mm2;					/* area (in um) of the bounding box SANS margin */
	float cell_area;
	char infected_str[255];
	/* Events */
	int infected_death, infected_lysis;	/* Death by virus-induced cytotoxicity or CTL lysis */
	int failed_infection;	/* Triggered when a virion goes over a cell but doesn't infect */
	/* Events: Average cells scanned by CTL each minute */
	int scan_complete;
	std::vector<int> scan;
	int lastCalled;
	float scan_average;
} Statistics;

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
