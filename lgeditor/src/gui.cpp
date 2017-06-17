/*
 * gui.cpp
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

#include "widgets.h"
#include "parser.h"
#include "data.h"
#include "mapview.h"
#include "gui.h"

extern SDL_Renderer *mrc;
extern std::string rcpath;
extern std::string lgenpath;
extern Data *data;
extern GUI *gui;
bool GUI::leave = false;

void renderCell(int id, SDL_Texture *t, int w, int h)
{
	int sel = gui->list->getSelId();

	SDL_SetRenderTarget(mrc,t);
	SDL_SetRenderDrawColor(mrc,0,0,0,128);
	SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(mrc,NULL);

	if (sel != -1) {
		if (gui->curItemView == ID_TERRAINITEMS)
			data->terrain[sel].tiles->copy(id,0,0,0,w,h);
		else if (gui->curItemView == ID_NATIONITEMS) {
			if (id == 0) /* normal flag */
				data->countries[sel].icon->copy(0,0,w/2,h/2);
			else /* victory hex */
				data->countries[sel].icon->copy(0,0,w,h);
		} else if (gui->curItemView == ID_UNITITEMS) {
			int uid = data->getUnitByIndex(sel,id);
			UnitInfo &ui = data->unitlib[uid];
			ui.icon->copy(0,0,w,h);
			data->countries[ui.nid].icon->copy(0,0);
		}
	}

	SDL_SetRenderTarget(mrc,NULL);
}

void handleAction(int id, Widget *w, const SDL_Event *e)
{
	std::vector<std::string> v;

	switch(id) {
	case ID_QUIT:
		GUI::leave = true;
		break;
	case ID_TERRAINITEMS:
		gui->curItemView = ID_TERRAINITEMS;
		for (unsigned int i = 0; i < data->terrain.size(); i++)
			v.push_back(data->terrain[i].name);
		gui->list->setItems(v);
		gui->iv->setSize(0);
		break;
	case ID_NATIONITEMS:
		gui->curItemView = ID_NATIONITEMS;
		for (unsigned int i = 0; i < data->countries.size(); i++)
			v.push_back(data->countries[i].name);
		gui->list->setItems(v);
		gui->iv->setSize(0);
		break;
	case ID_UNITITEMS:
		gui->curItemView = ID_UNITITEMS;
		for (unsigned int i = 0; i < data->uclasses.size(); i++)
			v.push_back(data->uclasses[i].name);
		gui->list->setItems(v);
		gui->iv->setSize(0);
		break;
	case ID_LISTSELECT:
		if (gui->list->getSelId() != -1) {
			if (gui->curItemView == ID_TERRAINITEMS) {
				gui->iv->setSize(data->terrain[gui->list->getSelId()].tiles->getGridSize());
				gui->tedit->setText(data->terrain[gui->list->getSelId()].name);
				gui->tedit->draw();
			}
			else if (gui->curItemView == ID_NATIONITEMS)
				gui->iv->setSize(2);
			else if (gui->curItemView == ID_UNITITEMS)
				gui->iv->setSize(data->countUnitsInClass(gui->list->getSelId()));
		}
		break;
	case ID_ITEMSELECT:
		if (gui->curItemView == ID_UNITITEMS) {
			int uid = data->getUnitByIndex(gui->list->getSelId(),gui->iv->getSelId());
			gui->tedit->setText(data->unitlib[uid].name);
			gui->tedit->draw();
		}
		break;
	case ID_SAVE:
		data->saveScenario(gui->nedit->getText(),NULL);
		gui->mv->render();
		break;
	case ID_LOAD:
		data->loadScenario(gui->nedit->getText());
		gui->mv->render();
		break;
	}
}

GUI::GUI()
{
	int sbw = Geom::rwidth(0.02);
	Widget *w;

	bk = new Image(rcpath + "/metal.png");
	bk2 = new Image(rcpath + "/buttonback.png");
	icons = new GridImage(rcpath + "/buttons.png", 48, 48);
	font = new Font(rcpath + "/osb.ttf",Geom::rheight(0.02));

	w1 = new Widget(NULL, Geom(0, 0, -1, -1), bk, false);
	tooltip = new Label(w1, Geom((float)0.23,0.02,0.7,0.03),bk,true,font,
			((std::string)("LGeneral Location: " + lgenpath)).c_str());
	w = new Button(w1, Geom((float)0.02, 0.02, 0.06, 0.03),
			bk2, true, font, "Terrain");
	w->setTooltip(tooltip,"Select Terrain List");
	w->setActionHandler(handleAction,ID_TERRAINITEMS);
	w = new Button(w1, Geom((float)0.09, 0.02, 0.06, 0.03),
			bk2, true, font, "Flags");
	w->setTooltip(tooltip,"Select Flag List");
	w->setActionHandler(handleAction,ID_NATIONITEMS);
	w = new Button(w1, Geom((float)0.16, 0.02, 0.05, 0.03),
			bk2, true, font, "Units");
	w->setTooltip(tooltip,"Select Unit List");
	w->setActionHandler(handleAction,ID_UNITITEMS);
	list = new Listbox(w1, Geom((float)0.02, 0.07, 0.19, 0.3),
			bk,true,sbw,icons,font,NULL,0);
	list->setActionHandler(handleAction,ID_LISTSELECT);
	iv = new ItemView(w1,Geom((float)0.02,0.39,0.19,0.54),
			bk,true,sbw,icons,2,5,renderCell,0);
	iv->setActionHandler(handleAction,ID_ITEMSELECT);
	tedit = new Edit(w1, Geom((float)0.02, 0.95, 0.19, 0.03), bk2, true, font, 512);
	tedit->setTooltip(tooltip,"Terrain name");
	nedit = new Edit(w1, Geom((float)0.23, 0.95, 0.7, 0.03), bk2, true, font, 512);
	nedit->setTooltip(tooltip,"Scenario name");
	mv = new MapView(w1, Geom((float)0.23, 0.07, 0.7, 0.86), bk, true, data);
	mv->render();
	mv->setTooltip(tooltip,"MapView");

	const char *btt[3] = {
		"Load Scenario (set name below)",
		"Save Scenario (set name below)",
		"Quit Editor (save before, NO prompt on unsaved changes)"
	};
	int bids[3] = {ID_LOAD, ID_SAVE, ID_QUIT};
	float by = 0.07;
	for (int i = 0; i < 3; i++) {
		w = new Button(w1, Geom((float)0.95,by,0.03,0.04),bk2, true, icons, i, 1);
		w->setTooltip(tooltip,btt[i]);
		w->setActionHandler(handleAction,bids[i]);
		by += 0.08;
	}

	curItemView = ID_TERRAINITEMS;
	std::vector<std::string> v;
	for (unsigned int i = 0; i < data->terrain.size(); i++)
		v.push_back(data->terrain[i].name);
	list->setItems(v);
}

GUI::~GUI() {
	delete bk;
	delete bk2;
	delete icons;
	delete font;
	delete w1; /* takes care of all children (hopefully) */
}

void GUI::run() {
	SDL_Event ev;

	w1->draw();
	while (!GUI::leave) {
		if (SDL_PollEvent(&ev)) {
			if (ev.type == SDL_QUIT)
				GUI::leave = true;
			else
				w1->handleEvent(&ev);

			/* XXX do scrolling here timers didnt work most
			 * like because of rendering in thread? */
			if (ev.type == SDL_KEYDOWN) {
				int dx = 0, dy = 0;
				if (ev.key.keysym.sym == SDLK_UP)
					dy -= 2;
				else if (ev.key.keysym.sym == SDLK_DOWN)
					dy += 2;
				else if (ev.key.keysym.sym == SDLK_LEFT)
					dx -= 2;
				else if (ev.key.keysym.sym == SDLK_RIGHT)
					dx += 2;
				if (dx != 0 || dy != 0) {
					gui->mv->scroll(dx,dy);
					gui->mv->render();
				}
			}
			SDL_RenderPresent(mrc);
		}
		SDL_Delay(20);
		SDL_FlushEvent(SDL_MOUSEMOTION); /* prevent event loop from dying */
	}
}

void GUI::getSelection(int &cat, int &sub, int &item) {
	cat = curItemView;
	sub = list->getSelId();
	item = iv->getSelId();
}
