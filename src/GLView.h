#ifndef GLVIEW_H
#define GLVIEW_H

#ifndef IMMUNEBOTS_NOGL
#include <GL/freeglut.h>
#endif

#include <vector>

#include "View.h"
#include "helpers.h"

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

#ifndef IMMUNEBOTS_NOGL
void gl_processNormalKeys(unsigned char key, int x, int y);
void gl_processSpecialKeys(int key, int x, int y);
void gl_processMouse(int button, int state, int x, int y);
void gl_processMouseActiveMotion(int x, int y);
void gl_processMousePassiveMotion(int x, int y);
void gl_changeSize(int w, int h);
void gl_handleIdle();
void gl_renderScene();
#endif

class GLView : public View {

public:
    GLView(World*, ImmunebotsSetup*);
    virtual ~GLView();

#ifndef IMMUNEBOTS_NOGL
    virtual void drawAgent(AbstractAgent*);
    virtual void drawCell(Cell*);
    virtual void drawDot(int x, int y, float z);
#endif

    void setWorld(World* w);
    void setCamera(float,float,float);

// GL functions
#ifndef IMMUNEBOTS_NOGL
    void setupDisplayLists(void);
#endif

// Tw Tweak functions
#ifndef IMMUNEBOTS_NOGL
    void createSetupMenu(bool visible);
    void createSimulationMenu(bool visible);
    void createStatsMenu(bool visible);
#endif

    // Returns the WorldSpace (Model) co-ords of the mouse position.
#ifndef IMMUNEBOTS_NOGL
    void getMouseWorldCoords(int x, int y, int z, GLdouble* ws);
#endif

// GL functions
#ifndef IMMUNEBOTS_NOGL
    void processNormalKeys(unsigned char key, int x, int y);
    void processSpecialKeys(int key, int x, int y);
    void processMouse(int button, int state, int x, int y);
    void processMouseActiveMotion(int x, int y);
    void processMousePassiveMotion(int x, int y);
    void changeSize(int w, int h);
    void renderScene();
#endif

    // The following is called by OpenGL but doesn't do any drawing
    // It *does* update the world! Very important function.
    void handleIdle();

// Control the state of the program (setup <-> simulation)
#ifndef IMMUNEBOTS_NOGL
    void switchToSimulationMode(bool dosim);
    int  getMouseState();
    void setMouseState(int);
#endif

    void toggleDrawing();
    void togglePaused();
    void toggleStepMode(bool);
    void setPaused(bool);

    void checkSetup();

// GL drawing functions
#ifndef IMMUNEBOTS_NOGL
    void drawCircle(float x, float y, float z, float r, bool);
    void RenderString(float x, float y, void *font, const char* string, float r, float g, float b);
    /* TEST FUNCTIONS*/
    void drawProgressBar(float completed);
#endif

	float currentWidth;
	float currentHeight;

    bool paused;
    int idlecalled;

private:

    // Compiler needs to know the size of structs, so full definitions go here.
    typedef struct {   /* Where da camera? */
		double x, y, z;   /* 3-D coordinates (we don't need w here) */
		float roll, pitch, heading;
		float zoom;
    } Camera;

    World *world;
    ImmunebotsSetup *ibs; // Our config/setup object
    Camera *cam;
    Statistics *stats;

    bool dosimulation; // Starts off as false, when true THEN paused is enabled
    bool draw;
    int skipdraw;
    bool stepmode, doanotherstep;
    bool sync_framerate;
    char buf[100];
    char buf2[10];
    int modcounter;
    int frames;
    int lastUpdate;
    int mouseState;
	bool processsetupevents;
	int fastforwardtime;

	/* Pan & mouse position info */
	bool mousedownnomove;
	int lastmousey; // Need to know the last place the mouse was panned from
	int lastmousex;
	float scale;

	/* Screenshot variables */
	int screenshot;
	char screenshotfilename[100];

	/* Vertex lists (enabled if conf::USE_VERTEX_ARRAY */
	int cell_list, cell_outline_list;

	void automaticEventSetup();
};

#endif // GLVIEW_H
