#ifndef WORLD_H
#define WORLD_H

#include "settings.h"
#include "helpers.h"
#include "vmath.h"

#include <vector>
#include <list>
#include <string>

#ifndef IMMUNEBOTS_NOSERIALISATION
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#endif

using namespace std;

/* Forward define what a View and Cell is */
class AbstractAgent;
class Cell;
class ImmunebotsSetup;
class View;

class World {

public:

	// Events which are triggered by agents
	const static int EVENT_INFECTEDDEATH_LYSIS 	= 1;
	const static int EVENT_INFECTEDDEATH_VIRUS 	= 2;
	const static int EVENT_FAILEDINFECTION	   	= 3;
	const static int EVENT_CTL_SCANCOMPLETE		= 4;
	const static int EVENT_INFECTED_ID			= 5;

	// Events specified in the configfile which take place at a future time..
	const static int EVENTSLIST_ADDETRATIO  = 1;
	const static int EVENTSLIST_ADDABSOLUTE = 2;
	const static int EVENTSLIST_ADDWHEN     = 3;
	const static int EVENTSLIST_ENDAFTER    = 4;
	const static int EVENTSLIST_ENDTIME		= 5;

    //World();
    World(ImmunebotsSetup*);
    ~World();
    ImmunebotsSetup * ibs;

    void update();
    void reset();

    void drawBackground(View* view, bool simulationmode);
    void drawForeground(View* view);

	void setPatch(int x, int y);
	void setPatch(int index);

	/* CELL COLOURS */
	float COLOUR_NOT_SUSCEPTIBLE[3];
	float COLOUR_SUSCEPTIBLE[3];
	float COLOUR_INFECTED[3];
	float COLOUR_CTL[3];
	float COLOUR_DEAD[3];

	/* Other Conf settings */
	float CTL_DENSITY;
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

    void setWorldDimensions(int, int);
    void resetShadowLayer();
    void setCellShadow(Cell *c);
    void resetCellShadow(Cell *c);
    void _setCellShadow(Cell *c, Cell * value);

    bool isOverCell(int x, int y); // TODO: Make private?
    Cell * getCell(int x, int y);
    Cell * getNearestInfectedCell(int x, int y);
    Vector2f getStrongestInfectedPull( int x0, int y0 );
    void toggleInfection(int x, int y);
    void infectCells(int total, bool onlySusceptible);

    void resetCellColours();
    float * getCellColourFromType(int ct);

    void resetCellSusceptibility();
    void clearAllCells();

    /* Place cells on patches (previously called: dojitter) */
    void placeCells();
    /* Place CTL at random inside the bounding box */
    void dropCTL();
    void dropCTL(int);

    /* Adding events to the EventsList */
    void addEvent(Event);

    /* Iterates through the right vector cells/agents and return the current number of the desired AbstractAgent */
    int getCurrentPopulation(int);

    /* LEGACY: Save information to file */
    void saveLayout();
    void loadLayout();

    // Bounding box
    Vector2<int> bounding_min;
    Vector2<int> bounding_max;

    Statistics * stats; // Statistics is defined in helpers.h (weird location, but widely shared)
    void updateStatsFull();
    void updateStats(bool);
    void writeReport(bool);

    void EventReporter(int);

private:

	int UID;

    void setInputs(float timestep);
    void processOutputs(float timestep);

    //void reproduce(int ai, float MR, float MR2);
    //void addNewByCrossover();
    void addRandomBots(int num);

    // This is the active agents vector
    std::vector<AbstractAgent*> agents;

    std::vector<Cell*> cells;

    // The Event struct is defined in helpers.h. I know, it's stupid, but follows the Statistics struct predecent.
    std::list<Event> EventsList;
    int  checkEvents(int);
    void activateEvent(Event*);

    void copyColourArray(float wc[], const float cc[]);

    // Cell * CellShadow[conf::WIDTH][conf::HEIGHT]; // Old C-style way
    vector< vector<Cell*> > CellShadow;
    //bool * CellShadowBool;
    vector< vector<bool> > CellShadowBool;


    /* PATCH */
    // If we use bools instead of ints, then 1bit will be used for each element, instead of 4 bytes!
    vector< vector<bool> > patch; 	// great for lookups
    vector<int> patchV;				// faster for loading/saving and drawing

    void resetBoundingBox();
    void checkBoundingBox(int x, int y);

    bool isNearPatch(int x, int y);

#ifndef IMMUNEBOTS_NOSERIALISATION
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
#endif

};


#endif // WORLD_H
