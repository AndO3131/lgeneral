/*
 * mapview.cpp
 *
 *  Created on: 03.03.2017
 *     Copyright 2017 Michael Speck
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <list>
#include <string>
#include <vector>
#include <sstream>

#include "widgets.h"
#include "parser.h"
#include "data.h"
#include "mapview.h"
#include "gui.h"

extern SDL_Renderer *mrc;
extern GUI *gui;

MapView::MapView(Widget *p, Geom g, Image *i, bool border, Data *d)
	:Widget(p,g,i,border), data(d), vx(0), vy(0), sx(-1), sy(-1)
{
	vw = g.width() / data->hexxoff + 2;
	vh = g.height() / data->hexh + 2;
}

/** Render current map into bkgnd and draw */
void MapView::render()
{
	int sdx = data->hexxoff - data->hexw ;
	int sdy = data->hexyoff - data->hexh;
	int basey = sdy;
	int dx = sdx;
	int dy = sdy;
	int trspid = -1;

	SDL_SetRenderTarget(mrc,bkgnd->getTex());
	SDL_SetRenderDrawColor(mrc,0,0,0,0);
	SDL_RenderClear(mrc);
	for (int j = vy; j < vy + vh; j++) {
		for (int i = vx; i < vx + vw; i++) {
			if (i >= data->mapw || j >= data->maph)
				continue;

			MapTile *t = &data->map[i][j];
			int tw = data->terrain[t->tid[0]].tiles->getGridWidth();
			int th = data->terrain[t->tid[0]].tiles->getGridHeight();

			/* render map tile */
			data->terrain[t->tid[0]].tiles->copy(t->tid[1],0,dx,dy);
			data->grid->copy(dx,dy);
			if (i == sx && j == sy)
				data->selectFrame->copy(dx,dy);
			/* add flag if any */
			if (t->fid != -1) {
				Image *i =data->countries[t->fid].icon;
				int fw = i->getWidth();
				int fh = i->getHeight();
				int fx = dx + (tw - fw)/2;
				int fy = dy + th - fh - 2;
				i->copy(fx, fy);
				if (t->obj) {
					SDL_Point pts[5] = {
						{fx-1,fy-1},
						{fx+fw,fy-1},
						{fx+fw,fy+fh},
						{fx-1,fy+fh},
						{fx-1,fy-1}
					};
					SDL_SetRenderDrawColor(mrc,
							255, 255, 0, SDL_ALPHA_OPAQUE);
					SDL_RenderDrawLines(mrc, pts, 5);
				}
			}
			/* add units */
			if (t->gunit.id != "") {
				UnitInfo &ui = data->unitlib[t->gunit.libidx];
				int ux = (tw - ui.icon->getWidth()) / 2;
				int uy = (th - ui.icon->getHeight()) / 2 + th/4;
				ui.icon->copy(dx + ux, dy + uy);
			}
			/* transporter */
			if (t->gunit.id != "" && t->gunit.trsp != "none" &&
					(trspid = data->getUnitById(t->gunit.trsp)) != -1) {
				UnitInfo &ui = data->unitlib[trspid];
				int uw = ui.icon->getWidth() *0.75;
				int uh = ui.icon->getHeight() *0.75;
				int ux = (tw - uw) / 2;
				int uy = (th - uh);
				ui.icon->copy(dx + ux, dy + uy, uw, uh);

			}
			if (t->aunit.id != "") {
				UnitInfo &ui = data->unitlib[t->aunit.libidx];
				int ux = (tw - ui.icon->getWidth()) / 2;
				int uy = (th - ui.icon->getHeight()) / 2 - th/4;
				ui.icon->copy(dx + ux, dy + uy);
			}

			/* adjust draw position */
			dx += data->hexxoff;
			if (i % 2 == 0)
				dy += data->hexyoff;
			else
				dy -= data->hexyoff;
		}
		dx = sdx;
		basey += data->hexh;
		dy = basey;
	}
      	SDL_SetRenderTarget(mrc,NULL);

	draw();
}

/** Convert mouse pointer coords (relative in widget) into map coords. */
void MapView::getMapCoordsFromPointer(int px, int py, int &mx, int &my)
{
	int sdx = data->hexw - data->hexxoff;
	int sdy = data->hexh - data->hexyoff;

	mx = px / data->hexxoff;
	if (mx % 2 == 0)
		my = (py + data->hexyoff) / data->hexh;
	else
		my = (py) / data->hexh;

	int rhx = px + sdx - mx*data->hexxoff;
	int rhy;
	if (mx % 2 == 0)
		rhy = py + sdy - my*data->hexh;
	else
		rhy = py + sdy - data->hexyoff - my*data->hexh;

	if (rhx > (data->hexxoff+2)) {
		if (rhy / (rhx - (data->hexxoff+2)) < 1.6) {
			mx++;
			if (mx % 2 != 0)
				my--;
		}
		if ((data->hexh - rhy) / (rhx - (data->hexxoff+2)) < 1.6) {
			mx++;
			if (mx % 2 == 0)
				my++;
		}
	}
}

void MapView::setTile(bool clear, bool onlyname)
{
	if (sx == -1 || sy == -1)
		return; /* no selection */

	int cat, sub,item;
	MapTile &tile = data->map[sx][sy];
	gui->getSelection(cat,sub,item);

	if (cat == ID_TERRAINITEMS) {
		if (clear) {
			tile.tid[0] = 0;
			tile.tid[1] = rand() % 6 + 12;
			tile.name = data->terrain[0].name;
		} else if (sub != -1 && item != -1 && !onlyname) {
			tile.tid[0] = sub;
			tile.tid[1] = item;
			tile.name = gui->tedit->getText();
		} else if (sub != -1 && onlyname) {
			tile.name = gui->tedit->getText();
		}
	} else if (cat == ID_NATIONITEMS) {
		if (clear)
			tile.fid = -1;
		else if (sub != -1 && item != -1) {
			tile.fid = sub;
			tile.obj = (item == 1);
		}
	} else if (cat == ID_UNITITEMS) {
		if (clear) {
			tile.gunit.id = "";
			tile.gunit.trsp = "";
			tile.aunit.id = "";
		} else {
			int uid = data->getUnitByIndex(sub,item);
			Unit *u;
			if (sub == GTRSP || sub == ATRSP || sub == STRSP) {
				/* transporter */
				if (tile.gunit.id != "") {
					tile.gunit.trsp = data->unitlib[uid].id;
					tile.gunit.trsptype = sub;
					if (sub == ATRSP)
						tile.aunit.id = ""; /* block air */
				}
			} else {
				/* FIXME class 8,9,10 are flying... correct solution
				 * would be to check for class flying flag */
				if (sub >= 8 && sub <= 10)
					u = &tile.aunit;
				else
					u = &tile.gunit;
				u->libidx = uid;
				u->id = data->unitlib[uid].id;
				u->nat = data->countries[data->unitlib[uid].nid].id;
				u->str = 10;
				u->core = 0;
				u->entr = data->terrain[data->map[sx][sy].tid[0]].minEntr;
				u->exp = 0;
				u->x = sx;
				u->y = sy;
				u->trsp = "none";
			}
		}
	}
}

void MapView::scroll(int dx, int dy)
{
	int oldvx = vx, oldvy = vy;
	vx += dx;
	vy += dy;
	if (vy >= data->maph - vh)
		vy = data->maph - vh;
	if (vx >= data->mapw - vw)
		vx = data->mapw - vw;
	if (vx < 0)
		vx = 0;
	if (vy < 0)
		vy = 0;
	if (oldvy != vy || oldvx != vx)
		render();
}

void MapView::handleEvent(const SDL_Event *ev)
{
	bool changed = false;

	checkFocus(ev);
	if (!hasFocus) {
		sx = -1;
		sy = -1;
		changed = true;
	} else if (ev->type == SDL_MOUSEMOTION) {
		int mx, my;
		getMapCoordsFromPointer(
				ev->motion.x - ax, ev->motion.y - ay, mx, my);
		sx = vx + mx;
		if (sx >= data->mapw)
			sx = data->mapw - 1;
		sy = vy + my;
		if (sy >= data->maph)
			sy = data->maph - 1;

		if (ev->motion.state & SDL_BUTTON_LMASK)
			setTile(false,false);
		else if (ev->motion.state & SDL_BUTTON_RMASK)
			setTile(true,false);
		else if (ev->motion.state & SDL_BUTTON_MMASK)
			setTile(false,true);
		changed = true;
	} else if (ev->type == SDL_MOUSEBUTTONDOWN) {
		if (ev->button.button == SDL_BUTTON_LEFT)
			setTile(false,false);
		else if (ev->button.button == SDL_BUTTON_MIDDLE)
			setTile(false,true);
		else
			setTile(true,false);
		changed = true;
	} else if (ev->type == SDL_KEYDOWN) {
		/* do nothing scrolling is done in main event loop */
	}

	if (changed) {
		render();

		if (sx != -1 && sy != -1) {
			MapTile &t =data->map[sx][sy];
			std::ostringstream txt;
			txt << t.name << " (" << sx << ", " << sy << ")";
			if (t.gunit.id != "")
				txt << "  G: " << data->unitlib[t.gunit.libidx].name;
			if (t.gunit.trsp != "none") {
				int tid = data->getUnitById(t.gunit.trsp);
				if (tid != -1)
					txt << " [" << data->unitlib[tid].name << "] ";
			}

			if (t.aunit.id != "")
				txt << "  A: " << data->unitlib[t.aunit.libidx].name;
			tooltipLabel->setText(txt.str().c_str());
		}
	}
}
