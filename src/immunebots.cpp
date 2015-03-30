//============================================================================
// Name        : immunebots.cpp
// Author      : Ulrich Kadolsky
// Version     :
// Copyright   : GPL
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <GL/glew.h>
#include <GL/glut.h>

#include "GLView.h"
#include "ImmunebotsSetup.h"
#include "World.h"
#include "config.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h> // for getopt
#include <AntTweakBar.h>
#include <ctime>
#include <stdio.h>

using namespace std;

GLView* GLVIEW;
void processArguments(int argc, char **argv, ImmunebotsSetup *is);
void printHelp();

int main(int argc, char **argv) {

    srand(time(0));
    ImmunebotsSetup *is = new ImmunebotsSetup();

    // Sort out the command line arguments
    processArguments(argc, argv, is);
    World* world = new World(is);
    GLVIEW = new GLView(world, is);
    if (is->automaticRun) {
    	// Want to start the simulation immediately..
    	if (is->useGlut) GLVIEW->switchToSimulationMode(true);
    	GLVIEW->setPaused(false);
    }
    cout << "Finished setting up config, the world and GLView\n";

    cout << "Display the help, regardless of whether -h was set or not\n";
    printHelp();

    // Check if we're running in X (useGlut) or command-line mode (!useGlut)
    if (!is->useGlut) {
		// Set drawing to false (although this is no longer necessary, as we check is->useGlut before trying to draw)
		GLVIEW->toggleDrawing();
    	// Just repeatedly call handleIdle();
    	while (1) {
    		GLVIEW->handleIdle();
    		// Will abort if no more agents or world end time is reached
    	}
    } else {
    	// Print out keys - only relevant if we have visualisation
    	printf("Setup Menu:\n r=reset view, Esc=quit\n");
        printf("Simulation menu:\n p=pause, d=drawing on/off (for faster computation), +=faster, -=slower, -->=fast forward to next day\n");
    }

    //GLUT SETUP
    glutInit(&argc, argv); // GLUT will auto-recognise some arguments
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // GLUT_ALPHA?
    glutInitWindowPosition(30,30);
    glutInitWindowSize(conf::WWIDTH,conf::WHEIGHT);
    glutCreateWindow("Immunebots");
    glewInit();

    // Initialise and create the Tw menu (only display the setup menu at the start)
	TwInit(TW_OPENGL, NULL);
	TwWindowSize(conf::WWIDTH,conf::WHEIGHT);
    GLVIEW->createSetupMenu(true);	// Create menu now (creates as visible)
    GLVIEW->createSimulationMenu(false); // Create menu now, but hide it until required.

    // Continue with rest of the GL initialisation
    //glClearColor(0.1,0.9,1.0,1.0); //cyan
    glClearColor(1.0,1.0,1.0,1.0); // white
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
    return 0;
}

// Parses the command line options using getopt and updates the ImmunebotsSetup
void processArguments(int argc, char **argv, ImmunebotsSetup *is) {
	int o;
	string runid; // If there is a runid specified on the command line, then ensure that it overwrites the one in the config file
	cout << "Processing arguments: " << argv << endl;
	while ((o = getopt(argc, argv, "ade:?hs:")) != -1) {
	        switch (o) {
	        case 'a':	// Automatic run
	        	cout << "Automatically start simulation and end program when complete.\n";
	        	is->automaticRun=true;
	        	break;
	        case 'd':	// Disable draw
	        	cout << "Disabling GLUT, autostart simulation and autoexit on completion.\n";
	        	is->useGlut=false;
	        	is->automaticRun=true;
	        	break;
	        case 'e':	// End time of the simulation
	        	is->setEndTime(optarg); break;
	        case '?':	// Intentional fallthrough (Help)
	        case 'h':	// Display help
	        	printHelp(); abort();
	        case 'i':	// Commandline RunID (override)
	        	runid = optarg; break;
	        case 's':	// Load setup file
	        	// Note: First check setup file exists
	        	ifstream setupfn(optarg);
	        	if (!setupfn.good()) {
	        		cout << "WARNING: Could not find the setup file ('"<<optarg<<"')\n";
	        	} else {
	        		cout << "Setting setupfile as: '"<<optarg<<"'\n";
		        	is->setupfilename = string(optarg);
		        	is->processSetupFile();
	        	}
	        	break;
			}
	}
	// If a run id was specified on the command line, ensure it overwrites the one in the config file
    if ( !runid.empty() ) is->id = runid;

    cout << ".. done processing arguments.\n";
}

void printHelp() {
	cout << "Immunebots Version 1.0001\n";
	cout << "	-a\t\tAutomate simulation start and exit program when done\n";
	cout << "   -d\t\tTurn draw off (implies -a, require -s)\n";
	cout << "   -e <time>\tRun simulation until <time>, e.g. '84600s' or '7d'\n";
	cout << "   -h\t\tDisplay help (this screen)\n";
	cout << "   -i <run id>\tSet run ID (overrides config file run ID)\n";
	cout << "   -s <config>\tLoad the specified settings file\n";
}
