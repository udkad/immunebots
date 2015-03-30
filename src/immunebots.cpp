//============================================================================
// Name        : immunebots.cpp
// Author      : Ulrich Kadolsky
// Version     :
// Copyright   : GPL
// Description : Hello World in C++, Ansi-style
//============================================================================

/* These are the GL-related libraries */
#ifndef IMMUNEBOTS_NOGL
#include <GL/glew.h>
#include <GL/glut.h>
#include <AntTweakBar.h>
#endif

#include "GLView.h"
#include "ImmunebotsSetup.h"
#include "World.h"
#include "config.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h> // for getopt
#include <ctime>
#include <stdio.h>

using namespace std;

GLView* GLVIEW;
void processArguments(int argc, char **argv, ImmunebotsSetup *is);
void printHelp();

int main(int argc, char **argv) {

	float version = 1.00081;

	/*
	 * Version differences in brief:
	 *
	 *  1.0001: First "mature" release.
	 *  1.0002: Support for compiler directives for NO_SERIALISATION and NO_GL (i.e. headless run).
	 *  		Also static linking to the boost libraries (default).
	 *  1.0003: Addition of the EventsList, i.e. adding of CTL at ratio E:T at a future time.
	 *  1.0004: Random seed taken from /dev/urandom [linux only] instead of time(0).
	 *  1.0005: Added new Event ('Event AddWhen 10CTL@100Infected' => "Add 10 CTL When Infected == 100").
	 *  		Rewrote the update stats and write_report code.
	 *  1.0006: Added new Event ('Event EndAfter 10Infected@100Infected' => "End when Infected count reaches 10 from 100").
	 *  		Now quits with error if trying to place more cells than the world defaults permit.
	 *  1.0007: New End Condition ('Event End 3d@1000Infected' => "End 3 days after Infected count reached 1000").
	 *  		CTL now log how much time they spend in each state (move, scan, lyse) if stats_extra is enabled.
	 *   		Minor corrections to the CTL code.
	 *  1.0008: Added in simple chemotaxis (after every persistence length of movement, there is a configurable chance than
	 *            the CTL will move in the direction of the nearest infected cell).
	 *          Reduced internal grid size from 1M cells to 100k cells (RAM from >3GB to 0.45).
	 *  1.00081: Improved run times by using the agents vector inside of the cells vector for finding the current pop of Infected cells.
	 *
	 */

	cout <<"  _                                            _             _        " 		<< endl;
	cout <<" (_) _ __ ___   _ __ ___   _   _  _ __    ___ | |__    ___  | |_  ___ " 		<< endl;
	cout <<" | || '_ ` _ \\ | '_ ` _ \\ | | | || '_ \\  / _ \\| '_ \\  / _ \\ | __|/ __|" 	<< endl;
	cout <<" | || | | | | || | | | | || |_| || | | ||  __/| |_) || (_) || |_ \\__ \\" 		<< endl;
	cout <<" |_||_| |_| |_||_| |_| |_| \\__,_||_| |_| \\___||_.__/  \\___/  \\__||___/" 	<< endl << endl;

	cout << "      Version " << version <<
			", adapted from Andrej Karpathy's Scriptbots" << endl <<
			"        by Ulrich D Kadolsky (ulrich.kadolsky@einstein.yu.edu)" << endl << endl;

#ifdef IMMUNEBOTS_NOGL
	cout << "                      ~ without OpenGL flavour ~" << endl << endl;
#endif
#ifndef IMMUNEBOTS_NOGL
	cout << "                        ~ with OpenGL flavour ~" << endl << endl;
#endif

	// Get a random integer from /dev/urandom (the system's entropy pool).
	// Note: We cannot use time(0) as this program might be run at the same second on the same multi-node cluster machine.
	unsigned int seed;
	ifstream urandom("/dev/urandom");
	urandom.read(reinterpret_cast<char*>(&seed), sizeof(seed));
	urandom.close();
	srand(seed);
	cout << "Random seed (from /dev/urandom): " << seed << endl << endl;

    ImmunebotsSetup *is = new ImmunebotsSetup();
    World *world = new World(is);

    // Let world & setup see each other
    is->setWorld(world);

    // Sort out the command line arguments after creating (an empty) world
    processArguments(argc, argv, is);
    GLVIEW = new GLView(world, is);
    if (is->automaticRun) {
    	// Want to start the simulation immediately..
#ifndef IMMUNEBOTS_NOGL
    	// TODO: Is this still necessary?
    	if (is->useGlut) GLVIEW->switchToSimulationMode(true);
#endif
    	GLVIEW->setPaused(false);
    }

    cout << "Finished setting up config, the world and GLView." << endl << endl;

    // Check if we're running in X (useGlut) or command-line mode (!useGlut)
    if (!is->useGlut) {
		// Set drawing to false (although this is no longer necessary, as we check is->useGlut before trying to draw)
		GLVIEW->toggleDrawing();
		GLVIEW->checkSetup(); // If ok, will set dosimulation to TRUE
		cout << "Starting headless run:" << endl;

		// Just repeatedly call handleIdle()
    	while ( !GLVIEW->paused ) {
    		GLVIEW->handleIdle();
    	}

    	clock_t programtime;
		programtime = clock() / CLOCKS_PER_SEC ;

		cout << "Simulation complete. "<< GLVIEW->idlecalled <<" world ticks took "<<programtime<<"s realtime."<<endl;

		exit(EXIT_SUCCESS);

    } else {
    	// Print out keys - only relevant if we have visualisation
    	printf("Setup Menu:\n  r=reset view, Esc=quit\n");
        printf("Simulation menu:\n  p=pause, d=drawing on/off (for faster computation), +=faster, -=slower, -->=fast forward to next day\n\n");
    }

/* GL-related functions */
#ifndef IMMUNEBOTS_NOGL
    glutInit(&argc, argv); // GLUT will auto-recognise some arguments
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // GLUT_ALPHA?
    glutInitWindowPosition(30,30);
    glutInitWindowSize(conf::WWIDTH,conf::WHEIGHT);
    int mainwindow = glutCreateWindow("Immunebots");
    glewInit();

    // Initialise and create the Tw menu (only display the setup menu at the start)
	TwInit(TW_OPENGL, NULL);
	TwWindowSize(conf::WWIDTH,conf::WHEIGHT);
    GLVIEW->createSetupMenu(true);	// Create menu now (creates as visible)
    GLVIEW->createSimulationMenu(false); // Create menu now, but hide it until required.
    GLVIEW->createStatsMenu(true); // Create menu now, but hide it until required.
    GLVIEW->setupDisplayLists();

    // Continue with rest of the GL initialisation
    //glClearColor(0.1,0.9,1.0,1.0); //cyan
    //glClearColor(1.0,1.0,1.0,1.0); // white
    glClearColor(0,0,0,0.5); // black
    glutDisplayFunc(gl_renderScene);
    glutIdleFunc(gl_handleIdle);
    glutReshapeFunc(gl_changeSize);

    glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
    // send the 'glutGetModifers' function pointer to AntTweakBar
    TwGLUTModifiersFunc(glutGetModifiers);

    glutKeyboardFunc(gl_processNormalKeys);
    glutSpecialFunc(gl_processSpecialKeys);
    glutMouseFunc(gl_processMouse);
    glutMotionFunc(gl_processMouseActiveMotion);
    glutPassiveMotionFunc(gl_processMousePassiveMotion);

    glutMainLoop();

    TwTerminate();
#endif // End of IMMUNEBOTS_NOGL

    return(EXIT_SUCCESS);

}

// Parses the command line options using getopt and updates the ImmunebotsSetup
void processArguments(int argc, char **argv, ImmunebotsSetup *is) {

	int o;

	cout << "Processing arguments: " << endl;

	while ((o = getopt(argc, argv, "ade:?hc:s:i:")) != -1) {
	        switch (o) {
	        case 'a':	// Automatic run
	        	cout << " (a) Automatically start simulation and end program when complete.\n";
	        	is->automaticRun=true;
	        	break;
	        case 'd':	// Disable draw
	        	cout << " (d) Disabling GLUT, autostart simulation and autoexit on completion.\n";
	        	is->useGlut=false;
	        	is->automaticRun=true;
	        	break;
	        case 'e':	// End time of the simulation
	        	is->setEndTime(optarg); break;
	        case '?':	// Intentional fallthrough (Help)
	        case 'h':	// Display help
	        	printHelp(); exit(EXIT_SUCCESS);
	        case 'i':	// Commandline RunID (override)
	        	is->id = optarg;
	        	cout << " (i) Run ID set as '" << optarg << "' (from commandline)" << endl;
	        	break;
	        case 'c':	// Intentional fallthrough to config file
	        case 's':	// Load config file
	        	// Note: First check setup file exists
	        	ifstream setupfn(optarg);
	        	if (!setupfn.good()) {
	        		cout << "WARNING: Could not find the configuration file ('"<<optarg<<"')" << endl;
	        	} else {
	        		cout << " (c) Setting setupfile as: '"<<optarg<<"'\n";
		        	is->setupfilename = string(optarg);
	        	}
	        	break;
			}
	}

	cout << "Finished processing arguments." << endl << endl << "Processing configuration: " << endl;

	// Process the setup file at this point
	is->processSetupFile();

	cout << "Finished processing configuration." << endl << endl;
}

void printHelp() {
	cout << " Displaying command line options:" << endl;
	cout << "   -a\t\tAutomate simulation start and exit program when done" << endl;
	cout << "   -d\t\tTurn draw off (implies -a, requires -c)" << endl;
	cout << "   -e <time>\tRun simulation until <time>, e.g. '84600s' or '7d'" << endl;
	cout << "   -h\t\tDisplay help (this screen)" << endl;
	cout << "   -i <run id>\tSet run ID (overrides config file run ID)" << endl;
	cout << "   -c <config>\tLoad the specified configuration file" << endl;
}
