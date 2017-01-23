/*
 * Copyright (c) 2014 multiple authors, see README for licence details
 */
#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include "dat.h"
#include "fns.h"

int manage(Client* c, int mapped)
{
  int fixsize, dohide, doreshape, state;
  long msize;
  XClassHint class_;
  XWMHints* hints;

  trace("manage", c, 0);
  XSelectInput(dpy, c->window, ColormapChangeMask | EnterWindowMask | PropertyChangeMask | FocusChangeMask);

  /*
   * Get loads of hints
   */

  if (XGetClassHint(dpy, c->window, &class_) != 0) { /* ``Success'' */
    c->instance = class_.res_name;
    c->class_ = class_.res_class;
    c->is9term = (strcmp(c->class_, "9term") == 0);
  }
  else {
    c->instance = 0;
    c->class_ = 0;
    c->is9term = 0;
  }
  c->iconname = getprop(c->window, XA_WM_ICON_NAME);
  c->name = getprop(c->window, XA_WM_NAME);
  setlabel(c);

  hints = XGetWMHints(dpy, c->window);
  if (XGetWMNormalHints(dpy, c->window, &c->size, &msize) == 0 || c->size.flags == 0)
    c->size.flags = PSize; /* not specified - punt */

  getcmaps(c);
  getproto(c);
  gettrans(c);
  if (c->is9term)
    c->hold = getiprop(c->window, _9wm_hold_mode);

  /*
   * Figure out what to do with the window from hints
   */

  if (!getwstate(c->window, &state))
    state = hints ? hints->initial_state : NormalState;
  dohide = (state == IconicState);

  fixsize = 0;
  if ((c->size.flags & (USSize | PSize)))
    fixsize = 1;
  if ((c->size.flags & (PMinSize | PMaxSize)) == (PMinSize | PMaxSize) && c->size.min_width == c->size.max_width &&
      c->size.min_height == c->size.max_height)
    fixsize = 1;
  doreshape = !mapped;
  if (fixsize) {
    if (c->size.flags & USPosition)
      doreshape = 0;
    if (dohide && (c->size.flags & PPosition))
      doreshape = 0;
    if (c->trans != None)
      doreshape = 0;
  }
  if (c->is9term)
    fixsize = 0;
  if (c->size.flags & PBaseSize) {
    c->min_dx = c->size.base_width;
    c->min_dy = c->size.base_height;
  }
  else if (c->size.flags & PMinSize) {
    c->min_dx = c->size.min_width;
    c->min_dy = c->size.min_height;
  }
  else if (c->is9term) {
    c->min_dx = 100;
    c->min_dy = 50;
  }
  else
    c->min_dx = c->min_dy = 0;

  if (hints)
    XFree(hints);

  /*
   * Now do it!!!
   */

  if (doreshape) {
    int xmax = DisplayWidth(dpy, c->screen->num);
    int ymax = DisplayHeight(dpy, c->screen->num);
    int x, y;

    getmouse(&x, &y, c->screen);

    c->x = x - (c->dx / 2);
    c->y = y - (c->dy / 2);

    if (c->x + c->dx > xmax) {
      c->x = xmax - c->dx;
    }
    if (c->x < 0) {
      c->x = 0;
    }

    if (c->y + c->dy > ymax) {
      c->y = ymax - c->dy;
    }
    if (c->y < 0) {
      c->y = 0;
    }
  }
  gravitate(c, 0);

  c->parent = XCreateSimpleWindow(dpy, c->screen->root, c->x - BORDER, c->y - BORDER, c->dx + 2 * (BORDER - 1),
                                  c->dy + 2 * (BORDER - 1), 1, c->screen->black, c->screen->white);
  XSelectInput(dpy, c->parent, SubstructureRedirectMask | SubstructureNotifyMask);
  if (mapped)
    c->reparenting = 1;
  if (doreshape && !fixsize)
    XResizeWindow(dpy, c->window, c->dx, c->dy);
  XSetWindowBorderWidth(dpy, c->window, 0);
  XReparentWindow(dpy, c->window, c->parent, BORDER - 1, BORDER - 1);
#ifdef SHAPE
  if (shape) {
    XShapeSelectInput(dpy, c->window, ShapeNotifyMask);
    ignore_badwindow = 1; /* magic */
    setshape(c);
    ignore_badwindow = 0;
  }
#endif
  XAddToSaveSet(dpy, c->window);
  if (dohide)
    hide(c);
  else {
    XMapWindow(dpy, c->window);
    XMapWindow(dpy, c->parent);
    if (nostalgia || doreshape)
      active(c);
    else if (c->trans != None && current && current->window == c->trans)
      active(c);
    else
      setactive(c, 0);
    setwstate(c, NormalState);
  }
  if (current && (current != c))
    cmapfocus(current);
  c->init = 1;
  return 1;
}

void scanwins(ScreenInfo* s)
{
  unsigned int i, nwins;
  Client* c;
  Window dw1, dw2, *wins;
  XWindowAttributes attr;

  XQueryTree(dpy, s->root, &dw1, &dw2, &wins, &nwins);
  for (i = 0; i < nwins; i++) {
    XGetWindowAttributes(dpy, wins[i], &attr);
    if (attr.override_redirect || wins[i] == s->menuwin)
      continue;
    c = getclient(wins[i], 1);
    if (c != 0 && c->window == wins[i] && !c->init) {
      c->x = attr.x;
      c->y = attr.y;
      c->dx = attr.width;
      c->dy = attr.height;
      c->border = attr.border_width;
      c->screen = s;
      c->parent = s->root;
      if (attr.map_state == IsViewable)
        manage(c, 1);
    }
  }
  XFree((void*)wins); /* cast is to shut stoopid compiler up */
}

void gettrans(Client* c)
{
  Window trans;

  trans = None;
  if (XGetTransientForHint(dpy, c->window, &trans) != 0)
    c->trans = trans;
  else
    c->trans = None;
}

void withdraw(Client* c)
{
  XUnmapWindow(dpy, c->parent);
  gravitate(c, 1);
  XReparentWindow(dpy, c->window, c->screen->root, c->x, c->y);
  gravitate(c, 0);
  XRemoveFromSaveSet(dpy, c->window);
  setwstate(c, WithdrawnState);

  /*
   * flush any errors
   */
  ignore_badwindow = 1;
  XSync(dpy, False);
  ignore_badwindow = 0;
}

void gravitate(Client* c, int invert)
{
  int gravity, dx, dy, delta;

  gravity = NorthWestGravity;
  if (c->size.flags & PWinGravity)
    gravity = c->size.win_gravity;

  delta = c->border - BORDER;
  switch (gravity) {
  case NorthWestGravity:
    dx = 0;
    dy = 0;
    break;
  case NorthGravity:
    dx = delta;
    dy = 0;
    break;
  case NorthEastGravity:
    dx = 2 * delta;
    dy = 0;
    break;
  case WestGravity:
    dx = 0;
    dy = delta;
    break;
  case CenterGravity:
  case StaticGravity:
    dx = delta;
    dy = delta;
    break;
  case EastGravity:
    dx = 2 * delta;
    dy = delta;
    break;
  case SouthWestGravity:
    dx = 0;
    dy = 2 * delta;
    break;
  case SouthGravity:
    dx = delta;
    dy = 2 * delta;
    break;
  case SouthEastGravity:
    dx = 2 * delta;
    dy = 2 * delta;
    break;
  default:
    fprintf(stderr, "9wm: bad window gravity %d for 0x%x\n", gravity, (int)c->window);
    return;
  }
  dx += BORDER;
  dy += BORDER;
  if (invert) {
    dx = -dx;
    dy = -dy;
  }
  c->x += dx;
  c->y += dy;
}

static void installcmap(ScreenInfo* s, Colormap cmap)
{
  if (cmap == None)
    XInstallColormap(dpy, s->def_cmap);
  else
    XInstallColormap(dpy, cmap);
}

void cmapfocus(Client* c)
{
  int i, found;
  Client* cc;

  if (c == 0)
    return;
  else if (c->ncmapwins != 0) {
    found = 0;
    for (i = c->ncmapwins - 1; i >= 0; i--) {
      installcmap(c->screen, c->wmcmaps[i]);
      if (c->cmapwins[i] == c->window)
        found++;
    }
    if (!found)
      installcmap(c->screen, c->cmap);
  }
  else if (c->trans != None && (cc = getclient(c->trans, 0)) != 0 && cc->ncmapwins != 0)
    cmapfocus(cc);
  else
    installcmap(c->screen, c->cmap);
}

void cmapnofocus(ScreenInfo* s) { installcmap(s, None); }

void getcmaps(Client* c)
{
  XWindowAttributes attr;

  if (!c->init) {
    XGetWindowAttributes(dpy, c->window, &attr);
    c->cmap = attr.colormap;
  }

  Window* cw;
  int n = _getprop(c->window, wm_colormaps, XA_WINDOW, 100L, (unsigned char**)&cw);
  if (c->ncmapwins != 0) {
    XFree((char*)c->cmapwins);
    free((char*)c->wmcmaps);
  }
  if (n <= 0) {
    c->ncmapwins = 0;
    return;
  }

  c->ncmapwins = n;
  c->cmapwins = cw;

  c->wmcmaps = (Colormap*)malloc(n * sizeof(Colormap));
  for (int i = 0; i < n; i++) {
    if (cw[i] == c->window)
      c->wmcmaps[i] = c->cmap;
    else {
      XSelectInput(dpy, cw[i], ColormapChangeMask);
      XGetWindowAttributes(dpy, cw[i], &attr);
      c->wmcmaps[i] = attr.colormap;
    }
  }
}

void setlabel(Client* c)
{
  char const* label;
  char* p;

  if (c->iconname != 0) {
    label = c->iconname;
  }
  else if (c->name != 0) {
    label = c->name;
  }
  else if (c->instance != 0) {
    label = c->instance;
  }
  else if (c->class_ != 0) {
    label = c->class_;
  }
  else {
    label = "no label";
  }
  while ((p = strstr(const_cast<char*>(label), " - "))) {
    label = p + 3;
  }
  if ((p = strchr(const_cast<char*>(label), ':')) != 0)
    *p = '\0';
  for (; *label == ' '; label += 1)
    ;
  c->label = const_cast<char*>(label);
}

#ifdef SHAPE
void setshape(Client* c)
{
  int n, order;
  XRectangle* rect;

  /*
   * don't try to add a border if the window is non-rectangular
   */
  rect = XShapeGetRectangles(dpy, c->window, ShapeBounding, &n, &order);
  if (n > 1)
    XShapeCombineShape(dpy, c->parent, ShapeBounding, BORDER - 1, BORDER - 1, c->window, ShapeBounding, ShapeSet);
  XFree((void*)rect);
}
#endif

int _getprop(Window w, Atom a, Atom type, long len, unsigned char** p)
{
  Atom real_type;
  int format;
  unsigned long n, extra;

  int status = XGetWindowProperty(dpy, w, a, 0L, len, False, type, &real_type, &format, &n, &extra, p);
  if (status != Success || *p == 0)
    return -1;
  if (n == 0)
    XFree((void*)*p);
  /*
   * could check real_type, format, extra here...
   */
  return n;
}

char* getprop(Window w, Atom a)
{
  unsigned char* p;

  if (_getprop(w, a, utf8_string, 100L, &p) <= 0) {
    return 0;
  }
  return (char*)p;
}

int get1prop(Window w, Atom a, Atom type)
{
  unsigned char* p;
  if (_getprop(w, a, type, 1L, &p) <= 0) {
    return 0;
  }

  int ret = (int)(unsigned long int)(*p);
  XFree((void*)p);
  return ret;
}

Window getwprop(Window w, Atom a) { return get1prop(w, a, XA_WINDOW); }

int getiprop(Window w, Atom a) { return get1prop(w, a, XA_INTEGER); }

void setwstate(Client* c, int state)
{
  long data[2];
  data[0] = (long)state;
  data[1] = (long)None;

  c->state = state;
  XChangeProperty(dpy, c->window, wm_state, wm_state, 32, PropModeReplace, (unsigned char*)data, 2);
}

int getwstate(Window w, int* state)
{
  long* p = 0;
  if (_getprop(w, wm_state, wm_state, 2L, (unsigned char**)&p) <= 0)
    return 0;

  *state = (int)*p;
  XFree((char*)p);
  return 1;
}

void getproto(Client* c)
{
  Window w = c->window;
  c->proto = 0;

  Atom* p;
  long n = _getprop(w, wm_protocols, XA_ATOM, 20L, (unsigned char**)&p);
  if (n <= 0) {
    return;
  }

  for (int i = 0; i < n; i++) {
    if (p[i] == wm_delete) {
      c->proto |= Pdelete;
    }
    else if (p[i] == wm_take_focus) {
      c->proto |= Ptakefocus;
    }
  }

  XFree((char*)p);
}
