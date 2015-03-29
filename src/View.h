#ifndef VIEW_H
#define VIEW_H

#include "Agent.h"
#include "Cell.h"
class View {

public:

    virtual void drawAgent(const Agent &a) = 0;
    virtual void drawCell(const Cell &cell) = 0;
    virtual void drawFood(int x, int y, float quantity) = 0;
    virtual void drawDot(int x, int y) = 0;

    Cell * cellSelected;
    Cell * cellInFocus;

    virtual void drawProgressBar(float completed) = 0;

};

#endif // VIEW_H
