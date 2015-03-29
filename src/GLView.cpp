#include "GLView.h"
#include <AntTweakBar.h>
#include <ctime>
#include <stdio.h>
#include "config.h"
#include <GL/glut.h>

#include <stdio.h>

/*Test libraries*/
#include <fstream>
#include <iostream>

using namespace std;

void gl_processNormalKeys(unsigned char key, int x, int y) {
    GLVIEW->processNormalKeys(key, x, y);
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


void RenderString(float x, float y, void *font, const char* string, float r, float g, float b) {
    glColor3f(r,g,b);
    glRasterPos2f(x, y);
    int len = (int) strlen(string);
    for (int i = 0; i < len; i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
}

void drawCircle(float x, float y, float r) {
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

void TW_CALL DoCellJitter( void * world ) {
	((World*)world)->startJitter();
}

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

void GLView::addMenu() {

	TwInit(TW_OPENGL, NULL);
	TwWindowSize(conf::WWIDTH,conf::WHEIGHT);
	//TwDeleteAllBars();
	TwBar* bar = TwNewBar("Immunebots");

	TwDefine("Immunebots color='0 0 0' alpha=128 position='10 10' size='240 500'");
	TwDefine("Immunebots fontresizable=false resizable=true");

	//TwEnumVal v[] = { { 0, "muted" }, { 1, "fire" }, { 2, "purple" }, { 3, "rainbow" }, { 4, "sky" } };
	//TwType type = TwDefineEnum("colors", &v[0], 5);
	//TwAddVarRW(bar, "Colors", type, &mPaletteIdx, "");

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

	// Allow user to change the cell colours (Cell Setup/Cell colour, starts off closed)
	TwAddVarRW(bar, "ctn_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_NOT_SUSCEPTIBLE), 	" label='Not susceptible' group='Cell colour' ");
	TwAddVarRW(bar, "cts_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_SUSCEPTIBLE), 		" label='Susceptible' group='Cell colour' ");
	TwAddVarRW(bar, "cti_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_INFECTED), 		" label='Infected' group='Cell colour' ");
	TwAddVarRW(bar, "ctc_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_CTL),				" label='CTL' group='Cell colour' ");
	TwAddVarRW(bar, "ctd_colour", TW_TYPE_COLOR3F, &((*world).COLOUR_DEAD),				" label='Dead' group='Cell colour' ");
	TwDefine(" Immunebots/'Cell colour' group='Cells Setup' opened=false ");  // group Color is moved into group Cells Setup

	// Callback to the manual cell infector on/off switch
	TwAddVarCB(bar, "ic_switch", TW_TYPE_BOOLCPP, SetMouseStateInfectCell, GetMouseStateInfectCell, this,
    		" label='Manually infect cell' group='Cells Setup' help='Toggle to switch on infect cell mode.' ");
	TwAddButton(bar, "clear_cells", ClearAllCells, world, " label='Clear all cells' group='Cells Setup' ");

	TwAddSeparator(bar, NULL, NULL);

	TwAddButton(bar, "toggle_label", NULL, NULL, "label='Display options:'");
	TwAddVarRW(bar, "fl_toggle", TW_TYPE_BOOLCPP, &drawfood,       " label='  Show food' ");
	TwAddVarRW(bar, "ct_toggle", TW_TYPE_BOOLCPP, &drawcelltarget, " label='  Draw cell target line' ");

	TwAddSeparator(bar, NULL, NULL);

	TwAddButton(bar, "jc_toggle", DoCellJitter, world, " label='Jitter cells' ");

	TwAddSeparator(bar, NULL, NULL);

	TwAddButton(bar, "todo", NULL, NULL, "label='To do:'");
	TwAddButton(bar, "iws_button", ResetWorld, world, " label='  Reset World'");
	TwAddButton(bar, "pl_toggle", NULL, NULL, "label='  Toggle patch layer'");
	TwAddButton(bar, "reset_colour_button", NULL, NULL, " label='  Reset cell colour' ");

}

GLView::GLView(World *s):
        world(world),
        paused(false),
        draw(true),
        skipdraw(1),
        drawfood(false),
        modcounter(0),
        frames(0),
        idlecalled(0),
        lastUpdate(0),
        mouseState(MOUSESTATE_DEFAULT),
		drawcelltarget(true),
		saveBackground(false)
{
	// The additional 1 element is due to a throw-back to the sample screenshot code (originally used to form the TGA header)
	// However, removing them causes the program to crash when saving the background image for the second time.
	// No idea why this happens.
	backgroundImage = (unsigned char *) calloc(1, (conf::HEIGHT*conf::WIDTH*4));
}

GLView::~GLView() {

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
    glOrtho(0,conf::WWIDTH,conf::WHEIGHT,0,0,1);

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

	//cout << "Mouse is at " << x << "," << y << "\n";

	if( !TwEventMouseButtonGLUT(button, state, x, y) ) { // event has not been handled by AntTweakBar
		// Left click action depends on the current mouseState

		// Place cell only on button down (state==0)
		if (mouseState == MOUSESTATE_PLACE_CELL && state == 0) {
			//cout << "In Mouse state " << mouseState << ": button went " << state << "\n";
			world->addCell(x, y);
		} else if (mouseState == MOUSESTATE_DEFAULT && state == 0) {
			// Tell world that a cell was clicked. World deals with the details of what this means.
			//cout << "[VIEW] About to call world->ssoc" << "\n";
			world->setSelectOnCell(x,y);
			//cout << "[VIEW] Returned from world->ssoc" << "\n";
		} else if (mouseState == MOUSESTATE_INFECT_CELL && state == 0) {
			// Clicking on a cell will toggle its infection status
			cout << "[] toggling infection at " << x << "," << y << "\n";
			world->setInfection(x,y);
		}

	}
}

// This is called if the mouse is move whilst EITHER button is down
void GLView::processMouseActiveMotion(int x, int y) {
	// Update internal mouse tracker
	mouseX = x; mouseY = y;
	// Try sending to Tw
	//printf("DEBUG: Mouse is at (%i,%i)\n", x, y);
	if ( !TwEventMouseMotionGLUT(x, y) ) {
		// Moving whilst left button is held down depends on the current mouseState
		if (mouseState == MOUSESTATE_DRAW_PATCH) {
			world->setPatch(x,y);
		} else if (mouseState == MOUSESTATE_PLACE_CELL) {
			drawCircle(x,y,10);
		}
	}
}

// This is called if the mouse is moved whilst no button is pressed.
void GLView::processMousePassiveMotion(int x, int y) {
	// Update internal mouse tracker
	mouseX = x; mouseY = y;
	// Try sending to Tw
	if ( !TwEventMouseMotionGLUT(x, y) ) {
		// If in states 2 (place cell) or 3 (make infected) or 0 (default) then check if mouse is currently above a cell
		if (mouseState == MOUSESTATE_DEFAULT || mouseState == MOUSESTATE_PLACE_CELL || mouseState == MOUSESTATE_INFECT_CELL) {
			// Let world deal with details of a mouse click on a cell
			world->setFocusOnCell(x,y);
		}
	}
}

void GLView::processNormalKeys(unsigned char key, int x, int y) {

    if (key == 27)
        exit(0);
    else if (key=='r') {
        world->reset();
        printf("Agents reset\n");
    } else if (key=='p') {
        //pause
        paused= !paused;
    } else if (key=='d') {
        //drawing
    	toggleDrawing();
    } else if (key==43) {
        //+
        skipdraw++;
    } else if (key==45) {
        //-
        skipdraw--;
    } else if (key=='f') {
        drawfood=!drawfood;
    } else if (key=='c') {
        world->setClosed( !world->isClosed() );
        printf("Environment closed now = %i\n",world->isClosed());
    } else {
        printf("Unknown key pressed: %i\n", key);
    }
}

// Everytime handleIdle is called, the world is updated, a wait MIGHT happen, and the scene MIGHT be rendered
void GLView::handleIdle() {
    modcounter++;
    idlecalled++;
    if (!paused) world->update();

    //show FPS
    int currentTime = glutGet( GLUT_ELAPSED_TIME );

    // Every 1000ms(==1s), update the titlebar and reset the frames (fps) variable
    if ((currentTime - lastUpdate) >= 1000) {
        std::pair<int,int> num_herbs_carns = world->numHerbCarnivores();
        sprintf( buf, "FPS: %d; CPS: %d; NumCells: %d; OLD:NumAgents: %d Carnivors: %d Herbivors: %d Epoch: %d", frames, idlecalled, world->numCells(), world->numAgents(), num_herbs_carns.second, num_herbs_carns.first, world->epoch() );
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
        if (skipdraw>0) {
            if (modcounter%skipdraw==0) renderScene();    //increase fps by skipping drawing
        }
        else renderScene(); //we will decrease fps by waiting using clocks
    } else {
    	// We render scene in almost every case, but this call won't draw the world, only the menu (I think).
    	renderScene();
    }

}


void GLView::renderScene() {
    frames++;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
    glPushMatrix();

    // Draw the world
    // If draw is off, then display the stored background picture instead
    if (draw) {
    	// Don't draw background scene unless it has changed drastically
    	if ( world->doBackgroundRefresh() ) {
    		// Draw the entire world, save to backgroundImage
    		// Reset first (must be an easier way to do this! Why doesn't glClear do this?
            glBegin(GL_QUADS);
            glColor3f(0.9,0.9,1.0);
            glVertex2f(0,0);
            glVertex2f(0,conf::HEIGHT);
            glVertex2f(conf::WIDTH,conf::HEIGHT);
            glVertex2f(conf::WIDTH,0);
            glEnd();
            // Draw background (patches and cells)
    		world->drawBackground(this, drawfood);
    		// Save new background
    		glReadPixels(0, 0, conf::WIDTH, conf::HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, backgroundImage+0);
    		// Add on the agents (and also the progress bar, if in docelljitter).
    		world->drawForeground(this);
    	} else {
    		// Draw the Background and then the Agents
        	glRasterPos2i(0,conf::HEIGHT);
        	glDrawPixels(conf::WIDTH, conf::HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, backgroundImage+0);
        	world->drawForeground(this);
    	}
    } else {
    	// Draw the contents of backgroundImage to the screen
    	glRasterPos2i(0,conf::HEIGHT);
    	glDrawPixels(conf::WIDTH, conf::HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, backgroundImage+0);
    }

    // Save background, if required. Note we do not save the menu to background.
    if (saveBackground) {
		//cout << "Pausing and storing the BG image \n";
		// Don't ask why backgroundImage is +1. Without it, the program crashes on the second call attempt.
		glReadPixels(0, 0, conf::WIDTH, conf::HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, backgroundImage+0);
    	saveBackground = false;
    	draw = false;
    }

	// Draw the menu
    TwDraw();

    glPopMatrix();

    glutSwapBuffers();
}

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

    //draw giving/receiving
    if(agent.dfood!=0){
        glBegin(GL_POLYGON);
        float mag=cap(abs(agent.dfood)/conf::FOODTRANSFER/3);
        if(agent.dfood>0) glColor3f(0,mag,0); //draw boost as green outline
        else glColor3f(mag,0,0);
        for (int k=0;k<17;k++){
            n = k*(M_PI/8);
            glVertex3f(agent.pos.x+rp*sin(n),agent.pos.y+rp*cos(n),0);
            n = (k+1)*(M_PI/8);
            glVertex3f(agent.pos.x+rp*sin(n),agent.pos.y+rp*cos(n),0);
        }
        glEnd();
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

    //draw giving/receiving
    if (agent.dfood!=0) {

        float mag=cap(abs(agent.dfood)/conf::FOODTRANSFER/3);
        if (agent.dfood>0) glColor3f(0,mag,0); //draw boost as green outline
        else glColor3f(mag,0,0);
        glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+36,0);
        glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+36,0);
        glVertex3f(agent.pos.x+xo+12,agent.pos.y+yo+46,0);
        glVertex3f(agent.pos.x+xo+6,agent.pos.y+yo+46,0);
    }


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


void GLView::drawCell(const Cell &cell) {
	// Useful variables
	float n;
	float r = cell.radius;
	float * cColor = world->getCellColourFromType(cell.cType);

	// 0. Draw a line from cell origin to the nearestPatch (i.e. the cell target)
	if ( drawcelltarget ) {
		glBegin(GL_LINES);
		glColor3f(0,1,0); // Green
		glVertex3f(cell.pos.x,cell.pos.y,0);
		glVertex3f(cell.nearestPatch.x,cell.nearestPatch.y,0);
		glEnd();
	}

	// 1. Draw the body (simple circle)
    glBegin(GL_POLYGON);
    //cout << "Cell colour: " << cColor[0] << ":"<< cColor[1] << ":"<< cColor[2] << "\n";
    //cout << "Cell (" << &cell << ") type is " << cell.cType << "\n";
    glColor3f(cColor[0],cColor[1],cColor[2]);
    drawCircle(cell.pos.x, cell.pos.y, r);
    glEnd();

    // 2. Draw a black line around the circle
    glBegin(GL_LINES);
    // If the cell is currently "in focus" then draw thicker, orange outline instead of a black thin line
    if ( cell.isSelected || cell.isInFocus ) {
    	glColor3f(0.925,0.596,0.145); // Orange outline
    	glLineWidth(2.0); // make it thick
    } else glColor3f(0,0,0); // Black outline
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
    if (1) {
    	sprintf(buf2, "%i", cell.cType);
    	RenderString(cell.pos.x-conf::BOTRADIUS*1.5, cell.pos.y+conf::BOTRADIUS*1.8+5, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);
    	//sprintf(buf2, "(%.0f,%.0f)", cell.pos.x, cell.pos.y);
    	//RenderString(cell.pos.x-conf::BOTRADIUS*1.5, cell.pos.y+conf::BOTRADIUS*1.8+15, GLUT_BITMAP_TIMES_ROMAN_24, buf2, 0.0f, 0.0f, 0.0f);
    }
}

void GLView::drawFood(int x, int y, float quantity) {
    //draw food
    if (drawfood) {
        glBegin(GL_QUADS);
        glColor3f(0.9-quantity,0.9-quantity,1.0-quantity);
        glVertex3f(x*conf::CZ,y*conf::CZ,0);
        glVertex3f(x*conf::CZ+conf::CZ,y*conf::CZ,0);
        glVertex3f(x*conf::CZ+conf::CZ,y*conf::CZ+conf::CZ,0);
        glVertex3f(x*conf::CZ,y*conf::CZ+conf::CZ,0);
        glEnd();
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


void GLView::toggleFoodLayer() {
	drawfood=!drawfood;
}

void GLView::toggleDrawing() {
	// When we toggle draw we have the following situations:
	// 1. Draw is currently on: toggle saveBackground first (this will toggle draw when it is hit)
	// 2. Draw is currently off: switch on draw.
	if (draw) {
		saveBackground = true;
	} else {
		world->setBackgroundRefresh(true);
		draw = true;
	}
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
