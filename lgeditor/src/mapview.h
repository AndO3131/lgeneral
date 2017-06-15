/*
 * mapview.h
 *
 *  Created on: 03.03.2017
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

#ifndef MAPVIEW_H_
#define MAPVIEW_H_

class MapView : public Widget
{
	Data *data; /* connected game/map data */
	int vx, vy, vw, vh; /* viewport of map */
	int sx, sy; /* selected map tile (absolute coords) */

	void setTile(bool clear, bool onlyname);
	void getMapCoordsFromPointer(int px, int py, int &mx, int &my);
public:
	MapView(Widget *p, Geom g, Image *i, bool border, Data *d);
	void render();
	void scroll(int dx, int dy);

	virtual void handleEvent(const SDL_Event *ev);
};



#endif /* MAPVIEW_H_ */
