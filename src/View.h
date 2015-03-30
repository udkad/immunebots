#ifndef VIEW_H
#define VIEW_H

class AbstractAgent;
class Cell;

class View {

public:

#ifndef IMMUNEBOTS_NOGL
    virtual void drawAgent(AbstractAgent *) = 0;
    virtual void drawCell(Cell *) = 0;
    virtual void drawDot(int x, int y, float z) = 0;
    virtual void drawProgressBar(float completed) = 0;
#endif

};

#endif // VIEW_H
