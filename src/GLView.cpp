
#include <AntTweakBar.h>

#include <ctime>
#include <stdio.h>
#include <string>

#include <GL/glut.h>

#include "settings.h"
#include "vmath.h"
#include "GLView.h"
#include "ImmunebotsSetup.h"
#include "config.h"
#include "World.h"
#include "AbstractAgent.h"
#include "Cell.h"
#include "CTL.h"

/*Test libraries*/
#include <fstream>
#include <iostream>

using namespace std;

void gl_processNormalKeys(unsigned char key, int x, int y) {
    GLVIEW->processNormalKeys(key, x, y);
}

void gl_processSpecialKeys(int key, int x, int y) {
    GLVIEW->processSpecialKeys(key, x, y);
}

void gl_changeSize(int w, int h) {
    GLVIEW->changeSize(w,h);
}

void gl_handleIdle() {
    GLVIEW->handleIdle();
}

void gl_processMouse(int button, int state, int x, int y) {
    GLVIEW->processMouse(button, state, x, y);
}

void gl_processMouseActiveMotion(int x, int y) {
    GLVIEW->processMouseActiveMotion(x,y);
}

void gl_processMousePassiveMotion(int x, int y) {
    GLVIEW->processMousePassiveMotion(x,y);
}

void gl_renderScene() {
    GLVIEW->renderScene();
}


void GLView::RenderString(float x, float y, void *font, const char* string, float r, float g, float b) {
    glColor3f(r,g,b);
    glRasterPos2f(x, y);
    int len = (int) strlen(string);
    for (int i = 0; i < len; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
}

void GLView::drawCircle(float x, float y, float r) {
    float n;
    for (int k=0;k<17;k++) {
        n = k*(M_PI/8);
        glVertex3f(x+r*sin(n),y+r*cos(n),0);
    }
}

// TW Callback function to reset the world
void TW_CALL ResetWorld( void * world ) {
	// This will reset the world by clearing the patch layer, the cells and the CTL.
	// Should also reset the time.
	printf("DEBUG: Resetting World\n");
	//((World *)world)->reset(); // Crashes program!! Try again, should work now
}

void TW_CALL AddCells( void * world ) {
	// Passing through 0 will use the world::DEFAULT_NUM_CELLS_TO_ADD instead
	((World *)world)->addRandomCells(0);
}

void TW_CALL ResetSusceptibleCells( void * world ) {
	((World*)world)->resetCellSusceptibility();
}

void TW_CALL ClearAllCells( void * world ) {
	((World*)world)->clearAllCells();
}

void TW_CALL PlaceCellsOnPatches( void * world ) {
	((World*)world)->placeCells();
}

void TW_CALL SaveLayout( void * world ) {
	((World*)world)->saveLayout();
}

void TW_CALL LoadLayout( void * world ) {
	((World*)world)->loadLayout();
}

void TW_CALL SwitchToSimulationMode( void * w ) {
	// When we call this we to the following:
	// 1. hide the setup bar
	// 2. show the simulation bar
	// 3. Change the world state
	// Note: Should we save the setup?
	TwDefine(" Setup visible=false ");
	TwDefine(" Simulation visible=true ");
	GLVIEW->switchToSimulationMode(true);
}


void TW_CALL SwitchToSetupMode( void * w ) {

	TwDefine(" Simulation visible=false ");
	TwDefine(" Setup visible=true ");
	GLVIEW->switchToSimulationMode(false);
}


void TW_CALL PauseContinueSimulation(void *) {
	GLVIEW->togglePaused();
}

/*
 * MOUSE TW_CALLS - check if still relevant *
 */


// This is the function which is called by the "helper" mousestate function to get the mousestate for the GUI
void TW_CALL GetMouseState(void * value, GLView * view, int ThisMouseState) {
	// Write true/false in the supplied value if view->getMouseState == ThisMouseState
	*(bool *)value = (view->getMouseState() == ThisMouseState);
}

void TW_CALL GetMouseStateDrawPatch(void *value, void *v) {
	// This just casts the void pointers (if required) and sets the right MouseState to compare to.
	GetMouseState(value, (GLView *)v, MOUSESTATE_DRAW_PATCH);
}

void TW_CALL GetMouseStatePlaceCell(void *value, void *v) {
	// This just casts the void pointers (if required) and sets the right MouseState to compare to.
	GetMouseState(value, (GLView *)v, MOUSESTATE_PLACE_CELL);
}

void TW_CALL GetMouseStateInfectCell(void *value, void *v) {
	// This just casts the void pointers (if required) and sets the right MouseState to compare to.
	GetMouseState(value, (GLView *)v, MOUSESTATE_INFECT_CELL);
}

// Function to set the new mouseState when the user toggles something
void TW_CALL SetMouseState( bool SwitchedOn, GLView *view, int ms ) {
	if ( SwitchedOn ) {
		view->setMouseState(ms);
	} else {
		view->setMouseState( MOUSESTATE_DEFAULT );
	}
}

void TW_CALL SetMouseStateDrawPatch(const void *v, void *view) {
	// Cast bool and view, adding the required mouseState if v
	SetMouseState( *(const bool *)v, (GLView *)(view), MOUSESTATE_DRAW_PATCH);
}
void TW_CALL SetMouseStatePlaceCell(const void *v, void *view) {
	// Cast bool and view, adding the required mouseState if v
	SetMouseState( *(const bool *)v, (GLView *)(view), MOUSESTATE_PLACE_CELL);
}
void TW_CALL SetMouseStateInfectCell(const void *v, void *view) {
	// Cast bool and view, adding the required mouseState if v
	SetMouseState( *(const bool *)v, (GLView *)(view), MOUSESTATE_INFECT_CELL);
}

void TW_CALL CopyStdStringToClient(std::string& destinationClientString, const std::string& sourceLibraryString) {
  // Copy the content of souceString handled by the AntTweakBar library to destinationClientString handled by your application
  destinationClientString = sourceLibraryString;
}


void GLView::createSetupMenu(bool visible) {

	TwBar* bar = TwNewBar("Setup");

	TwDefine("Setup color='0 0 0' alpha=128 position='10 10' size='240 550'");
	TwDefine("Setup fontresizable=false resizable=true");

	TwAddVarRO(bar, "MouseState", TW_TYPE_INT32, (&mouseState), " label='Mouse state' " );

	TwAddSeparator(bar, NULL, NULL);

	TwAddVarCB(bar, "dp_switch", TW_TYPE_BOOLCPP, SetMouseStateDrawPatch, GetMouseStateDrawPatch, this,
    		" label='Draw patches' group='Patch Setup' help='Toggle to switch on patch draw mode.' ");
	TwAddVarRW(bar, "sp_roto", TW_TYPE_FLOAT, &((*world).SUSCEPTIBLE_PERCENTAGE), " label='Susceptible cells (%)' min=0 max=100 group='Cells Setup' step=0.1 ' ");
	TwAddButton(bar, "sp_button", ResetSusceptibleCells, world, " label='Reset cell susceptibility' group='Cells Setup' ");
	TwAddVarRW(bar, "nc_roto", TW_TYPE_INT32, &((*world).DEFAULT_NUM_CELLS_TO_ADD), " label='Number of cells' min=100 max=10000 group='Cells Setup' step=50 ' ");
	TwAddButton(bar, "cd_button", AddCells, world, " label='   Add cells' group='Cells Setup' ");
    // Callback to switch on/off the cell dropper
	TwAddVarCB(bar, "pc_switch", TW_TYPE_BOOLCPP, SetMouseStatePlaceCell, GetMouseStatePlaceCell, this,
    		" label='Manually place cells' group='Cells Setup' help='Toggle to switch on cell dropping mode.' ");
	TwAddButton(bar, "clear_cells", ClearAllCells, world, " label='Clear all cells' group='Cells Setup' ");

	// Allow user to change the cell colours (Cell Setup/Cell colour, starts off closed)
	TwAddVarRW(bar, "ctn_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_NOT_SUSCEPTIBLE), 	" label='Not susceptible' group='Cell colour' ");
	TwAddVarRW(bar, "cts_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_SUSCEPTIBLE), 		" label='Susceptible' group='Cell colour' ");
	TwAddVarRW(bar, "cti_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_INFECTED), 		" label='Infected' group='Cell colour' ");
	TwAddVarRW(bar, "ctc_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_CTL),				" label='CTL' group='Cell colour' ");
	TwAddVarRW(bar, "ctd_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_DEAD),				" label='Dead' group='Cell colour' ");
	TwDefine(" Setup/'Cell colour' group='Cells Setup' opened=false ");  // group Color is moved into group Cells Setup

	// Callback to the manual cell infector on/off switch
	TwAddVarCB(bar, "ic_switch", TW_TYPE_BOOLCPP, SetMouseStateInfectCell, GetMouseStateInfectCell, this,
    		" label='Manually infect cell' group='Cells Setup' help='Toggle to switch on infect cell mode.' ");

	TwAddSeparator(bar, NULL, NULL);

	TwAddButton(bar, "toggle_label", NULL, NULL, "label='Display options:'");

	TwAddSeparator(bar, NULL, NULL);

	TwAddButton(bar, "placecells_toggle", PlaceCellsOnPatches, world, " label='Place cells on patches' ");

	TwAddSeparator(bar, NULL, NULL);

	TwCopyStdStringToClientFunc(CopyStdStringToClient); /* Bit of magic to copy user-defined strings to the program */
	TwAddVarRW(bar, "filename_str", TW_TYPE_STDSTRING, &((*ibs).layoutfilename), "label='Setup filename' group='Setup load/save' ");
	TwAddButton(bar, "save_Setup", SaveLayout, world, " label='Save Setup' group='Setup load/save' ");
	TwAddButton(bar, "load_Setup", LoadLayout, world, " label='Load Setup' group='Setup load/save' ");

	TwAddSeparator(bar, NULL, NULL);

	TwAddButton(bar, "todo", NULL, NULL, "label='To do:'");
	TwAddButton(bar, "iws_button", ResetWorld, world, " label='  Reset World'");
	TwAddButton(bar, "pl_toggle", NULL, NULL, "label='  Toggle patch layer'");
	TwAddButton(bar, "reset_colour_button", NULL, NULL, " label='  Reset cell colour' ");

	TwAddSeparator(bar, NULL, NULL);

	TwAddButton(bar, "switchToSimulation_button", SwitchToSimulationMode, world, " label='Switch To Simulation Mode' ");

	// And hide if required
	if (!visible) {
		TwDefine("'Setup' visible=false");
	}

}

void GLView::createSimulationMenu(bool visible) {

	TwBar* barsm = TwNewBar("Simulation");

	TwDefine("'Simulation' color='0 0 0' alpha=128 position='15 15' size='240 150'");
	TwDefine("'Simulation' fontresizable=false resizable=true");

	TwAddVarRO(barsm, "simstate", TW_TYPE_BOOLCPP, (&paused), " label='Is simulation paused?' " );

	TwAddSeparator(barsm, NULL, NULL);

	TwAddButton(barsm, "pause_toggle", PauseContinueSimulation, NULL, " label='Start/Pause simulation' ");

	TwAddSeparator(barsm, NULL, NULL);

	TwAddButton(barsm, "switchToSetup_button", SwitchToSetupMode, world, " label='Return to Setup Mode' ");

	// And hide if required
	if (!visible) {
		TwDefine("'Simulation' visible=false");
	}

}

GLView::GLView(World *w, ImmunebotsSetup *is):
        dosimulation(false),
        paused(true),
        draw(true),
        skipdraw(1),
        modcounter(0),
        frames(0),
        idlecalled(0),
        lastUpdate(0),
        mouseState(MOUSESTATE_DEFAULT),
    	lastmousey(0),
    	lastmousex(0),
    	mousedownnomove(false),
    	xoffset(0),
    	yoffset(0),
    	scale(1.0),
		processsetupevents(false),
		fastforwardtime(0)
{
	world = w;
	ibs   = is;
}

GLView::~GLView() {

}

// Switch over to Simulation mode if dosim (always pause)
void GLView::switchToSimulationMode(bool dosim) {
	dosimulation = dosim;
	paused = true;
}

// Stupid accessor function for a Tw callback to get the current value of the mouse state
int GLView::getMouseState() {
	return( mouseState );
}

// Stupid accessor function for a Tw callback to get the current value of the mouse state
void GLView::setMouseState(int v) {
	mouseState = v;
}

void GLView::changeSize(int w, int h) {
    // Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,w,h,0,0,1);

    //TwWindowSize(w, h);

}

// class UnderMousePredicate {
//     UnderMousePredicate(int x, int y) : X(x), Y(y) {}
//
//     bool operator()(Agent a) {
//         float mind=1e10;
//         float d;
//         d= pow(X-a.pos.x,2)+pow(Y-a.pos.y,2);
//         if (d<mind) {
//             mind=d;
//             return true;
//         }
//     }
//     int X;
//     int Y;
// };

// Catch mouse clicks
void GLView::processMouse(int button, int state, int x, int y) {

	if( !TwEventMouseButtonGLUT(button, state, x, y) ) { // event has not been handled by AntTweakBar

		// Left click action depends on the current mouseState

		// LEFT CLICK: decide between place cell (no move) and pan left/right (with move)
		if (button == 0) {
			if (state == 0) {
				// Pan
				mousedownnomove = true;
				lastmousex = x;
				lastmousey = y;
			} else if (state == 1) {

				// Mouse button has been pressed, but mouse has not moved -> check state and place cell/trigger infection
				if ( mousedownnomove ) {
					if (mouseState == MOUSESTATE_PLACE_CELL) {
						//cout << "In Mouse state " << mouseState << ": button went " << state << "\n";
						world->addCell(x+xoffset,y+yoffset);
					} else if (mouseState == MOUSESTATE_INFECT_CELL) {
						// We may have panned left/right, so add the offset to the reported mouse position
						world->toggleInfection(x+xoffset,y+yoffset);
					}
				} // Else: Do nothing
				mousedownnomove = false;
			}

		}

		// SETUP MODE ONLY:
		// - Only zoom in/out in simulation mode - causes too many problems in setup mode
		if ( dosimulation ) {
			// WHEEL UP
			if (button == 3) {
				// Zoom in. Wheel is 3 and 4: N.B probably system-specific
				scale += scale*0.05;
				lastmousex = x;
				lastmousey = y;
			// WHEEL DOWN
			} else if (button == 4) {
				// Zoom out. Wheel is 3 and 4: N.B probably system-specific
				scale -= scale*0.05;
				if (scale<0.001) scale = 0.001; // Don't scale less than 1/1000th
				lastmousex = x;
				lastmousey = y;
			}
		}

	}
}

// This is called if the mouse is move whilst EITHER button is down
void GLView::processMouseActiveMotion(int x, int y) {

	// Try sending to Tw
	if ( TwEventMouseMotionGLUT(x, y) ) return;

	// Button is down, but mouse has moved -> suggests we are panning
	mousedownnomove = false;

	if (!dosimulation) {
		// Moving whilst left button is held down depends on the current mouseState
		if (mouseState == MOUSESTATE_DRAW_PATCH) {
			// We may have panned left/right, so add the offset to the reported mouse position
			world->setPatch(x+xoffset,y+yoffset);
		//} else if (mouseState == MOUSESTATE_PLACE_CELL) {
			// We may have panned left/right, so add the offset to the reported mouse position
			//drawCircle(x+xoffset,y+yoffset,10);
		} else {
			// Pan left/right
			xoffset += (lastmousex-x);
			yoffset += (lastmousey-y);
			lastmousex = x;
			lastmousey = y;
		}
	}

	if (dosimulation) {
		// At the moment, just pan left/right
		xoffset += (lastmousex-x);
		yoffset += (lastmousey-y);
		lastmousex = x;
		lastmousey = y;
	}

}

// This is called if the mouse is moved whilst no button is pressed.
void GLView::processMousePassiveMotion(int x, int y) {
	// Try sending to Tw
	if ( !TwEventMouseMotionGLUT(x, y) ) {
		// TODO: Allow selection on CTL?
		// Something like: world->setFocusOnCell(x,y); ?
		if (mouseState == MOUSESTATE_INFECT_CELL) {
			// Let world deal with details of a mouse click on a cell
			//world->toggleInfection(x,y);
		}
	}
}

void GLView::processNormalKeys(unsigned char key, int x, int y) {

	// Need to first send keyboard command to the Tw bar, automatically return if over
	// the menu. (Why? When typing in a 'q' for the filename don't want to quit!).
	if ( TwEventKeyboardGLUT(key,x,y) ) return;

	// Keys for all modes: exit and reset view
    if (key == 27) exit(0);

    if (key=='r') {
   		// reset view
   		scale   = 1.0;
   		xoffset = 0.0;
   		yoffset = 0.0;
    }

    if (key=='d') {
   		//drawing
   		toggleDrawing();
    }

    // Keys for setup
	// Currently none (could do: d=draw patch mode, i=infect cell mode, a=autoplace cells, c=manual place cells)

	// Keys for simulation
	if (dosimulation) {

		if (key=='p') {
			//pause
			paused= !paused;
		} else if (key==43) {
			//+
			skipdraw++;
		} else if (key==45) {
			//-
			skipdraw--;
		} else {
	        printf("Unknown key pressed: %i\n", key);
	    }

	}

}

void GLView::processSpecialKeys(int key, int x, int y) {

	// Should first send to Tw, but there is nothing which is triggered by a "special key"
	// From API: The special keyboard callback is triggered	when keyboard function or directional keys are pressed.

	// SIMULATION
	if (dosimulation) {
		// Skip to next day
		if (key == GLUT_KEY_RIGHT) {
			float timeNow = world->worldtime;
			int nextDay = (int(timeNow / (24*60*60))+1) * 24*60*60;
			cout << "FF to next day. Time now is "<< timeNow << "s, next day is therefore at "<< nextDay << "s\n";
			fastforwardtime = nextDay;
			draw = false;
		}
	}

}

// This is called repeatedly by GLUT (if visualisation is enabled) or by the "game loop"
// The first time handleIdle is called, we execute the function "automaticEventSetup()"
// Then, every time handleIdle is called:
//	- we check whether we have reached the end of the simulation (either by reaching the end time, or if there are no active agents left)
//	- the world is updated (by x seconds)
//  - if in fast-forward mode, we check whether to come out of fast-forward mode
//  - if visualisation is enabled, we update the titlebar (every second) and draw the entire scene at a max of 30fps
void GLView::handleIdle() {

	// Call this the very first time we render the scene
	//if (!processsetupevents) automaticEventSetup();

    // Check if we have reached the end of the simulation
	if ( dosimulation && !paused && world->getWorldTime() >= ibs->endTime ) {
		cout << "Have reached the required world time of " << world->getWorldTime() << "s, ";
		if (!ibs->useGlut) {
			cout << "exiting program.\n";
			exit(0);
		} else {
    		cout << "pausing simulation.\n";
    		paused = true;
    		draw = true;
    	}
	}

    // Check if we have anything left in agents (and later, also in the EventsQueue).
    if ( dosimulation && !paused && world->numAgents() == 0 ) {
    	if (!ibs->useGlut) {
    		// TODO: Graceful exit (remember to flush everything to disk)
    		cout << "No Active Agents in queue, so exiting program (worldtime:"<<world->getWorldTime()<<"s)\n";
    		exit(0);
    	} else {
    		cout << "No Active Agents left, pausing simulation.\n";
    		paused = true;
    		// Need to force a refresh, as we might have hit this spot from a Fast Forward
    		draw = true;
    	}
    }

    modcounter++;
    idlecalled++;

    // Only update the world is we're in simulation mode and the simulation isn't paused.
    if (dosimulation && !paused) world->update();

    // Check if we are in FF mode and have hit the required world time
    if ( fastforwardtime > 0 && world->getWorldTime() >= fastforwardtime ) {
    	// Switch on draw and reset the time to fast forward to
    	draw = true;
    	fastforwardtime = 0;
    }

    // Only do the following if we can draw
    if ( ibs->useGlut ) {
    	//show FPS
    	int currentTime = glutGet( GLUT_ELAPSED_TIME );

		// Every 1000ms(==1s), update the titlebar and reset the frames (fps) variable
		int dt = (currentTime - lastUpdate);
		if (dt >= 1000) {
			sprintf( buf, "FPS: %.2f; CPS: %.2f; NumAgents: %d; Worldtime: %.2f s",
					float(frames)/dt*1000, float(idlecalled)/dt*1000, world->numAgents(), world->getWorldTime() );
			glutSetWindowTitle( buf );
			frames = 0;
			idlecalled = 0;
			lastUpdate = currentTime;
		}

		// Delay draw for a little while (?) if skipdraw (def:1) is <=0.
		if (skipdraw<=0 && draw) {
			clock_t endwait;
			float mult=-0.005*(skipdraw-1); //ugly, ah well
			endwait = clock() + mult * CLOCKS_PER_SEC ;
			while (clock() < endwait) {}
		}

		// Draw scene every time, or every (%skipdraw) calls
		if (draw) {
			// Render if skipdraw is >0 or it's been (skipdraw-1?) cycles without a draw. Keeps to 30fps.
			if (skipdraw<1 || modcounter%skipdraw==0) {
				if (dt >= 1.0/30.0*frames*1000) renderScene();    //increase fps by skipping drawing
			}
		}
    } // End of HAVE_GLUT

}


void GLView::automaticEventSetup() {

	cout << "[GLView] Automatic Events Setting Up..\n";
	// On start (first processIdle call), do the following: load the set up file, switch to sim mode, start the sim
	world->loadLayout();

	// Add some CTL (these will eventually "come into the simulation" after a variable delay).
	world->addAgent( new CTL(100,100) );
	world->addAgent( new CTL(200,200) );
	world->addAgent( new CTL(300,300) );
	world->addAgent( new CTL(400,400) );
	world->addAgent( new CTL(500,500) );
	world->resetShadowLayer();

	SwitchToSimulationMode( world );
	GLVIEW->togglePaused();
	processsetupevents = true;

	cout << "[GLView] Automatic Events Completed.\n";
}

void GLView::renderScene() {

	frames++;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
    glPushMatrix();

	// Global translate (from holding the mouse down and moving)
    glTranslatef(-xoffset,-yoffset,0);

    // Can't get scaling to work properly
    glScalef(scale, scale, 1.0f);

    // Draw the world
    if (draw) {
		// Draw the entire world

		// Reset first (must be an easier way to do this! Why doesn't glClear do this?
		glBegin(GL_QUADS);
		glColor3f(0.1,0.9,1.0);
		glVertex2f(0,0);
		glVertex2f(0,conf::HEIGHT);
		glVertex2f(conf::WIDTH,conf::HEIGHT);
		glVertex2f(conf::WIDTH,0);
		glEnd();

		// Draw background: patches (only if in setup mode), and cells.
		world->drawBackground(this, dosimulation);

		// Add on the active agents
		world->drawForeground(this);
    }

	// Draw the menu
    TwDraw();

    glPopMatrix();

    glutSwapBuffers();
}

void GLView::drawAgent(AbstractAgent* a) {
	// This just calls the agent's draw function
	a->drawAgent(this);
}

/* Scriptbots code: DEPRECATED
void GLView::drawAgent(const Agent& agent) {
    float n;
    float r = conf::BOTRADIUS;
    float rp = conf::BOTRADIUS+2;

    //handle selected agent
    if (agent.selectflag>0) {

        //draw selection
        glBegin(GL_POLYGON);
        glColor3f(1,1,0);
        drawCircle(agent.pos.x, agent.pos.y, conf::BOTRADIUS+5);
        glEnd();

        glPushMatrix();
        glTranslatef(agent.pos.x-80,agent.pos.y+20,0);

        //draw inputs, outputs
        float col;
        float yy=15;
        float xx=15;
        float ss=16;
        glBegin(GL_QUADS);
        for (int j=0;j<INPUTSIZE;j++) {
            col= agent.in[j];
            glColor3f(col,col,col);
            glVertex3f(0+ss*j, 0, 0.0f);
            glVertex3f(xx+ss*j, 0, 0.0f);
            glVertex3f(xx+ss*j, yy, 0.0f);
            glVertex3f(0+ss*j, yy, 0.0f);
        }
        yy+=5;
        for (int j=0;j<OUTPUTSIZE;j++) {
            col= agent.out[j];
            glColor3f(col,col,col);
            glVertex3f(0+ss*j, yy, 0.0f);
            glVertex3f(xx+ss*j, yy, 0.0f);
            glVertex3f(xx+ss*j, yy+ss, 0.0f);
            glVertex3f(0+ss*j, yy+ss, 0.0f);
        }
        yy+=ss*2;

        //draw brain. Eventually move this to brain class?
        float offx=0;
        ss=8;
        for (int j=0;j<BRAINSIZE;j++) {
            col= agent.brain.boxes[j].out;
            glColor3f(col,col,col);
            glVertex3f(offx+0+ss*j, yy, 0.0f);
            glVertex3f(offx+xx+ss*j, yy, 0.0f);
            glVertex3f(offx+xx+ss*j, yy+ss, 0.0f);
            glVertex3f(offx+ss*j, yy+ss, 0.0f);
            if ((j+1)%30==0) {
                yy+=ss;
                offx-=ss*30;
            }
        }

        glEnd();
        glPopMatrix();
    }


    // Fix event drawing
    //draw indicator of this agent... used for various events
//     if (agent.indicator>0) {
//         glBegin(GL_POLYGON);
//         glColor3f(agent.ir,agent.ig,agent.ib);
//         drawCircle(agent.pos.x, agent.pos.y, conf::BOTRADIUS+((int)agent.indicator));
//         glEnd();
//         agent.indicator-=1;
//     }

    //viewcone of this agent
    glBegin(GL_LINES);
    //and view cones
    glColor3f(0.5,0.5,0.5);
    for (int j=-2;j<3;j++) {
        if (j==0)continue;
        glVertex3f(agent.pos.x,agent.pos.y,0);
        glVertex3f(agent.pos.x+(conf::BOTRADIUS*4)*cos(agent.angle+j*M_PI/8),agent.pos.y+(conf::BOTRADIUS*4)*sin(agent.angle+j*M_PI/8),0);
    }
    //and eye to the back
    glVertex3f(agent.pos.x,agent.pos.y,0);
    glVertex3f(agent.pos.x+(conf::BOTRADIUS*1.5)*cos(agent.angle+M_PI+3*M_PI/16),agent.pos.y+(conf::BOTRADIUS*1.5)*sin(agent.angle+M_PI+3*M_PI/16),0);
    glVertex3f(agent.pos.x,agent.pos.y,0);
    glVertex3f(agent.pos.x+(conf::BOTRADIUS*1.5)*cos(agent.angle+M_PI-3*M_PI/16),agent.pos.y+(conf::BOTRADIUS*1.5)*sin(agent.angle+M_PI-3*M_PI/16),0);
    glEnd();

    glBegin(GL_POLYGON); //body
    glColor3f(agent.red,agent.gre,agent.blu);
    drawCircle(agent.pos.x, agent.pos.y, conf::BOTRADIUS);
    glEnd();

    glBegin(GL_LINES);
    //outline
    if (agent.boost) glColor3f(0.8,0,0); //draw boost as green outline
    else glColor3f(0,0,0);

    for (int k=0;k<17;k++)
    {
        n = k*(M_PI/8);
        glVertex3f(agent.pos.x+r*sin(n),agent.pos.y+r*cos(n),0);
        n = (k+1)*(M_PI/8);
        glVertex3f(agent.pos.x+r*sin(n),agent.pos.y+r*cos(n),0);
    }
    //and spike
    glColor3f(0.5,0,0);
    glVertex3f(agent.pos.x,agent.pos.y,0);
    glVertex3f(agent.pos.x+(3*r*agent.spikeLength)*cos(agent.angle),agent.pos.y+(3*r*agent.spikeLength)*sin(agent.angle),0);
    glEnd();

    //and health
    int xo=18;
    int yo=-15;
    glBegin(GL_QUADS);
    //black background
    glColor3f(0,0,0);
    glVertex3f(agent.pos.x+xo,agent.pos.y+yo,0);
    glVertex3f(agent.pos.x+xo+5,agent.pos.y+yo,0);
    glVertex3f(agent.pos.x+xo+5,agent.pos.y+yo+40,0);
    glVertex3f(agent.pos.x+xo,agent.pos.y+yo+40,0);

    //health
    glColor3f(0,0.8,0);
    glVertex3f(agent.pos.x+xo,agent.pos.y+yo+20*(2-agent.health),0);
    glVertex3f(agent.pos.x+xo+5,agent.pos.y+yo+20*(2-agent.health),0);
    glVertex3f(agent.pos.x+xo+5,agent.pos.y+yo+40,0);
    glVertex3f(agent.pos.x+xo,agent.pos.y+yo+40,0);

    //if this is a hybrid, we want to put a marker down
    if (agent.hybrid) {
        glColor3f(0,0,0.8);
        glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo,0);
        glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo,0);
        glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+10,0);
        glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+10,0);
    }

    glColor3f(1-agent.herbivore,agent.herbivore,0);
    glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+12,0);
    glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+12,0);
    glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+22,0);
    glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+22,0);

    //how much sound is this bot making?
    glColor3f(agent.soundmul,agent.soundmul,agent.soundmul);
    glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+24,0);
    glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+24,0);
    glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+34,0);
    glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+34,0);

    glEnd();

    //print stats
    //generation count
    sprintf(buf2, "%i", agent.gencount);
    RenderString(agent.pos.x-conf::BOTRADIUS*1.5, agent.pos.y+conf::BOTRADIUS*1.8, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);
    //age
    sprintf(buf2, "%i", agent.age);
    RenderString(agent.pos.x-conf::BOTRADIUS*1.5, agent.pos.y+conf::BOTRADIUS*1.8+12, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);

    //health
    sprintf(buf2, "%.2f", agent.health);
    RenderString(agent.pos.x-conf::BOTRADIUS*1.5, agent.pos.y+conf::BOTRADIUS*1.8+24, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);

    //repcounter
    sprintf(buf2, "%.2f", agent.repcounter);
    RenderString(agent.pos.x-conf::BOTRADIUS*1.5, agent.pos.y+conf::BOTRADIUS*1.8+36, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);
}
*/

void GLView::drawCell(const Cell &cell) {
	// Useful variables
	float n;
	float r = cell.radius;
	float * cColor = world->getCellColourFromType(cell.cType);

	// 1. Draw the body (simple circle)
    glBegin(GL_POLYGON);
    glColor3f(cColor[0],cColor[1],cColor[2]);
    drawCircle(cell.pos.x, cell.pos.y, r);
    glEnd();

    // 2. Draw a black line around the circle
    glBegin(GL_LINES);
    glColor3f(0,0,0); // Black outline
    // Draw outline as set of 16 lines around the circumference
    for (int k=0;k<17;k++) {
        n = k*(M_PI/8);
        glVertex3f(cell.pos.x+r*sin(n), cell.pos.y+r*cos(n),0);
        n = (k+1)*(M_PI/8);
        glVertex3f(cell.pos.x+r*sin(n), cell.pos.y+r*cos(n),0);
    }
    glEnd();

    // Display cell type, if debug is on
    // TODO: Add debug bool to switch on/off cell loc information?
    if (false) {
    	sprintf(buf2, "%i", cell.cType);
    	RenderString(cell.pos.x-conf::BOTRADIUS*1.5, cell.pos.y+conf::BOTRADIUS*1.8+5, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);
    	//sprintf(buf2, "(%.0f,%.0f)", cell.pos.x, cell.pos.y);
    	//RenderString(cell.pos.x-conf::BOTRADIUS*1.5, cell.pos.y+conf::BOTRADIUS*1.8+15, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);
    }
}

void GLView::drawProgressBar(float completed) {

	int halfW = conf::WIDTH/2;
	int halfH = conf::HEIGHT/2;

	int boxW = 180*2;
	int boxH = 100*2;

	// 1. Shift the stack
	glPushMatrix();

	// 2. Draw the outline box
	glTranslatef(halfW-boxW/2, halfH-boxH/2, 0);
	glBegin(GL_QUADS);
    glColor4f(1.0,0,0,0.5);
    glVertex3f(0,0,0);
    glVertex3f(boxW,0,0);
    glVertex3f(boxW,boxH,0);
    glVertex3f(0,boxH,0);
    glEnd();

    // 2b. Add some interesting text
    char bufl[150];
    sprintf(bufl, "Please wait..");
    RenderString(10, 30, GLUT_BITMAP_TIMES_ROMAN_24, bufl, 0.0f, 0.0f, 0.0f);
    sprintf(bufl, "Complicated (unoptimised) calculations are being performed..");
    RenderString(10, 55, GLUT_BITMAP_TIMES_ROMAN_24, bufl, 0.0f, 0.0f, 0.0f);


	// 3. Draw the progress outline box, relative to the previous box
	glTranslatef(10,boxH/2-25, 0);
	glBegin(GL_QUADS);
    glColor3f(0,0,0);
    glVertex3f(0,0,0);
    glVertex3f(boxW-20,0,0);
    glVertex3f(boxW-20,50,0);
    glVertex3f(0,50,0);
    glEnd();

	// 3. Draw the progress bar itself
	glTranslatef(1,1, 0);
	glBegin(GL_QUADS);
    glColor3f(1,1,1);
    glVertex3f(0,0,0);
    glVertex3f(completed*(boxW-20-2),0,0);
    glVertex3f(completed*(boxW-20-2),48,0);
    glVertex3f(0,48,0);
    glEnd();

    // 4. Reset the stack
    glPopMatrix();

}

void GLView::toggleDrawing() {
	draw = !draw;
}

void GLView::togglePaused() {
	paused = !paused;
}


void GLView::drawDot(int x, int y) {
	// Draw the user-specified patch
    glBegin(GL_POINTS);
    glColor3f(0.5,0.5,0.5);
	glVertex2f(x,y);
    glEnd();
}

void GLView::setWorld(World* w) {
    world = w;
}
