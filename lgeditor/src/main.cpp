/*
 * main.cpp
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

std::string rcpath = DATADIR; /* editor data directory */
std::string lgenpath = LGENDIR; /* lgeneral directory */
Data *data = NULL; /* global for anyone to access */
GUI *gui = NULL; /* global for anyone to access */

int main(int argc, char **argv)
{
	std::string fname;
	int w,h;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
		SDL_Log("SDL_Init failed: %s\n", SDL_GetError());
	if (TTF_Init() < 0)
		SDL_Log("TTF_Init failed: %s\n", SDL_GetError());
	
	/* read basic options */
	fname = "./newmap";
	w = 30;
	h = 30;
	if (argc > 1)
		fname = argv[1];
	if (argc > 2) {
		w = atoi(argv[2]);
		if (w % 2)
			w++;
		if (w > MAXMAPW)
			w = MAXMAPW;
	}
	if (argc > 3) {
		h = atoi(argv[3]);
		if (h % 2)
			h++;
		if (h > MAXMAPH)
			h = MAXMAPH;
	}

	new MainWindow("Window", 800, 600);
	data = new Data(w, h);
	gui = new GUI();
	/* DEBUG: */fname = "./Berlin";
	gui->nedit->setText(fname);
	data->loadScenario(fname); /* might fail */

	gui->run();

	TTF_Quit();
	SDL_Quit();
	return 0;
}
