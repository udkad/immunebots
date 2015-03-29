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
#include "World.h"
#include <AntTweakBar.h>
#include <ctime>
#include "config.h"
#include <iostream>

#include <stdio.h>

using namespace std;


GLView* GLVIEW = new GLView(0);

int main(int argc, char **argv) {

	cout << "Size of a _GLubyte_: " << sizeof(GLubyte) << "\n";
	cout << "Size of an _unsigned_ byte: " << sizeof(unsigned) << "\n";
	cout << "Size of an _unsigned char_: " << sizeof(unsigned char) << "\n";

    srand(time(0));
    if (conf::WIDTH%conf::CZ!=0 || conf::HEIGHT%conf::CZ!=0) printf("CAREFUL! The cell size variable conf::CZ should divide evenly into  both conf::WIDTH and conf::HEIGHT! It doesn't right now!");
    printf("p= pause, d= toggle drawing (for faster computation), f= draw food too, += faster, -= slower\n");

    World* world = new World();
    GLVIEW->setWorld(world);

    //GLUT SETUP
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);// GLUT_DEPTH
    glutInitWindowPosition(30,30);
    glutInitWindowSize(conf::WWIDTH,conf::WHEIGHT);
    glutCreateWindow("Immunebots");
    glewInit();

    // Create the Tw menu
    GLVIEW->addMenu();

    // Continue with rest of the GL initialisation
    glClearColor(0.9f, 0.9f, 1.0f, 0.0f);
    glutDisplayFunc(gl_renderScene);
    glutIdleFunc(gl_handleIdle);
    glutReshapeFunc(gl_changeSize);


    // directly redirect GLUT events to AntTweakBar
    //glutMouseFunc((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
    //glutMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);

    //glutKeyboardFunc((GLUTkeyboardfun)TwEventKeyboardGLUT);
    glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
    // send the ''glutGetModifers'' function pointer to AntTweakBar
    TwGLUTModifiersFunc(glutGetModifiers);

    glutKeyboardFunc(gl_processNormalKeys);
    glutMouseFunc(gl_processMouse);
    glutMotionFunc(gl_processMouseActiveMotion);
    glutPassiveMotionFunc(gl_processMousePassiveMotion);

    glutMainLoop();

    TwTerminate();
    return 0;
}


