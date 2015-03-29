#ifndef WORLD_H
#define WORLD_H

#include "View.h"
#include "Agent.h"
#include "Cell.h"
#include "settings.h"
#include <vector>

class World {

public:
    World();
    ~World();

    void update();
    void reset();

    //void draw(View* view, bool drawfood, bool drawBackground);
    void drawBackground(View* view, bool drawfood);
    void drawForeground(View* view);

    bool isClosed() const;
    void setClosed(bool close);

	void setPatch(int x, int y);

	/* CELL COLOURS */
	float COLOUR_NOT_SUSCEPTIBLE[3];
	float COLOUR_SUSCEPTIBLE[3];
	float COLOUR_INFECTED[3];
	float COLOUR_CTL[3];
	float COLOUR_DEAD[3];

	/* Other Conf settings */
	float SUSCEPTIBLE_PERCENTAGE;
	int DEFAULT_NUM_CELLS_TO_ADD;

    /**
     * Returns the number of herbivores and
     * carnivores in the world.
     * first : num herbs
     * second : num carns
     */
    std::pair<int,int> numHerbCarnivores() const;

	// World state is currently unused
    int WorldState;
    int numAgents() const;
    int numCells() const;
    int epoch() const;

    void addCell(int x, int y);
    void addRandomCells(int num);

    void resetShadowLayer(Cell * c, Vector2f oldpos);
    void setCellShadow(Cell *c);
    void resetCellShadow(Cell *c);
    void _setCellShadow(Cell *c, Cell * value);

    bool isOverCell(int x, int y);
    Cell * getCell(int x, int y);
    void setSelectOnCell(int x, int y);
    void setFocusOnCell(int x,int y);
    void setInfection(int x, int y);

    Cell * selectedCell;
    Cell * inFocusCell;

    void resetCellColours();
    float * getCellColourFromType(int ct);

    void resetCellSusceptibility();
    void clearAllCells();

    /* JITTER */
    void startJitter();

    // If true, View will call drawBackground() and save the background
    bool doBackgroundRefresh();
    void setBackgroundRefresh(bool r);

private:
    void setInputs();
    void processOutputs();
    void brainsTick();  //takes in[] to out[] for every agent

    void writeReport();

    void reproduce(int ai, float MR, float MR2);
    void addNewByCrossover();
    void addRandomBots(int num);

    int modcounter;
    int current_epoch;
    int idcounter;

    std::vector<Agent> agents;

    std::vector<Cell*> cells;
    int cidcounter;

    void copyColourArray(float wc[], const float cc[]);

    // food
    int FW;
    int FH;
    int fx;
    int fy;
    float food[conf::WIDTH/conf::CZ][conf::HEIGHT/conf::CZ];
    Cell * CellShadow[conf::WIDTH][conf::HEIGHT];

    /* PATCH */
    int patch[conf::WIDTH][conf::HEIGHT];
    std::vector<int> patchVector;

    /* JITTER */
    bool docelljitter;
    int cellJitterIteration;

    bool dobackgroundrefresh;

    bool CLOSED; //if environment is closed, then no random bots are added per time interval
    float goc[conf::WIDTH][conf::HEIGHT]; // This is the Grid of Cells
};

#endif // WORLD_H
