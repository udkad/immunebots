#ifndef SETTINGS_H
#define SETTINGS_H

namespace conf {

    const int WIDTH = 1800;  //width and height of simulation
    const int HEIGHT = 1000;
    const int WWIDTH = 1800;  //window width and height
    const int WHEIGHT = 1000;

    /* Set the (initial) colour of the different cells */
    const float COLOUR_NOT_SUSCEPTIBLE[3]  = {1,1,1}; // white
    const float COLOUR_SUSCEPTIBLE[3]      = {1,1,0}; // yellow
    const float COLOUR_INFECTED[3]		   = {0,1,0}; // green
    const float COLOUR_CTL[3]			   = {0,0,1}; // blue
    const float COLOUR_DEAD[3]			   = {0.2,0.2,0.2}; // darkgray

	/* New configuration variables */
    const float SUSCEPTIBLE_PERCENTAGE    = 2.0f;
    const int DEFAULT_NUM_CELLS_TO_ADD    = 1000;

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
