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
	GridImage *icons;
} UnitInfo;
typedef struct {
	int tid[2]; /* terrain type and pic id */
	int gid; /* ground unit */
	int aid; /* air unit */
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
	std::vector<UnitInfo> units; /* lib entries */

	int mapw, maph;
	MapTile map[MAXMAPW][MAXMAPH];

	Data(int w, int h);
	~Data();

	void loadMap(std::string fname);
	void saveMap(std::string fname);

	int countUnitsInClass(int cid);
	int getUnitByIndex(int cid, int uid);
};

#endif /* DATA_H_ */
