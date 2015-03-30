#ifndef WORLD_H
#define WORLD_H

#include "settings.h"
#include "helpers.h"
#include "vmath.h"

#include <vector>
#include <string>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace std;

/* Forward define what a View and Cell is */
class AbstractAgent;
class Cell;
class ImmunebotsSetup;
class View;

class World {

public:

    World(ImmunebotsSetup*);
    ~World();

    void update();
    void reset();

    void drawBackground(View* view, bool simulationmode);
    void drawForeground(View* view);

    bool isClosed() const;
    void setClosed(bool close);

	void setPatch(int x, int y);
	void setPatch(int index);

	/* CELL COLOURS */
	float COLOUR_NOT_SUSCEPTIBLE[3];
	float COLOUR_SUSCEPTIBLE[3];
	float COLOUR_INFECTED[3];
	float COLOUR_CTL[3];
	float COLOUR_DEAD[3];

	/* Other Conf settings */
	float SUSCEPTIBLE_PERCENTAGE;
	int DEFAULT_NUM_CELLS_TO_ADD;

	// World state is currently unused
    int WorldState;
    int numAgents() const;
    int numCells() const;
    float worldtime;

	float getWorldTime();

    void addCell(int x, int y);
    void addCTL(int x, int y);
    void addRandomCells(int num);
    void addAgent(AbstractAgent *);

    void resetShadowLayer();
    void setCellShadow(Cell *c);
    void resetCellShadow(Cell *c);
    void _setCellShadow(Cell *c, Cell * value);

    bool isOverCell(int x, int y); // TODO: Make private?
    Cell * getCell(int x, int y); // TODO: Make private?
    void toggleInfection(int x, int y);

    void resetCellColours();
    float * getCellColourFromType(int ct);

    void resetCellSusceptibility();
    void clearAllCells();

    /* Place cells on patches (previously called: dojitter) */
    void placeCells();

    /* Save information to file */
    void saveLayout();
    void loadLayout();

    // Bounding box
    Vector2<int> bounding_min;
    Vector2<int> bounding_max;

private:

    void setInputs(float timestep);
    void processOutputs(float timestep);

    void writeReport();

    void reproduce(int ai, float MR, float MR2);
    void addNewByCrossover();
    void addRandomBots(int num);

    ImmunebotsSetup * ibs;
    int modcounter;

    // This is the active agents vector
    std::vector<AbstractAgent*> agents;

    std::vector<Cell*> cells;

    void copyColourArray(float wc[], const float cc[]);

    // Cell * CellShadow[conf::WIDTH][conf::HEIGHT]; // Old C-style way
    vector< vector<Cell*> > CellShadow;

    /* PATCH */
    // If we use bools instead of ints, then 1bit will be used for each element, instead of 4 bytes!
    vector< vector<bool> > patch; 	// great for lookups
    vector<int> patchV;				// faster for loading/saving and drawing

    void resetBoundingBox();
    void checkBoundingBox(int x, int y);

    bool isNearPatch(int x, int y);

    bool CLOSED; // if environment is closed, then no random bots are added per time interval

	// For serialisation: We're only interested in the cells and patch vectors, and the bounding min/max.
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & cells;
        ar & patchV;
        ar & bounding_max;
        ar & bounding_min;
        ar & agents;
    }

};


#endif // WORLD_H
