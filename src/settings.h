#ifndef SETTINGS_H
#define SETTINGS_H

namespace conf {

	// Absolute maximum size of the world!
	// Note: most of the time we'll be using a sub-set of this (see World::bounding_m{in,ax})
    const int WIDTH   = 10000;
    const int HEIGHT  = 10000;
    const int WWIDTH  = 1800;  // window width and height
    const int WHEIGHT = 1000;

    const float FAR_PLANE = 10.0; // The extent of the z-axis

    /* Set the (initial) colour of the different cells */
    const float COLOUR_NOT_SUSCEPTIBLE[4]  = {1,1,1,1}; // white
    const float COLOUR_SUSCEPTIBLE[4]      = {1,1,0,1}; // yellow
    const float COLOUR_INFECTED[4]		   = {0,1,0,1}; // green
    const float COLOUR_CTL[4]			   = {0,0,1,1}; // blue
    const float COLOUR_DEAD[4]			   = {0.2,0.2,0.2,1}; // darkgray (black?)

	/* New configuration variables */
    const float SUSCEPTIBLE_PERCENTAGE    = 2.0f;
    const int DEFAULT_NUM_CELLS_TO_ADD    = 1000;

    /* Simulation-related variables */
    /* Number of timesteps to run based on the slowest rate
     *   i.e. if slowest rate is 6/min (i.e. 1/10 per second) and #TS is 10, then every timestep will be 1s.
	 *   Formula: TS time = 1/(rate * #TS)
     */
    const int NUMBER_OF_TIMESTEPS = 10;

    /* SCRIPTBOTS legacy, but used in Immunebots */
    const int NUMBOTS=5; //initially, and minimally
    const float BOTRADIUS=5; //for drawing

    /* EVERYTHING BELOW HERE IS FROM SCRIPTBOTS (DEPRECATED) */
    const int CZ = 50; //cell size in pixels, for food squares. Should divide well into Width Height

    const float BOTSPEED= 0.1;
    const float SPIKESPEED= 0.005; //how quickly can attack spike go up?
    const float SPIKEMULT= 0.5; //essentially the strength of every spike impact
    const int BABIES=2; //number of babies per agent when they reproduce
    const float BOOSTSIZEMULT=2; //how much boost do agents get? when boost neuron is on
    const float REPRATEH=7; //reproduction rate for herbivours
    const float REPRATEC=7; //reproduction rate for carnivours

    const float DIST= 150;		//how far can the eyes see on each bot?
    const float EYE_SENSITIVITY= 2; //how sensitive are the eyes?
    const float BLOOD_SENSITIVITY= 2; //how sensitive are blood sensors?
    const float METAMUTRATE1= 0.002; //what is the change in MUTRATE1 and 2 on reproduction? lol
    const float METAMUTRATE2= 0.05;

    const float FOODGROWTH= 0;//0.00001; //how quickly does food grow on a square?
    const float FOODINTAKE= 0.00325; //how much does every agent consume?
    const float FOODWASTE= 0.001; //how much food disapears if agent eats?
    const float FOODMAX= 0.5; //how much food per cell can there be at max?
    const int FOODADDFREQ= 30; //how often does random square get to full food?

    const float FOODTRANSFER= 0.001; //how much is transfered between two agents trading food? per iteration
    const float FOOD_SHARING_DISTANCE= 50; //how far away is food shared between bots?


    const float FOOD_DISTRIBUTION_RADIUS=100; //when bot is killed, how far is its body distributed?
}

#endif
