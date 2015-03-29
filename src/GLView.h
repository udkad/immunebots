#ifndef GLVIEW_H
#define GLVIEW_H

#include <GL/glut.h>

#include "View.h"

/* Set of static const values to determine what the LEFT_CLICK does */
const static int MOUSESTATE_DEFAULT     = 0;
const static int MOUSESTATE_DRAW_PATCH  = 1;
const static int MOUSESTATE_PLACE_CELL  = 2;
const static int MOUSESTATE_INFECT_CELL = 3;

class AbstractAgent;
class GLView;
class ImmunebotsSetup;
class World;

extern GLView* GLVIEW;

void gl_processNormalKeys(unsigned char key, int x, int y);
void gl_processSpecialKeys(int key, int x, int y);
void gl_processMouse(int button, int state, int x, int y);
void gl_processMouseActiveMotion(int x, int y);
void gl_processMousePassiveMotion(int x, int y);
void gl_changeSize(int w, int h);
void gl_handleIdle();
void gl_renderScene();

class GLView : public View {

public:
    GLView(World*, ImmunebotsSetup*);
    virtual ~GLView();

    virtual void drawAgent(AbstractAgent*);
    virtual void drawCell(const Cell &cell);
    virtual void drawDot(int x, int y);

    void setWorld(World* w);

    // Tw Tweak functions
    void createSetupMenu(bool visible);
    void createSimulationMenu(bool visible);

    //GLUT functions
    void processNormalKeys(unsigned char key, int x, int y);
    void processSpecialKeys(int key, int x, int y);
    void processMouse(int button, int state, int x, int y);
    void processMouseActiveMotion(int x, int y);
    void processMousePassiveMotion(int x, int y);
    void changeSize(int w, int h);
    void handleIdle();
    void renderScene();

    // Control the state of the program (setup <-> simulation)
    void switchToSimulationMode(bool dosim);

    int  getMouseState();
    void setMouseState(int v);

    void toggleDrawing();
    void togglePaused();

    // Drawing functions
    void drawCircle(float x, float y, float r);
    void RenderString(float x, float y, void *font, const char* string, float r, float g, float b);

    /* TEST FUNCTIONS*/
    void drawProgressBar(float completed);

private:

    World *world;
    ImmunebotsSetup *ibs; // Our config/setup object

    bool dosimulation; // Starts off as false, when true THEN paused is enabled
    bool paused;
    bool draw;
    int skipdraw;
    char buf[100];
    char buf2[10];
    int modcounter;
    int frames;
    int idlecalled;
    int lastUpdate;
    int mouseState;
	bool processsetupevents;
	int fastforwardtime;

	/* Pan & mouse position info */
	float xoffset;	// Determine if we have panned left/right (def is: 0)
	float yoffset;
	bool mousedownnomove;
	int lastmousey; // Need to know the last place the mouse was panned from
	int lastmousex;
	float scale;

	void automaticEventSetup();
};

#endif // GLVIEW_H
