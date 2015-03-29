#ifndef VIEW_H
#define VIEW_H

//#include "Cell.h"

class AbstractAgent;
class Cell;

class View {

public:

    virtual void drawAgent(AbstractAgent *) = 0;
    virtual void drawCell(const Cell &cell) = 0;
    virtual void drawDot(int x, int y) = 0;

    virtual void drawProgressBar(float completed) = 0;

};

#endif // VIEW_H
