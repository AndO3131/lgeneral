/*
 * gui.h
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

#ifndef GUI_H_
#define GUI_H_

enum {
	ID_NONE = 0,
	ID_TERRAINITEMS,
	ID_NATIONITEMS,
	ID_UNITITEMS,
	ID_LOAD,
	ID_SAVE,
	ID_SETTINGS,
	ID_QUIT,
	ID_LISTSELECT,
	ID_ITEMSELECT
};

class GUI
{
public:
	Image *bk; /* background for general widgets */
	Image *bk2; /* background with transparency */
	GridImage *icons;
	Font *font;
	Widget *w1; /* the big parent */
	MapView *mv;
	Listbox *list;
	ItemView *iv;
	Edit *nedit; /* file name */
	Edit *tedit; /* terrain name */
	Label *tooltip;
	static bool leave;
	int curItemView; /* use ID_SEL* for this */

	GUI();
	~GUI();
	void run();
	void getSelection(int &cat, int &sub, int &item);
};

#endif /* GUI_H_ */
