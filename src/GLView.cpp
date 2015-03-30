#include "GLView.h"

#ifndef IMMUNEBOTS_NOGL
#include <GL/glu.h>
#include <GL/glut.h>
#include <AntTweakBar.h>
#endif

#include <ctime>
#include <stdio.h>
#include <string>

#include "settings.h"
#include "vmath.h"
#include "ImmunebotsSetup.h"
#include "config.h"
#include "World.h"
#include "AbstractAgent.h"
#include "Cell.h"
#include "CTL.h"

#include <vector>

/*Test libraries*/
#include <fstream>
#include <iostream>

using namespace std;

/* GL handler functions */
#ifndef IMMUNEBOTS_NOGL
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

#endif

/* More GL functions */

#ifndef IMMUNEBOTS_NOGL

void GLView::RenderString(float x, float y, void *font, const char* string, float r, float g, float b) {
    glColor4f(r,g,b,1.0);
    glRasterPos2f(x, y);
    int len = (int) strlen(string);
    for (int i = 0; i < len; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
}

void GLView::drawCircle(float x, float y, float z, float r, bool draw_outline) {
    float n;
    glPushMatrix();
    if (conf::USE_VERTEX_ARRAY) {
    	glTranslatef(x,y,z);
    	glScalef(r,r,r);
    	glCallList(cell_list);
    	if (draw_outline) { glCallList(cell_outline_list); }
    } else {

    	glBegin(GL_POLYGON);
    	for (int k=0;k<17;k++) {
    		n = k*(M_PI/8);
    		glVertex4f(x+r*sin(n),y+r*cos(n),z,1.0);
    	}
    	glEnd();

    	if (draw_outline) {
   	    	glBegin(GL_LINES);
   	    	glColor3f(0,0,0); // Black outline
   	    	// Draw outline as set of 16 lines around the circumference
   	    	for (int k=0;k<17;k++) {
   	    		n = k*(M_PI/8);
   	    		glVertex3f(x+r*sin(n), y+r*cos(n),z+0.001);
   	    		n = (k+1)*(M_PI/8);
   	    		glVertex3f(x+r*sin(n), y+r*cos(n),z+0.001);
   	    	}
   	    	glEnd();
    	}
    }
    glPopMatrix();
}
#endif


/* TW Menu functions */

#ifndef IMMUNEBOTS_NOGL

// TW Callback function to reset the world
void TW_CALL ResetWorld( void * world ) {
	// This will reset the world by clearing the patch layer, the cells and the CTL.
	// Should also reset the time.
	printf("DEBUG: Resetting World\n");
	((World *)world)->reset(); // Crashes program!! Try again, should work now
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
	// TODO: See notes for LoadLayout
	((World*)world)->placeCells();
}

void TW_CALL DropCTL( void * world ) {
	((World*)world)->dropCTL();
}

void TW_CALL SaveLayout( void * world ) {
	// TODO: See notes for LoadLayout
	((World*)world)->saveLayout();
}

void TW_CALL LoadLayout( void * world ) {
	// TODO: Can we add a "loading" screen here?
	// Note: May have to 'queue' the loadLayout function, i.e. set a bool which:
	// - shows a loading screen
	// - calls loadLayout()
	// - removes the loading screen
	((World*)world)->loadLayout();
	// Do we scale by Width (x) or Height (y)?
	float bw = (((World*)world)->bounding_max.x-((World*)world)->bounding_min.x);
	float bh = (((World*)world)->bounding_max.y-((World*)world)->bounding_min.y);
	float zoom;
	if (bw/GLVIEW->currentWidth > bh/GLVIEW->currentHeight) {
		zoom = (GLVIEW->currentWidth-50.0)/bw;
	} else {
		zoom = (GLVIEW->currentHeight-50.0)/bh;
	}

	// Find centre of the bounding box
	float tx = ((World*)world)->bounding_min.x + bw/2.0;
	float ty = ((World*)world)->bounding_min.y + bh/2.0;
	GLVIEW->setCamera( tx*zoom, ty*zoom, zoom );
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
void TW_CALL GetMouseStatePlaceCTL(void *value, void *v) {
	// This just casts the void pointers (if required) and sets the right MouseState to compare to.
	GetMouseState(value, (GLView *)v, MOUSESTATE_PLACE_CTL);
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
void TW_CALL SetMouseStatePlaceCTL(const void *v, void *view) {
	// Cast bool and view, adding the required mouseState if v
	SetMouseState( *(const bool *)v, (GLView *)(view), MOUSESTATE_PLACE_CTL);
}
void TW_CALL CopyStdStringToClient(std::string& destinationClientString, const std::string& sourceLibraryString) {
  // Copy the content of souceString handled by the AntTweakBar library to destinationClientString handled by your application
  destinationClientString = sourceLibraryString;
}

/* Create the TW menus */

void GLView::createSetupMenu(bool visible) {

	TwBar* bar = TwNewBar("Setup");

	TwDefine("Setup color='0 0 0' alpha=128 position='10 10' size='300 500'");
	TwDefine("Setup fontresizable=false resizable=true valueswidth=100");

	//TwAddVarRO(bar, "MouseState", TW_TYPE_INT32, (&mouseState), " label='Mouse state' " );

	/* PATCHES  */
	TwAddVarCB(bar, "dp_switch", TW_TYPE_BOOLCPP, SetMouseStateDrawPatch, GetMouseStateDrawPatch, this,
    		" label='Draw patches' group='Patch Setup' help='Toggle to switch on patch draw mode.' ");
	TwAddButton(bar, "placecells_toggle", PlaceCellsOnPatches, world, " label='  Place cells on patches' group='Patch Setup' ");

	TwAddSeparator(bar, NULL, NULL);

	//TwAddVarRW(bar,  "sp_roto", TW_TYPE_FLOAT, &((*world).SUSCEPTIBLE_PERCENTAGE), " label='Susceptible cells (%)' min=0 max=100 group='Cells Setup' step=0.1 ' ");
	TwAddButton(bar, "sp_button", ResetSusceptibleCells, world, " label='  Reset cell susceptibility' group='Cells Setup' ");

	TwAddVarRW(bar,  "nc_roto", TW_TYPE_INT32, &((*world).DEFAULT_NUM_CELLS_TO_ADD), " label='Number of cells' min=100 max=10000 group='Cells Setup' step=50 ' ");
	TwAddButton(bar, "cd_button", AddCells, world, " label='  Add cells' group='Cells Setup' ");
	TwAddButton(bar, "clear_cells", ClearAllCells, world, " label='Clear all cells' group='Cells Setup' ");

	// Allow user to change the cell colours (Cell Setup/Cell colour, starts off closed)
	TwAddVarRW(bar, "ctn_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_NOT_SUSCEPTIBLE), 	" label='Not susceptible' group='Cell colour' ");
	TwAddVarRW(bar, "cts_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_SUSCEPTIBLE), 		" label='Susceptible' group='Cell colour' ");
	TwAddVarRW(bar, "cti_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_INFECTED), 		" label='Infected' group='Cell colour' ");
	TwAddVarRW(bar, "ctc_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_CTL),				" label='CTL' group='Cell colour' ");
	TwAddVarRW(bar, "ctd_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_DEAD),				" label='Dead' group='Cell colour' ");
	TwDefine(" Setup/'Cell colour' group='Cells Setup' opened=false ");  // group Color is moved into group Cells Setup

    // Callback to switch on/off the cell dropper
	TwAddVarCB(bar, "pc_switch", TW_TYPE_BOOLCPP, SetMouseStatePlaceCell, GetMouseStatePlaceCell, this,
    		" label='Manually place cells' group='Cells Setup' help='Toggle to switch on cell dropping mode.' ");

	// Callback to the manual cell infector on/off switch
	TwAddVarCB(bar, "ic_switch", TW_TYPE_BOOLCPP, SetMouseStateInfectCell, GetMouseStateInfectCell, this,
    		" label='Manually infect cell' group='Cells Setup' help='Toggle to switch on infect cell mode.' ");

	TwAddSeparator(bar, NULL, NULL);

	TwAddVarRW(bar,  "cp_roto", TW_TYPE_FLOAT, &((*world).CTL_DENSITY), " label='CTL density (mm^-2)' min=0 max=500 group='CTL Setup' step=1.0 ' ");
	TwAddButton(bar, "cp_button", DropCTL, world, " label='  Add CTL' group='CTL Setup' ");
	// Callback to the drop CTL on/off switch
	TwAddVarCB(bar, "ctl_place_switch", TW_TYPE_BOOLCPP, SetMouseStatePlaceCTL, GetMouseStatePlaceCTL, this,
    		" label='Manually place CTL' group='CTL Setup' help='Toggle to switch on place CTL mouse mode.' ");

	TwAddSeparator(bar, NULL, NULL);

	TwCopyStdStringToClientFunc(CopyStdStringToClient); /* Bit of magic to copy user-defined strings to the program */
	TwAddVarRW(bar, "filename_str", TW_TYPE_STDSTRING, &((*ibs).layoutfilename), "label='Setup filename' group='Setup load/save' ");
	TwAddButton(bar, "save_Setup", SaveLayout, world, " label='Save Setup' group='Setup load/save' ");
	TwAddButton(bar, "load_Setup", LoadLayout, world, " label='Load Setup' group='Setup load/save' ");

	TwAddSeparator(bar, NULL, NULL);

	TwAddButton(bar, "iws_button", ResetWorld, world, " label='  Reset World'");
	//TwAddButton(bar, "pl_toggle", NULL, NULL, "label='  Toggle patch layer'");
	//TwAddButton(bar, "reset_colour_button", NULL, NULL, " label='  Reset cell colour' ");

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

	TwAddVarRW(barsm, "togg_stepmode", TW_TYPE_BOOLCPP, (&stepmode), " label='Toggle stepmode' ");
	TwAddVarRW(barsm, "togg_syncframerate", TW_TYPE_BOOLCPP, (&sync_framerate), " label='Limit simulation to 60CPS' ");

	TwAddSeparator(barsm, NULL, NULL);

	TwAddButton(barsm, "switchToSetup_button", SwitchToSetupMode, world, " label='Return to Setup Mode' ");

	// And hide if required
	if (!visible) {
		TwDefine("'Simulation' visible=false");
	}

}

void GLView::createStatsMenu(bool visible) {

	TwBar* barst = TwNewBar("Statistics");

	char buffer[255];
	int sim_width = 320;
	sprintf(buffer, " Statistics color='0 0 0' alpha=128 position='%i 15' size='%i 350' ", conf::WWIDTH-sim_width-15, sim_width);

	TwDefine(buffer);
	TwDefine(" Statistics fontresizable=false resizable=true valueswidth=100 ");

	/* Absolute counts */
	TwAddVarRO(barst, "cell_total",          TW_TYPE_INT32, (&stats->total_cells),   " label='Total cells' group='Absolute counts' " );
	TwAddVarRO(barst, "cell_notsusceptible", TW_TYPE_INT32, (&stats->notsusceptible), " label='Not susceptible cells' group='Absolute counts' " );
	TwAddVarRO(barst, "cell_susceptible",    TW_TYPE_INT32, (&stats->susceptible), " label='Susceptible cells' group='Absolute counts' " );
	TwAddVarRO(barst, "cell_infected",       TW_TYPE_INT32, (&stats->infected), " label='Infected cells' group='Absolute counts' " );
	TwAddVarRO(barst, "cell_ctl",            TW_TYPE_INT32, (&stats->ctl),   " label='Killer T cells' group='Absolute counts' " );
	TwAddVarRO(barst, "cell_virion",         TW_TYPE_INT32, (&stats->virion), " label='Virions' group='Absolute counts' " );

	/* Event counter, e.g. lysis and death of infecteds */
	TwAddVarRO(barst, "event_infectedlysis",   TW_TYPE_INT32, (&stats->infected_lysis),   " label='CTL lysis' group='Events' " );
	TwAddVarRO(barst, "event_infecteddeath",   TW_TYPE_INT32, (&stats->infected_death),   " label='Virus-induced cell death' group='Events' " );
	TwAddVarRO(barst, "event_failedinfection", TW_TYPE_INT32, (&stats->failed_infection), " label='Failed infections' group='Events' " );
	TwAddVarRO(barst, "event_ctlscan",         TW_TYPE_FLOAT, (&stats->scan_average), " label='Average # cells scanned (cells/min)' group='Events' " );

	/* Area */
	TwAddVarRO(barst, "bounding_box_area", TW_TYPE_FLOAT, (&stats->area_mm2),  " label='Bounding box (mm^2)' group='Area' " );
	TwAddVarRO(barst, "cell_area",         TW_TYPE_FLOAT, (&stats->cell_area), " label='Cell area (mm^2)' group='Area' " );

	/* Density counter of CTL and virions per mm^2 or per total cells (e.g. 1 in 1000) */
	TwAddVarRO(barst, "density_ctl_permm2",     TW_TYPE_FLOAT, (&stats->ctl_per_mm2),     " label='CTL per mm^2'     group='Density' " );
	TwAddVarRO(barst, "density_ctl_percell",    TW_TYPE_FLOAT, (&stats->ctl_per_cell),    " label='CTL per cell'     group='Density' " );
	TwAddVarRO(barst, "density_ctl_perinfected",TW_TYPE_FLOAT, (&stats->ctl_per_infected)," label='CTL per target (E:T)' group='Density' " );
	TwAddVarRO(barst, "density_virion_permm2",  TW_TYPE_FLOAT, (&stats->virion_per_mm2),  " label='Virions per mm^2' group='Density' " );
	TwAddVarRO(barst, "density_virion_percell", TW_TYPE_FLOAT, (&stats->virion_per_cell), " label='Virions per cell' group='Density' " );
	//TwAddVarRO(barst, "cell_infected",    TW_TYPE_STDSTRING, &(stats->infected_str), "label='Infected cell # (%)' group='Cells' ");
	//TwAddSeparator(barst, NULL, NULL);

	// And hide if required
	if (!visible) {
		TwDefine(" Statistics visible=false ");
	}
}

#endif

/* Create the world */

GLView::GLView(World *w, ImmunebotsSetup *is):
        dosimulation(false),
        paused(true),
        draw(true),
        skipdraw(1),
        stepmode(false),
        doanotherstep(false),
        sync_framerate(false),
        modcounter(0),
        frames(0),
        idlecalled(0),
        lastUpdate(0),
        mouseState(MOUSESTATE_DEFAULT),
    	lastmousey(0),
    	lastmousex(0),
    	mousedownnomove(false),
    	scale(1.0),
		processsetupevents(false),
		fastforwardtime(0)
{
	world = w;
	ibs   = is;

	// Initialise the camera
	cam = (Camera*) malloc( sizeof( Camera ) );
	cam->x = 0.0;
	cam->y = 0.0;
	cam->z = 0.0;
	cam->heading = 0.0;
	cam->pitch = 0.0;
	cam->roll = 0.0;
	cam->zoom = 0.10;

	// Initialise the stats module
	stats = world->stats;
}

GLView::~GLView() {

}

#ifndef IMMUNEBOTS_NOGL

// Switch over to Simulation mode if dosim (always pause)
void GLView::switchToSimulationMode(bool dosim) {
	dosimulation = dosim;
	paused = true;
	doanotherstep = true;
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
	if (h==0) {h=1.0;} // Prevent divide by zero
	currentWidth  = w;
	currentHeight = h;

    // Reset the coordinate system before modifying
	glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho( -w/2, w/2, h/2, -h/2, -conf::FAR_PLANE-1.0, conf::FAR_PLANE+1.0 );

	// Inform Menubar (system) of the new window coordinates
    TwWindowSize(w, h);
}

/* Given a mouse cursor location ("WindowSpace"), translates into where in the model the mouse cursor is pointing ("WorldSpace") */
void GLView::getMouseWorldCoords(int x, int y, int z, GLdouble* ws) {

	/* Complicated code to translate the mouse position into a world co-ord */
	GLdouble mvmatrix[16];
	GLdouble projmatrix[16];
	GLint viewport[4];

	// Get the matrices
	glGetDoublev(  GL_MODELVIEW_MATRIX,  mvmatrix   );
	glGetDoublev(  GL_PROJECTION_MATRIX, projmatrix );
	glGetIntegerv( GL_VIEWPORT,          viewport   );

	gluUnProject(x,y,1.0,
			mvmatrix,projmatrix,viewport,
			ws, ws+1, ws+2
	);

	// Correct for camera zoom and position
	*(ws+0) = ((cam->x+*(ws+0))/cam->zoom);
	*(ws+1) = ((cam->y-*(ws+1))/cam->zoom);
}

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

					GLdouble ws[3] = {0,0,0};
					this->getMouseWorldCoords(x, y, 0, ws);

					if ( !dosimulation ) {

						if (mouseState == MOUSESTATE_PLACE_CELL) {
							//cout << "In Mouse state " << mouseState << ": button went " << state << "\n";
							world->addCell(floor(ws[0]),floor(ws[1]));
						} else if (mouseState == MOUSESTATE_INFECT_CELL) {
							// We may have panned left/right, so add the offset to the reported mouse position
							world->toggleInfection(floor(ws[0]),floor(ws[1]));
						} else if (mouseState == MOUSESTATE_PLACE_CTL) {
							// We may have panned left/right, so add the offset to the reported mouse position
							world->addCTL(floor(ws[0]),floor(ws[1]));
						}

					} else { // End of !dosimulation
						// In Simulation mode
						if (world->isOverCell(ws[0],ws[1])) {
							Cell * c = world->getCell(ws[0],ws[1]);
							cout << "[MP] Picked cell["<<c<<"] - "<<c->lastScanTime<<"s ("<<(c->agent_type)<<")\n";
						}
					}

				} // Else: Do nothing
				mousedownnomove = false;
			}

		}

		// SETUP MODE ONLY:
		// - Only zoom in/out in simulation mode - causes too many problems in setup mode
		//if ( dosimulation ) {
			// WHEEL UP
			if (button == 3) {
				// Zoom in. Wheel is 3 and 4: N.B probably system-specific
				cam->zoom *= 1.05;
				lastmousex = x;
				lastmousey = y;
			// WHEEL DOWN
			} else if (button == 4) {
				// Zoom out. Wheel is 3 and 4: N.B probably system-specific
				cam->zoom *= 0.95;
				//if (cam->zoom<0.001) cam->zoom = 0.001; // Don't scale less than 1/1000th
				lastmousex = x;
				lastmousey = y;
			}
			//cout << "[PM] zoom is "<< cam->zoom << " @("<<cam->x<<","<<cam->y<<"), bb is("<<world->bounding_max.x-world->bounding_min.x<<","<<world->bounding_max.y-world->bounding_min.y<<")"<<endl;
		//}

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

			GLdouble ws[3] = {0,0,0};
			this->getMouseWorldCoords(x, y, 0, ws);

			// We may have panned left/right, so add the offset to the reported mouse position
			world->setPatch(floor(*ws),floor(*(ws+1)));
		//} else if (mouseState == MOUSESTATE_PLACE_CELL) {
			// We may have panned left/right, so add the offset to the reported mouse position
			//drawCircle(x+xoffset,y+yoffset,10);
		} else {
			// Pan left/right
			cam->x += (lastmousex-x);
			cam->y += (lastmousey-y);
			lastmousex = x;
			lastmousey = y;
		}
	}

	if (dosimulation) {
		// Currently we just pan left/right
		cam->x += (lastmousex-x);
		cam->y += (lastmousey-y);
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

	// Exit program
    if (key == 27) exit(EXIT_SUCCESS);

	// Reset view
    if (key=='r') {
   		cam->zoom = 1.0;
   		cam->x    = 0.0;
   		cam->y    = 0.0;
   		cam->z    = 0.0;
    }

    // Drawing toggle
    if (key=='d') {
   		toggleDrawing();
    }

    // Fullscreen toggle (TODO)
    // See: http://biospud.blogspot.com/2009/08/write-your-own-stereoscopic-3d-program.html
    if (key=='f') {
    	// If not in Game mode: destroy window, setup game mode, enter game mode, create context
    	// Else: leave game mode, create window, create context
    	//glutGameModeString("800x600:32@60");
    	//glutEnterGameMode();
    }

    // Keys for setup
	// Currently none (could do: d=draw patch mode, i=infect cell mode, a=autoplace cells, c=manual place cells or place CTL)

	// Keys for simulation
	if (dosimulation) {

		if (key=='p') {
			//pause
			paused = !paused;
		} else if (key==32) {
			// space (step) do one iteration then pause until space is pressed again
			this->toggleStepMode(true);
		}  else if (key==43) {
			//+
			skipdraw++;
		} else if (key==45) {
			//-
			skipdraw--;
		} else if (key=='s') {
			// "Slow": Cheap way to limit the simulation speed to the max framerate
			sync_framerate = !sync_framerate;
		} else {
	        printf("Unknown key pressed: %i\n", key);
	    }

	}

}

void GLView::processSpecialKeys(int key, int x, int y) {

	if ( TwEventKeyboardGLUT(key,x,y) ) return;

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
#endif

// This is called repeatedly by GLUT (if visualisation is enabled) or by the "game loop"
// The first time handleIdle is called, we execute the function "automaticEventSetup()"
// Then, every time handleIdle is called:
//	- we check whether we have reached the end of the simulation (either by reaching the end time, or if there are no active agents left)
//	- the world is updated (by x seconds)
//  - if in fast-forward mode, we check whether to come out of fast-forward mode
//  - if visualisation is enabled, we update the titlebar (every second) and draw the entire scene at a max of 30fps
void GLView::handleIdle() {

	// Call this the very first time we render the scene
	if (!processsetupevents) automaticEventSetup();

    // Check if we have reached the end of the simulation
	if ( dosimulation && !paused && ibs->hasReachedEnd() ) { //world->getWorldTime() >= ibs->endTime ) {
		cout << " - Simulation has been paused or has reached an end condition, ";
		if (ibs->automaticRun) {
			cout << "exiting program." << endl;
			paused = true; //returns control to the headless loop in GLVIEW
		} else {
    		cout << "pausing simulation." << endl;
    		paused = true;
    		draw = true;
    	}
	}

    // Check if we have anything left in agents (and later, also in the EventsQueue).
    if ( dosimulation && !paused && world->numAgents() == 0 ) {
    	if (!ibs->useGlut) {
    		// TODO: Graceful exit (remember to flush everything to disk)
    		cout << "No Active Agents in queue, so exiting program (worldtime:"<<world->getWorldTime()<<"s)\n";
    		exit(EXIT_SUCCESS);
    	} else {
    		cout << "No Active Agents left, pausing simulation.\n";
    		paused = true;
    		// Need to force a refresh, as we might have hit this spot from a Fast Forward
    		draw = true;
    	}
    }

    idlecalled++;

    // Reset modcounter
    if (modcounter>=10000) {
        modcounter=0;
    }

	// Update stats and write-out report every 300 timesteps (i.e. every 5 minutes)
	if (!paused && modcounter%ibs->getParm("report_time",300)==0) {
		world->updateStats(false);
	}

    // Only update the world is we're in simulation mode and the simulation isn't paused.
    if (dosimulation && !paused && (!stepmode || doanotherstep)) {
    	world->update(modcounter%ibs->getParm("report_time",300)==0);
    	if (stepmode) {
    		doanotherstep = false;
    	}
    }

    // Check if we are in FF mode and have hit the required world time
    if ( fastforwardtime > 0 && world->getWorldTime() >= fastforwardtime ) {
    	// Switch on draw and reset the time to fast forward to
    	draw = true;
    	fastforwardtime = 0;
    }

    // Only do the following if we can draw
#ifndef IMMUNEBOTS_NOGL
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

		// Delay draw (and indeed, world updating) for a little while (?) if skipdraw (def:1) is <=0.
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
				if (dt >= 1.0/30.0*frames*1000) glutPostRedisplay(); //renderScene();    //increase fps by skipping drawing
			}
		}
    } // End of HAVE_GLUT
#endif

    modcounter++;
}


void GLView::automaticEventSetup() {

	/*
	cout << "[GLView] Automatic Events Setting Up..\n";

	for (int i=0; i<100; i+=10) {
		for (int j=0; j<100; j+=10) {
			world->addCell(i,j);
		}
	}
	world->addCell( 90,100);
	world->addCell(100, 90);
*/


	/*
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
	*/

	// Always do this
	processsetupevents = true;
	//cout << "[GLView] Automatic Events Completed.\n";

}

#ifndef IMMUNEBOTS_NOGL
void GLView::renderScene() {

	frames++;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();

    // 1. Put the "camera" in thte right place for all actions
	glRotatef(-cam->roll,    0, 0, 1);
	glRotatef(-cam->pitch,   1, 0, 0);
	glRotatef(-cam->heading, 0, 1, 0);
	glTranslatef(-cam->x, -cam->y, -cam->z);
	glScalef(cam->zoom, cam->zoom, 1.0);

    glPushMatrix();

    // Draw the world
    if (draw) {
		// Draw background - world colour, bounding box and patches (if in setup mode)
		world->drawBackground(this, dosimulation);

		// Add on the cells and active agents (CTL/virions/etc).
		world->drawForeground(this);
    }

	// Draw the menu
    TwDraw();

    glPopMatrix();

    glPopMatrix();
    glutSwapBuffers();

    if (sync_framerate) {
    	glutPostRedisplay(); // Uncommenting this limits the FPS and CPS to 60.
    }
}
#endif

#ifndef IMMUNEBOTS_NOGL
void GLView::drawAgent(AbstractAgent* a) {
	// This just calls the agent's draw function
	a->drawAgent(this);
}
#endif

// Allow this to be defined, but as an empty function
#ifndef IMMUNEBOTS_NOGL
void GLView::drawCell(Cell *cell) {

	// Useful variables
	float r = cell->radius;

	// 1. Draw the body (simple circle)
	// Special colour for uninfected cells)
	if ((cell->agent_type == AbstractAgent::AGENT_NOT_SUSCEPTIBLE) && (cell->lastScanTime>1.0)) {
		// Create a blueish colour based on the last cell type
		float decayFactor = 12*60*60; // Decay in 12h
		float timeSinceScan = world->worldtime - cell->lastScanTime + 1.0/decayFactor;
		// Now interpolate between verylightblue (200,200,220) and mediumblue (70,70,170)
		glColor4f(
			( 70.0+(60.0*timeSinceScan/decayFactor))/255.0,
			( 70.0+(60.0*timeSinceScan/decayFactor))/255.0,
			(170.0+(25.0*timeSinceScan/decayFactor))/255.0,
			 1.0);
	} else {
		glColor4fv( world->getCellColourFromType(cell->agent_type) );
	}
    drawCircle(cell->pos.x, cell->pos.y, cell->draw_priority, r, true);

    glPushMatrix();

    // Display cell type, if debug is on
    // TODO: Add debug bool to switch on/off cell loc information?
    if (false) {
    	sprintf(buf2, "%i", cell->agent_type);
    	RenderString(cell->pos.x-conf::BOTRADIUS*1.5, cell->pos.y+conf::BOTRADIUS*1.8+5, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);
    	//sprintf(buf2, "(%.0f,%.0f)", cell.pos.x, cell.pos.y);
    	//RenderString(cell.pos.x-conf::BOTRADIUS*1.5, cell.pos.y+conf::BOTRADIUS*1.8+15, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);
    }

    glPopMatrix();
}
#endif

#ifndef IMMUNEBOTS_NOGL
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
#endif

void GLView::toggleDrawing() {
	draw = !draw;
}

void GLView::togglePaused() {
	paused = !paused;
}

void GLView::toggleStepMode(bool onoff) {

	if (onoff) {
		if (!stepmode) {
			stepmode = true;
			doanotherstep = false;
		} else {
			doanotherstep = true;
		}
	}
}

void GLView::setPaused(bool p) {
	paused = p;
}

// Checks if setup has been done, i.e. there are cells and agents, and an endtime.
void GLView::checkSetup() {

	if ( world->numAgents()>0 ) {
		if ( world->numCells() > 0) {
			if (ibs->endTime>0) {
				dosimulation = true;
				return;
			} else {
				cout << "ERROR: No end time set. Aborting..\n";
			}
		} else {
			cout << "ERROR: No cells in simulation. Aborting..\n";
		}
	} else {
		cout << "ERROR: No agents (infected, virions or CTL) in simulation. Aborting..\n";
	}

	// If we reach here then something is wrong - abort!
	exit(EXIT_FAILURE);
}

#ifndef IMMUNEBOTS_NOGL
void GLView::drawDot(int x, int y, float z) {
	// Draw a dot! Used for patches (and virions)
    glBegin(GL_POINTS);
    glColor3f(0.5,0.5,0.5);
	glVertex3f(x,y,z);
    glEnd();
}
#endif

void GLView::setWorld(World* w) {
    world = w;
}

void GLView::setCamera(float tx, float ty, float z) {
	cam->x = tx;
	cam->y = ty;
	cam->zoom = z;
}

// Setup the simple cell vertex list
#ifndef IMMUNEBOTS_NOGL
void GLView::setupDisplayLists() {
	if (conf::USE_VERTEX_ARRAY) {

		cell_list = glGenLists(1);
		glNewList( cell_list, GL_COMPILE );
		glBegin(GL_POLYGON);
		float n;
		for (int k=0;k<17;k++) {
			n = k*(M_PI/8);
			glVertex4f(1.0*sin(n),1.0*cos(n),0.0,1.0);
		}
		glEnd();
		glEndList();


		cell_outline_list = glGenLists(1);
		glNewList( cell_outline_list, GL_COMPILE );
		glBegin(GL_LINES);
    	glColor3f(0,0,0); // Black outline
    	// Draw outline as set of 16 lines around the circumference
    	for (int k=0;k<17;k++) {
    		n = k*(M_PI/8);
    		glVertex3f(1.0*sin(n), 1.0*cos(n),0.001);
    		n = (k+1)*(M_PI/8);
    		glVertex3f(1.0*sin(n), 1.0*cos(n),0.001);
    	}
    	glEnd();
    	glEndList();
	}
}
#endif



