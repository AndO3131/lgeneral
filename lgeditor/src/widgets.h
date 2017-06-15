/*
 * widgets.h
 *
 *  Copyright 2017 Michael Speck
 *
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef WIDGETS_H_
#define WIDGETS_H_

class MainWindow {
public:
	SDL_Window* mw;
	SDL_Renderer* mr;
	int w, h;

	MainWindow(const char *title, int _w, int _h);
	~MainWindow();
	void refresh();
};

class Geom {
	int gx, gy, gw, gh;
public:
	static int sw; /* screen geometry, only available after MainWindow created */
	static int sh;

	Geom(int _x, int _y, int _w, int _h) : gx(_x), gy(_y), gw(_w), gh(_h) {
		if (_w <= 0)
			gw = Geom::sw;
		if (_h <= 0)
			gh = Geom::sh;
	}
	Geom(float _x, float _y, float _w, float _h) {
		gx = _x*Geom::sw;
		gy = _y*Geom::sh;
		gw = _w*Geom::sw;
		gh = _h*Geom::sh;
	}
	int width() {return gw;}
	int height() {return gh;}
	int xpos() {return gx;}
	int ypos() {return gy;}
	static int rwidth(float r) {return r*Geom::sw;}
	static int rheight(float r) {return r*Geom::sh;}
};

class Image {
protected:
	SDL_Texture *tex;
	int w, h;
public:
	Image(std::string fname);
	Image(SDL_Surface *s);
	Image(Image *s, int x, int y, int w, int h);
	~Image();
	SDL_Texture *getTex();
	void copy();
	void copy(int x, int y);
	void copy(int x, int y, int w, int h);
	void copy(int sx, int sy, int sw, int sh, int dx, int dy);
	int getWidth() {return w;}
	int getHeight() {return h;}
};

class GridImage : public Image
{
	int gw, gh; /* grid cell geometry */
public:
	GridImage(std::string fname, int _gw, int _gh);
	GridImage(SDL_Surface *s, int _gw, int _gh);
	int getGridSize() {return w/gw;}
	int getGridWidth() {return gw;}
	int getGridHeight() {return gh;}
	void copy(int gx, int gy, int x, int y);
	void copy(int gx, int gy, int x, int y, int w, int h);
};

#define ALIGN_X_LEFT	1
#define ALIGN_X_CENTER	2
#define ALIGN_X_RIGHT	4
#define ALIGN_Y_TOP	8
#define ALIGN_Y_CENTER	16
#define ALIGN_Y_BOTTOM	32

class Font {
protected:
	TTF_Font *font;
	SDL_Color clr;
	int align;
	int size;
public:
	Font(std::string fname, int size);
	~Font();
	void setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	void setColor(SDL_Color c);
	void setAlign(int a);
	int getSize() {return size;};
	int getLineHeight() {return TTF_FontLineSkip(font);};
	int getTextSize(const char *str, int *w, int *h) {return TTF_SizeText(font,str,w,h);};
	void write(SDL_Texture *target, int x, int y, const char *str);
	void writeText(SDL_Texture *target, int x, int y, const char *text, int width);
};

/** safe some typing with iterators */
#define _foreach(__LISTOBJPTR, __ITOBJ) \
		for(__ITOBJ = (__LISTOBJPTR)->begin(); \
			__ITOBJ != (__LISTOBJPTR)->end(); ++__ITOBJ)
/** easy log sdl error */
#define _logsdlerr() \
	fprintf(stderr,"ERROR: %s:%d: %s(): %s\n", \
			__FILE__, __LINE__, __FUNCTION__, SDL_GetError())

/** forward declarations */
class Widget;
class Label;

/** to avoid massive derivation shit use callback for performing actions */
typedef void (*ActionCallback)(int i, Widget *w, const SDL_Event *e);

class Widget
{
protected:
	Widget *parent; /* parent widget */
	std::list<Widget*> children; /* children widgets */
	std::list<Widget*>::iterator cit; /* iterator for list */
	Image *bkgnd; /* background image */
	int rx, ry; /* relative position to parent */
	int ax, ay; /* absolute position in screen */
	int w, h; /* size */
	bool hasFocus; /* mouse on widget? */
	bool isHidden; /* invisible? */
	int actionID; /* action id */
	ActionCallback actionCB; /* action callback */
	Label *tooltipLabel; /* pointer only */
	std::string tooltipText;

	int checkFocus(const SDL_Event *ev);
	void adjustAbsolutePosition();

public:
	Widget(Widget * p, Geom g, Image *i, bool border);
	virtual ~Widget();

	void addChild(Widget *w) { children.push_back(w); }
	void setActionHandler(ActionCallback ac, int id);
	Widget *getParent();
	int getFocus();
	void setTooltip(Label *l, std::string text);
	Image *getBkgnd();
	int getWidth() {return w;};

	void show();
	void hide();
	void move(int _rx, int _ry);
	virtual void draw();

	virtual void handleEvent(const SDL_Event *ev);
};

class Label : public Widget
{
protected:
	Font *font; /* pointer only */
	SDL_Color color;
	int align;
	std::string text;
	int tx, ty; /* position of text */
public:
	Label(Widget * p, Geom g, Image *i, bool border, Font *f, const char *t);

	void setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	void setText(std::string str);
	void setFont(Font *f);
	void setAlign(int a);

	virtual void draw();
};

class Button : public Label
{
public:
	Button(Widget * p, Geom g, Image *i, bool border, Font *f, const char *t);
	Button(Widget *p, Geom g, Image *bk, bool border, GridImage *i, int ix, int iy);
	virtual void draw();
	virtual void handleEvent(const SDL_Event *ev);
};

class Edit : public Label
{
protected:
	int size;
public:
	Edit(Widget *p, Geom g, Image *i, bool border, Font *f, int sz);

	virtual void draw();
	virtual void handleEvent(const SDL_Event *ev);

	const char *getText() {
		return text.c_str();
	}
	void setText(std::string t) {
		text = t;
	}
};

class VScrollbar : public Widget
{
protected:
	Button *upBtn, *downBtn, *trackBtn;
	int trackYMin, trackYMax;
	int cur, min, max, step;

	void adjustTrackBtn();
	static void actionHandler(int id, Widget *w, const SDL_Event *ev);
public:
	VScrollbar(Widget * p, Geom g, Image *bk, bool border,
				GridImage *i, int _min, int _max, int _step);

	void inc();
	void dec();
	void set(int v);
	void setRange(int min, int max, int step);

	int getValue();
	virtual void handleEvent(const SDL_Event *ev);
};

class Listbox : public Widget
{
protected:
	VScrollbar *sbar;
	Font *font;
	std::vector<std::string> items;
	int lineCount; /* visible lines */
	int linePadding; /* offset to start writing for x and y */
	int sel; /* index in vector of selected item or -1 */
public:
	Listbox(Widget *p, Geom g, Image *bk, bool border,
			int sbw, GridImage *sbi,
			Font *f, const char **_items, int icount);

	int getSelId();
	const char * getSelItem();
	void setItems(std::vector<std::string> &i);

	virtual void draw();
	virtual void handleEvent(const SDL_Event *ev);
};

class ItemView : public Widget
{
protected:
	VScrollbar *sbar;
	int cpad; /* cell padding */
	int cw, ch; /* cell size */
	int sel; /* selected index */
	int size; /* max items */
	int nrows, ncols;
	SDL_Texture *ctex; /* texture for rendering cell */
	void (*renderCB)(int,SDL_Texture*,int,int); /* callback to render cell */
public:
	ItemView(Widget *p, Geom g, Image *bk, bool border,
			int sbw, GridImage *sbi,
			int cols, int rows, void (*cb)(int,SDL_Texture*,int,int), int max);
	~ItemView();
	int getSelId();
	void setSize(int max);
	virtual void draw();
	virtual void handleEvent(const SDL_Event *ev);
};


#endif /* WIDGETS_H_ */
