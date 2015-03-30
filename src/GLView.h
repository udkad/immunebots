#ifndef GLVIEW_H
#define GLVIEW_H

#include <GL/glut.h>
#include <vector>

#include "View.h"

/* Set of static const values to determine what the LEFT_CLICK does */
const static int MOUSESTATE_DEFAULT     = 0;
const static int MOUSESTATE_DRAW_PATCH  = 1;
const static int MOUSESTATE_PLACE_CELL  = 2;
const static int MOUSESTATE_INFECT_CELL = 3;
const static int MOUSESTATE_PLACE_CTL   = 4;

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
    virtual void drawCell(Cell*);
    virtual void drawDot(int x, int y, float z);

    void setWorld(World* w);
    void setCamera(float,float,float);

    // Tw Tweak functions
    void createSetupMenu(bool visible);
    void createSimulationMenu(bool visible);

    // Returns the WorldSpace (Model) co-ords of the mouse position.
    void getMouseWorldCoords(int x, int y, int z, GLdouble* ws);

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
    void toggleStepMode(bool);
    void setPaused(bool p);

    // Drawing functions
    void drawCircle(float x, float y, float z, float r);
    void RenderString(float x, float y, void *font, const char* string, float r, float g, float b);

    /* TEST FUNCTIONS*/
    void drawProgressBar(float completed);

	float currentWidth;
	float currentHeight;

private:

    // Compiler needs to know the size of the Camera struct, so full definition goes here.
    typedef struct {   /* Where da camera? */
		double x, y, z;   /* 3-D coordinates (we don't need w here) */
		float roll, pitch, heading;
		float zoom;
    } Camera;

    World *world;
    ImmunebotsSetup *ibs; // Our config/setup object
    Camera *cam;

    bool dosimulation; // Starts off as false, when true THEN paused is enabled
    bool paused;
    bool draw;
    int skipdraw;
    bool stepmode, doanotherstep;
    bool sync_framerate;
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
	bool mousedownnomove;
	int lastmousey; // Need to know the last place the mouse was panned from
	int lastmousex;
	float scale;

	void automaticEventSetup();
};

#endif // GLVIEW_H
