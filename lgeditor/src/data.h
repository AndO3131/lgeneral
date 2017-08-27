/*
 * data.h
 *
 *  Created on: 28.02.2017
 *      Copyright 2017 Michael Speck
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DATA_H_
#define DATA_H_

typedef struct {
	std::string id;
	std::string name;
	int minEntr;
	GridImage *tiles;
} TerrainInfo;
typedef struct {
	std::string id;
	std::string name;
} WeatherInfo;
typedef struct {
	std::string id;
	std::string name;
	Image *icon;
} NationInfo;
typedef struct {
	std::string id;
	std::string name;
} UnitClassInfo;
typedef struct {
	std::string id;
	std::string name;
	int cid; /* class id */
	int nid; /* nation id */
	Image *icon;
} UnitInfo;
enum {
	GTRSP = 15,
	ATRSP,
	STRSP
};
typedef struct {
	std::string id;
	int libidx; /* index in unit lib */
	std::string nat;
	int core;
	int x, y;
	int str, entr, exp;
	std::string trsp;
	int trsptype;
	int delay;
} Unit;
typedef struct {
	int tid[2]; /* terrain type and pic id */
	Unit gunit; /* ground unit */
	Unit aunit; /* air unit */
	int fid; /* flag id */
	int obj; /* objective */
	std::string name; /* tile name */
} MapTile;

#define MAXMAPW 200
#define MAXMAPH 200

/** Container for LGeneral data */
class Data
{
public:
	int hexw, hexh;
	int hexxoff, hexyoff;
	Image *grid;
	Image *selectFrame;

	std::vector<TerrainInfo> terrain;
	std::vector<WeatherInfo> weather;
	std::vector<NationInfo> countries;
	std::vector<UnitClassInfo> uclasses;
	std::vector<UnitInfo> unitlib;
	std::vector<Unit> reinfs;

	int mapw, maph;
	MapTile map[MAXMAPW][MAXMAPH];
	PData *scen; /* scenario as unprocessed parser data */
	bool addDefaultScenData;

	Data(int w, int h);
	~Data();

	void resetMap(int w, int h);
	void loadMap(std::string fname);
	void saveMap(std::string fname);
	void saveMapInfile(FILE *fh);
	void loadScenario(std::string fname);
	void saveScenario(std::string fname, PData *mpd);
	void saveUnit(Unit &u, FILE *fh);
	void writeDefaultScenData(FILE *fh);

	int countUnitsInClass(int cid);
	int getUnitByIndex(int cid, int uid);
	int getUnitById(std::string id);
};

#endif /* DATA_H_ */
