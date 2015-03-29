#ifndef GLVIEW_H
#define GLVIEW_H

#include <GL/glut.h>


#include "View.h"
#include "World.h"

/* Set of static const values to determine what the LEFT_CLICK does */
const static int MOUSESTATE_DEFAULT     = 0;
const static int MOUSESTATE_DRAW_PATCH  = 1;
const static int MOUSESTATE_PLACE_CELL  = 2;
const static int MOUSESTATE_INFECT_CELL = 3;


class GLView;

extern GLView* GLVIEW;

void gl_processNormalKeys(unsigned char key, int x, int y);
void gl_processMouse(int button, int state, int x, int y);
void gl_processMouseActiveMotion(int x, int y);
void gl_processMousePassiveMotion(int x, int y);
void gl_changeSize(int w, int h);
void gl_handleIdle();
void gl_renderScene();

class GLView : public View {

public:
    GLView(World* w);
    virtual ~GLView();

    virtual void drawAgent(const Agent &a);
    virtual void drawCell(const Cell &cell);
    virtual void drawFood(int x, int y, float quantity);
    virtual void drawDot(int x, int y);

    void setWorld(World* w);
    void addMenu();

    //GLUT functions
    void processNormalKeys(unsigned char key, int x, int y);
    void processMouse(int button, int state, int x, int y);
    void processMouseActiveMotion(int x, int y);
    void processMousePassiveMotion(int x, int y);
    void changeSize(int w, int h);
    void handleIdle();
    void renderScene();

    int  getMouseState();
    void setMouseState(int v);

    void toggleDrawing();
    void toggleFoodLayer();

    /* TEST FUNCTIONS*/
    void drawProgressBar(float completed);

    // Background image store
    unsigned char * backgroundImage;

private:

    World *world;
    bool paused;
    bool draw;
    int skipdraw;
    bool drawfood;
    bool drawcelltarget;
    char buf[100];
    char buf2[10];
    int modcounter;
    int lastUpdate;
    int frames;
    int idlecalled;
    int mouseState;
	int mouseX;
	int mouseY;
	bool saveBackground;
};

#endif // GLVIEW_H
