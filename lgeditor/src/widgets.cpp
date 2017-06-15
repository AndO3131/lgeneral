/*
 * widgets.cpp
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

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <list>
#include <string>
#include <vector>

#include "widgets.h"

int Geom::sw = 640; /* safe values before MainWindow is called */
int Geom::sh = 480;

SDL_Renderer *mrc = NULL; /* main window render context, got only one */

/** Main application window */

MainWindow::MainWindow(const char *title, int _w, int _h)
{
	int flags = 0;
	if (_w <= 0 || _h <= 0) {
		/* use full (fake) screen */
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode(0,&mode);
		_w = mode.w;
		_h = mode.h;
		flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	w = _w;
	h = _h;
	Geom::sw = w;
	Geom::sh = h;
	if( (mw = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, w, h, flags)) == NULL)
		_logsdlerr();
	if ((mr = SDL_CreateRenderer(mw, -1, SDL_RENDERER_ACCELERATED)) == NULL)
		_logsdlerr();
	mrc = mr;
}
MainWindow::~MainWindow()
{
	if (mr)
		SDL_DestroyRenderer(mr);
	if (mw)
		SDL_DestroyWindow(mw);
}

void MainWindow::refresh()
{
	SDL_RenderPresent(mr);
}

/** Image */

Image::Image(std::string fname)
{
	SDL_Surface *surf = IMG_Load(fname.c_str());
	if (surf == NULL)
		_logsdlerr();
	SDL_SetColorKey(surf, SDL_TRUE, 0x0); /* set black as transparent */
	if ((tex = SDL_CreateTextureFromSurface(mrc, surf)) == NULL)
		_logsdlerr();
	w = surf->w;
	h = surf->h;
	SDL_FreeSurface(surf);
}
Image::Image(SDL_Surface *s)
{
	if ((tex = SDL_CreateTextureFromSurface(mrc,s)) == NULL)
		_logsdlerr();
	w = s->w;
	h = s->h;
}
Image::Image(Image *s, int x, int y, int w, int h)
{
	SDL_Rect srect = {x, y, w, h};
	SDL_Rect drect = {0, 0, w, h};

	this->w = w;
	this->h = h;
	if ((tex = SDL_CreateTexture(mrc,SDL_PIXELFORMAT_RGBA8888,
				SDL_TEXTUREACCESS_TARGET,w, h)) == NULL)
		_logsdlerr();
	SDL_SetRenderTarget(mrc, tex);
	SDL_RenderCopy(mrc, s->getTex(), &srect, &drect);
	SDL_SetRenderTarget(mrc, NULL);
}


Image::~Image()
{
	SDL_DestroyTexture(tex);
}

SDL_Texture *Image::getTex()
{
	return tex;
}

void Image::copy() /* full scale */
{
	SDL_RenderCopy(mrc, tex, NULL, NULL);
}
void Image::copy(int x, int y)
{
	SDL_Rect drect = {x, y, w, h};
	SDL_RenderCopy(mrc, tex, NULL, &drect);
}
void Image::copy(int x, int y, int w, int h)
{
	SDL_Rect drect = {x, y, w, h};
	SDL_RenderCopy(mrc, tex, NULL, &drect);
}
void Image::copy(int sx, int sy, int sw, int sh, int dx, int dy) {
	SDL_Rect srect = {sx, sy, sw, sh};
	SDL_Rect drect = {dx, dy, sw, sh};
	SDL_RenderCopy(mrc, tex, &srect, &drect);
}

/** Grid image: large bitmap with same sized icons */

GridImage::GridImage(std::string fname, int _gw, int _gh)
	:Image(fname), gw(_gw), gh(_gh) {}

GridImage::GridImage(SDL_Surface *s, int _gw, int _gh)
	:Image(s), gw(_gw), gh(_gh) {}

void GridImage::copy(int gx, int gy, int x, int y)
{
	SDL_Rect srect = {gx * gw, gy * gh, gw, gh};
	SDL_Rect drect = {x, y, gw, gh};
	SDL_RenderCopy(mrc, tex, &srect, &drect);
}
void GridImage::copy(int gx, int gy, int x, int y, int w, int h)
{
	SDL_Rect srect = {gx * gw, gy * gh, gw, gh};
	SDL_Rect drect = {x, y, w, h};
	SDL_RenderCopy(mrc, tex, &srect, &drect);
}

/** Font */

Font::Font(std::string fname, int sz) : size(sz)
{
	if ((font = TTF_OpenFont(fname.c_str(), size)) == NULL)
		_logsdlerr();
	setColor(255,255,255,255);
	setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
}
Font::~Font() {
	if (font)
		TTF_CloseFont(font);
}

void Font::setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	SDL_Color c = {r, g, b, a};
	clr = c;
}
void Font::setColor(SDL_Color c) {
	clr = c;
}
void Font::setAlign(int a) {
	align = a;
}
void Font::write(SDL_Texture *target, int x, int y, const char *str) {
	/* XXX doesn't look good, why no rendering
	 * into texture directly available? */
	SDL_Surface *surf;
	SDL_Texture *tex;
	SDL_Rect drect;

	if (strlen(str) == 0)
		return;

	if ((surf = TTF_RenderUTF8_Blended(font, str, clr)) == NULL)
		_logsdlerr();
	if ((tex = SDL_CreateTextureFromSurface(mrc, surf)) == NULL)
		_logsdlerr();
	if (align & ALIGN_X_LEFT)
		drect.x = x;
	else if (align & ALIGN_X_RIGHT)
		drect.x = x - surf->w;
	else
		drect.x = x - surf->w/2; /* center */
	if (align & ALIGN_Y_TOP)
		drect.y = y;
	else if (align & ALIGN_Y_BOTTOM)
		drect.y = y - surf->h;
	else
		drect.y = y - surf->h/2;
	drect.w = surf->w;
	drect.h = surf->h;
	if (target)
		SDL_SetRenderTarget(mrc, target);
	SDL_RenderCopy(mrc, tex, NULL, &drect);
	if (target)
		SDL_SetRenderTarget(mrc, NULL);
}
void Font::writeText(SDL_Texture *target, int x, int y, const char *text, int wrapwidth)
{
	/* XXX doesn't look good, why no rendering
	 * into texture directly available? */
	SDL_Surface *surf;
	SDL_Texture *tex;
	SDL_Rect drect;

	if (strlen(text) == 0)
		return;

	if ((surf = TTF_RenderUTF8_Blended_Wrapped(font, text, clr, wrapwidth)) == NULL)
		_logsdlerr();
	if ((tex = SDL_CreateTextureFromSurface(mrc, surf)) == NULL)
		_logsdlerr();
	drect.x = x;
	drect.y = y;
	drect.w = surf->w;
	drect.h = surf->h;
	if (target)
		SDL_SetRenderTarget(mrc, target);
	SDL_RenderCopy(mrc, tex, NULL, &drect);
	if (target)
		SDL_SetRenderTarget(mrc, NULL);
}

/** Widget */

int Widget::checkFocus(const SDL_Event *ev) {
	int oldFocus = hasFocus;
	if (ev->type == SDL_MOUSEMOTION)
		hasFocus = (ev->motion.x >= ax &&
				ev->motion.x < ax + w &&
				ev->motion.y >= ay &&
				ev->motion.y < ay + h);
	if (oldFocus != hasFocus) {
		if (tooltipLabel && hasFocus)
			tooltipLabel->setText(tooltipText.c_str());
		else if (tooltipLabel)
			tooltipLabel->setText("");
		return 1;
	}
	return 0;
}

Widget::Widget(Widget * p, Geom g, Image *i, bool border)
			: parent(p), rx(g.xpos()), ry(g.ypos()), w(g.width()), h(g.height()),
			  hasFocus(false), isHidden(false), actionID(0), actionCB(NULL),
			  tooltipLabel(NULL)
{
	if (parent == NULL) {
		ax = rx;
		ay = ry;
	} else {
		ax = parent->ax + rx;
		ay = parent->ay + ry;
		parent->addChild(this);
	}

	bkgnd = new Image(i, 0, 0, w, h);
	/* copy from parent (if any) to allow transparency. easy approach for one layer
	 * of none overlapping widgets... which is good enough for now */
	if (parent) {
		SDL_SetRenderTarget(mrc, bkgnd->getTex());
		parent->getBkgnd()->copy(rx, ry,w,h,0,0);
		SDL_SetRenderTarget(mrc, NULL);
	}
	/* re-render scaled full image... looks nicer */
	SDL_SetRenderTarget(mrc, bkgnd->getTex());
	i->copy();
	SDL_SetRenderTarget(mrc, NULL);
	/* if border is set, surround by white line */
	if (border) {
		SDL_Point pts[5] = {
				{0, 0},
				{w - 1, 0},
				{w - 1, h - 1},
				{0, h - 1},
				{0, 0}
		};
		SDL_SetRenderTarget(mrc, bkgnd->getTex());
		SDL_SetRenderDrawBlendMode(mrc, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(mrc, 255,255,255, 128);
		SDL_RenderDrawLines(mrc, pts, 5);
		SDL_SetRenderDrawBlendMode(mrc, SDL_BLENDMODE_NONE);
		SDL_SetRenderTarget(mrc, NULL);
	}
}
Widget::~Widget()
{
	if (bkgnd)
		delete bkgnd;
	_foreach(&children, cit) {
		Widget *w = *cit;
		delete w;
	}
}

void Widget::setActionHandler(ActionCallback ac, int id)
{
	actionCB = ac;
	actionID = id;
}
Widget *Widget::getParent() {
	return parent;
}
int Widget::getFocus() {
	return hasFocus;
}
void Widget::setTooltip(Label *l, std::string text) {
	tooltipLabel = l;
	tooltipText = text;
}
Image *Widget::getBkgnd()
{
	return bkgnd;
}

void Widget::show()
{
	isHidden = false;
	draw();
}
void Widget::hide()
{
	isHidden = true;
	if (parent)
		parent->draw(); /* redraw without us */
}

void Widget::adjustAbsolutePosition() {
	if (parent == NULL) {
		ax = rx;
		ay = ry;
	} else {
		ax = parent->ax + rx;
		ay = parent->ay + ry;
	}
	_foreach(&children, cit) {
		Widget *w = *cit;
		w->adjustAbsolutePosition();
	}
}
void Widget::move(int _rx, int _ry)
{
	rx = _rx;
	ry = _ry;
	adjustAbsolutePosition();
	if (parent)
		parent->draw();
}

void Widget::draw()
{
	if (isHidden)
		return;
	/* draw me... */
	bkgnd->copy(ax, ay);
	/* ... and my beloved children */
	_foreach(&children, cit) {
		Widget *w = *cit;
		w->draw();
	}
}

void Widget::handleEvent(const SDL_Event *ev)
{
	if (isHidden)
		return;
	checkFocus(ev);

	/* pass on to children */
	_foreach(&children, cit) {
		Widget *w = *cit;
		w->handleEvent(ev);
	}
}

/** Label */

Label::Label(Widget * p, Geom g, Image *i, bool border, Font *f, const char *t)
	: Widget(p,g,i,border), font(f)
{
	setColor(255,255,255,255); /* white */
	setAlign(ALIGN_X_CENTER | ALIGN_Y_CENTER);
	if (t)
		setText(t);
	else
		setText("");
}

void Label::setColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
	SDL_Color c = {r,g,b,a};
	color = c;
}
void Label::setText(std::string str) {
	text.clear();
	text += str;
	draw();
}
void Label::setFont(Font *f) {
	font = f;
}
void Label::setAlign(int a)
{
	int pad = 2; /* XXX fixed padding for now */

	align = a;
	tx = w/2;
	ty = h/2;
	if (a & ALIGN_X_LEFT)
		tx = pad;
	else if (a & ALIGN_X_RIGHT)
		tx = w - pad;
	if (a & ALIGN_Y_TOP)
		ty = pad;
	else if (a & ALIGN_Y_BOTTOM)
		ty = h - pad;
}
void Label::draw() {
	if (isHidden)
		return;
	Widget::draw();
	if (font) {
		SDL_Rect cr = {ax,ay,w,h};

		SDL_RenderSetClipRect(mrc, &cr);
		font->setColor(color);
		font->setAlign(align);
		font->write(NULL,ax+tx,ay+ty,text.c_str());
		SDL_RenderSetClipRect(mrc, NULL);
	}
}

/** Button */

Button::Button(Widget * p, Geom g, Image *i, bool border, Font *f, const char *t)
	: Label(p,g,i,border,f,t)
{
}
Button::Button(Widget *p, Geom g, Image *bk, bool border, GridImage *i, int ix, int iy)
	: Label(p, g, bk, border, NULL, NULL)
{
	/* add (scaled) icon to background */
	SDL_SetRenderTarget(mrc,bkgnd->getTex());
	i->copy(ix,iy,0,0,g.width(),g.height());
	SDL_SetRenderTarget(mrc,NULL);
}

void Button::draw() {
	if (isHidden)
		return;
	Label::draw();
	if (hasFocus) {
		/* add golden frame on focus */
		SDL_Point pts[5] = {
				{ax + 1, ay + 1},
				{ax + w - 2, ay + 1},
				{ax + w - 2, ay + h - 2},
				{ax + 1, ay + h - 2},
				{ax + 1, ay + 1}
		};
		SDL_SetRenderDrawColor(mrc, 255, 230, 0, 255);
		SDL_RenderDrawLines(mrc, pts, 5);
	}
}

void Button::handleEvent(const SDL_Event *ev)
{
	if (isHidden)
		return;
	if (checkFocus(ev))
		draw();
	if (ev->type == SDL_MOUSEBUTTONUP && hasFocus && actionCB)
		actionCB(actionID, this, ev);
}

/** Edit */

Edit::Edit(Widget *p, Geom g, Image *i, bool border, Font *f, int sz)
	: Label(p,g,i,border,f,""), size(sz)
{
}

void Edit::draw() {
	if (isHidden)
		return;
	Label::draw();
	if (hasFocus) {
		/* add golden frame on focus */
		SDL_Point pts[5] = {
				{ax + 1, ay + 1},
				{ax + w - 2, ay + 1},
				{ax + w - 2, ay + h - 2},
				{ax + 1, ay + h - 2},
				{ax + 1, ay + 1}
		};
		SDL_SetRenderDrawColor(mrc, 255, 230, 0, 128);
		SDL_RenderDrawLines(mrc, pts, 5);
	}
}

 void Edit::handleEvent(const SDL_Event *ev)
{
	int changed = false;

	if (isHidden)
		return;
	if (checkFocus(ev)) {
		changed = true;
		if (hasFocus)
			SDL_StartTextInput();
		else
			SDL_StopTextInput();
	}

	if (hasFocus) {
		if (ev->type == SDL_KEYDOWN) {
			if( ev->key.keysym.sym == SDLK_BACKSPACE) {
				if (text.length() > 0)
					text.erase(text.length()-1,1);
				changed = true;
			}
		} else if (ev->type == SDL_KEYUP) {
			if (ev->key.keysym.sym == SDLK_RETURN && actionCB)
				actionCB(actionID, this, ev);
		} else if (ev->type == SDL_TEXTINPUT && text.length() < (unsigned)size) {
			text += ev->text.text;
			changed = true;
		}
	}
	if (changed)
		draw();
}

/** Vertical Scrollbar */

void VScrollbar::inc() {
	cur += step;
	if (cur > max)
		cur = max;
	adjustTrackBtn();
}
void VScrollbar::dec() {
	cur -= step;
	if (cur < min)
		cur = min;
	adjustTrackBtn();
}
void VScrollbar::set(int v) {
	if (v < min)
		cur = min;
	else if (v > max)
		cur = max;
	else
		cur = v;
	adjustTrackBtn();
}
void VScrollbar::setRange(int _min, int _max, int _step)
{
	min = _min;
	max = _max;
	step = _step;
	if (max < min) /* check weird range */
		max = min;
	set(min);
}
void VScrollbar::adjustTrackBtn() {
	int ypos = trackYMin;
	if (min < max) {
		float rel = (float)(cur - min) / (max - min);
		ypos = trackYMin + rel*(trackYMax - trackYMin);
	}
	trackBtn->move(0,ypos);
}
void VScrollbar::actionHandler(int id, Widget *w, const SDL_Event *ev) {
	VScrollbar *vs = (VScrollbar*)w->getParent();
	if (id == 0)
		vs->dec();
	else if (id == 1)
		vs->inc();
}

VScrollbar::VScrollbar(Widget * p, Geom g, Image *bk, bool border,
			GridImage *i, int _min, int _max, int _step)
	: Widget(p,g,bk,border), cur(_min), min(_min), max(_max), step(_step)
{
	int bs = g.width(); /* make buttons squares of width*width */

	if (max < min) { /* check weird range */
		max = min;
		cur = min;
	}

	upBtn = new Button(this, Geom(0, 0, bs, bs), bk, true, i, 0, 0 );
	upBtn->setActionHandler(VScrollbar::actionHandler, 0);
	downBtn = new Button(this, Geom(0, g.height() - bs, bs, bs), bk, true, i, 1, 0);
	downBtn->setActionHandler(VScrollbar::actionHandler, 1);
	trackBtn = new Button(this, Geom(0, bs, bs, bs), bk, true, i, 2, 0);
	trackYMin = bs;
	trackYMax = g.height() - 2*bs;
}

int VScrollbar::getValue() {
	return cur;
}

void VScrollbar::handleEvent(const SDL_Event *ev)
{
	/* if we don't override Widget::handleEvent it dies recursively
	 * if drawing is invoked by actionhandler... wtf?!?
	 * -> if we do have children override it */
	if (isHidden)
		return;
	upBtn->handleEvent(ev);
	downBtn->handleEvent(ev);
	/* XXX a bit messy yes a trackbutton class remembering absolute position
	 * where it was clicked would be much better... is good enough anyway */
	if (ev->type == SDL_MOUSEMOTION &&
				trackBtn->getFocus()
				&& (ev->motion.state & SDL_BUTTON_LMASK)) {
		float rel = (float)(ev->motion.y - (ay + trackYMin)) /
						(trackYMax - trackYMin);
		if (rel < 0)
			rel = 0;
		if (rel > 1)
			rel = 1;
		set(min + rel*(max-min));
		adjustTrackBtn();
	} else
		trackBtn->handleEvent(ev);
}

/** List */

Listbox::Listbox(Widget *p, Geom g, Image *bk, bool border,
		int sbw, GridImage *sbi,
		Font *f, const char **_items, int icount)
	: Widget(p, g, bk, border), font(f), linePadding(2), sel(-1)
{
	lineCount = h/f->getLineHeight();
	sbar = new VScrollbar(this, Geom(w - sbw, 0, sbw, h), bk, true, sbi,
					0, icount - lineCount, 1);
	for (int i = 0; i < icount; i++)
		items.push_back(_items[i]);
}

int Listbox::getSelId() {
	return sel;
}
const char * Listbox::getSelItem() {
	if (sel > -1)
		return items[sel].c_str();
	return NULL;
}
void Listbox::setItems(std::vector<std::string> &i)
{
	items = i;
	sbar->setRange(0, items.size() - lineCount, 1);
	sel = -1;
	draw();
}

void Listbox::draw() {
	if (isHidden)
		return;
	bkgnd->copy(ax, ay);
	int wx = ax + linePadding, wy = ay + linePadding;
	int start = sbar->getValue();
	int end1 = start + lineCount;
	if ((unsigned)end1 > items.size())
		end1 = items.size();

	SDL_Rect cr = {ax+1,ay+1,w-2,h-2};
	SDL_RenderSetClipRect(mrc,&cr);

	for (int i = start; i < end1; i++) {
		font->setAlign(ALIGN_X_LEFT | ALIGN_Y_TOP);
		if (sel == i) {
			SDL_Rect dr = {wx, wy, 0, 0};
			font->getTextSize(items[i].c_str(),&dr.w,&dr.h);
			dr.w = w - 2*linePadding; /* fill all of line */
			SDL_SetRenderDrawBlendMode(mrc, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(mrc,  255, 230, 0, 128);
			SDL_RenderFillRect(mrc, &dr);
			SDL_SetRenderDrawBlendMode(mrc, SDL_BLENDMODE_NONE);
		}
		font->write(NULL,wx,wy,items[i].c_str());
		wy += font->getLineHeight();
	}

	SDL_RenderSetClipRect(mrc,NULL);
	sbar->draw();
}

void Listbox::handleEvent(const SDL_Event *ev)
{
	bool changed = false;
	if (isHidden)
		return;
	if (checkFocus(ev))
		changed = true;

	int ov = sbar->getValue();
	sbar->handleEvent(ev);
	if (ov != sbar->getValue())
		changed = true;

	if (hasFocus) {
		if (ev->type == SDL_MOUSEBUTTONUP  &&
			ev->button.x - ax < w - sbar->getWidth()) {
			sel = sbar->getValue();
			sel += (ev->button.y - ay - linePadding) / font->getLineHeight();
			if ((unsigned)sel > items.size() - 1)
				sel = items.size() - 1;
			if (actionCB)
				actionCB(actionID, this, ev);
			changed = true;
		} else if (ev->type == SDL_MOUSEWHEEL) {
			if (ev->wheel.y < 0)
				sbar->inc();
			else
				sbar->dec();
			changed = true;
		} else if (ev->type == SDL_KEYDOWN) {
			if (ev->key.keysym.sym == SDLK_UP)
				sbar->dec();
			else if (ev->key.keysym.sym == SDLK_DOWN)
				sbar->inc();
			changed = true;
		}
	}
	if (changed)
		draw();
}

/** Generic Item view */

ItemView::ItemView(Widget *p, Geom g, Image *bk, bool border,
		int sbw, GridImage *sbi,
		int cols, int rows, void (*cb)(int,SDL_Texture*,int,int), int max)
	: Widget(p, g, bk, border), cpad(4), sel(-1), size(max), nrows(rows), ncols(cols)
{
	int sbmax = (max + 1) / cols - rows;
	sbar = new VScrollbar(this, Geom(w - sbw, 0, sbw, h), bk, true,
					sbi, 0, sbmax, 1);
	cw = (w - sbw - cpad*(cols+1)) / cols;
	ch = (h - cpad*(rows+1)) / rows;
	if ((ctex = SDL_CreateTexture(mrc,SDL_PIXELFORMAT_RGBA8888,
				SDL_TEXTUREACCESS_TARGET, cw, ch)) == NULL)
		_logsdlerr();
	renderCB = cb;
}
ItemView::~ItemView()
{
	SDL_DestroyTexture(ctex);
	delete sbar;
	if (bkgnd)
		delete bkgnd;
}

int ItemView::getSelId() {
	return sel;
}
void ItemView::setSize(int max) {
	int sbmax = (max + 1) / ncols - nrows;
	size = max;
	sel = -1;
	sbar->setRange(0,sbmax,1);
	draw();
}

void ItemView::draw() {
	if (isHidden)
		return;
	bkgnd->copy(ax, ay);
	int cx = ax + cpad, cy = ay + cpad;
	int start = sbar->getValue() * ncols;
	int end1 = start + ncols*nrows;
	if (end1 > size)
		end1 = size;

	for (int i = start; i < end1; i++) {
		SDL_Rect drect = {cx, cy, cw, ch};
		renderCB(i,ctex,cw,ch);
		SDL_RenderCopy(mrc, ctex, NULL, &drect);
		if (sel == i) {
			SDL_Point pts[5] = {
					{cx, cy},
					{cx + cw - 1, cy},
					{cx + cw - 1, cy + ch - 1},
					{cx, cy + ch - 1},
					{cx, cy}
			};
			SDL_SetRenderDrawColor(mrc, 255, 230, 0, 128);
			SDL_RenderDrawLines(mrc, pts, 5);
		}
		cx += cw + cpad;
		if ((i+1) % ncols == 0) {
			cx = ax + cpad;
			cy += ch + cpad;
		}
	}

	sbar->draw();
}

void ItemView::handleEvent(const SDL_Event *ev)
{
	bool changed = false;
	if (isHidden)
		return;
	if (checkFocus(ev))
		changed = true;

	int ov = sbar->getValue();
	sbar->handleEvent(ev);
	if (ov != sbar->getValue())
		changed = true;

	if (hasFocus) {
		if (ev->type == SDL_MOUSEBUTTONUP  &&
				ev->button.x - ax < w - sbar->getWidth()) {
			sel = sbar->getValue() * ncols;
			int sx = (ev->button.x - ax - cpad) / (cw + cpad);
			int sy = (ev->button.y - ay - cpad) / (ch + cpad);
			sel += sy * ncols + sx;
			if (sel > size - 1)
				sel = size - 1;
			if (actionCB)
				actionCB(actionID, this, ev);
			changed = true;
		} else if (ev->type == SDL_MOUSEWHEEL) {
			if (ev->wheel.y < 0)
				sbar->inc();
			else
				sbar->dec();
			changed = true;
		} else if (ev->type == SDL_KEYDOWN) {
			if (ev->key.keysym.sym == SDLK_UP)
				sbar->dec();
			else if (ev->key.keysym.sym == SDLK_DOWN)
				sbar->inc();
			changed = true;
		}
	}
	if (changed)
		draw();
}
