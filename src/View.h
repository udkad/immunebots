#ifndef VIEW_H
#define VIEW_H

//#include "Cell.h"

class AbstractAgent;
class Cell;

class View {

public:

    virtual void drawAgent(AbstractAgent *) = 0;
    virtual void drawCell(Cell *) = 0;
    virtual void drawDot(int x, int y, float z) = 0;

    virtual void drawProgressBar(float completed) = 0;

};

#endif // VIEW_H
