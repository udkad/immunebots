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
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);// GLUT_DEPTH // GLUT_ALPHA?
    glutInitWindowPosition(30,30);
    glutInitWindowSize(conf::WWIDTH,conf::WHEIGHT);
    glutCreateWindow("Immunebots");
    glewInit();

    // Initialise and create the Tw menu (only display the setup menu at the start)
	TwInit(TW_OPENGL, NULL);
	TwWindowSize(conf::WWIDTH,conf::WHEIGHT);
    GLVIEW->createSetupMenu(true);
    GLVIEW->createSimulationMenu(false);

    // Continue with rest of the GL initialisation
    glClearColor(0.9f, 0.9f, 1.0f, 0.0f);
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
	cout << "Processing arguments: " << **argv << endl;
	while ((o = getopt(argc, argv, "ade:?hs:")) != -1) {
	        switch (o) {
	        case 'a': 	break;
	        case 'd':
	        	cout << "Disabling GLUT\n";
	        	is->useGlut=false; break;
	        case 'e':
	        	is->setEndTime(optarg); break;
	        case '?':	// Intentional fallthrough
	        case 'h': 	printHelp(); abort();
	        case 'i':	runid = optarg; break;
	        case 's':
	        	// Check setup file exists
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
	cout << "Immunebots Version 1.0\n";
	cout << "   -c <config>\tLoad the specified config file\n";
	cout << "   -d\t\tTurn draw off\n";
	cout << "   -e <time>\tRun simulation until <time>, e.g. '84600s' or '7d'\n";
	cout << "   -h\t\tDisplay help (this screen)\n";
	cout << "   -s\t\tUNUSED Automatically start the simulation (must be used with -c)\n";
}
