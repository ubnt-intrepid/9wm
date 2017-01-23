/*
 * Copyright (c) 2014 multiple authors, see README for licence details
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "9wm.h"

Client* clients;
Client* current;

void setactive(Client* c, int on)
{
  if (c->parent == c->screen->root) {
    return;
  }
  if (on) {
    XUngrabButton(dpy, AnyButton, AnyModifier, c->parent);
    XSetInputFocus(dpy, c->window, RevertToPointerRoot, timestamp());
    if (c->proto & Ptakefocus)
      sendcmessage(c->window, wm_protocols, wm_take_focus, 0);
    cmapfocus(c);
  }
  else {
    XGrabButton(dpy, AnyButton, AnyModifier, c->parent, False, ButtonMask, GrabModeAsync, GrabModeSync, None, None);
  }
  draw_border(c, on);
}

void draw_border(Client* c, int active)
{
  XSetWindowBackground(dpy, c->parent, active ? bordercolor : c->screen->white);
  XClearWindow(dpy, c->parent);
  if (c->hold && active) {
    XDrawRectangle(dpy, c->parent, c->screen->gc, _inset, _inset, c->dx + _border - _inset, c->dy + _border - _inset);
  }
}

void active(Client* c)
{
  if (c == nullptr) {
    fprintf(stderr, "9wm: active(c==0)\n");
    return;
  }

  if (c == current) {
    return;
  }

  if (current != nullptr) {
    setactive(current, 0);
    if (current->screen != c->screen)
      cmapnofocus(current->screen);
  }
  setactive(c, 1);

  for (Client* cc = clients; cc; cc = cc->next) {
    if (cc->revert == c)
      cc->revert = c->revert;
  }
  c->revert = current;
  while (c->revert && !normal(c->revert)) {
    c->revert = c->revert->revert;
  }
  current = c;

  if (debug) {
    dump_revert();
  }
}

void nofocus()
{
  static Window w = 0;

  if (current) {
    setactive(current, 0);
    for (Client* c = current->revert; c != nullptr; c = c->revert) {
      if (normal(c)) {
        active(c);
        return;
      }
    }
    cmapnofocus(current->screen);
    /*
     * if no candidates to revert to, fall through
     */
  }
  current = nullptr;

  if (w == 0) {
    int mask = CWOverrideRedirect;
    XSetWindowAttributes attr;
    attr.override_redirect = 1;
    w = XCreateWindow(dpy, screens[0].root, 0, 0, 1, 1, 0, CopyFromParent, InputOnly, CopyFromParent, mask, &attr);
    XMapWindow(dpy, w);
  }
  XSetInputFocus(dpy, w, RevertToPointerRoot, timestamp());
}

void top(Client* c)
{
  Client** l = &clients;

  for (Client* cc = *l; cc != nullptr; cc = *l) {
    if (cc == c) {
      *l = c->next;
      c->next = clients;
      clients = c;
      return;
    }
    l = &cc->next;
  }
  fprintf(stderr, "9wm: %p not on client list in top()\n", (void*)c);
}

Client* getclient(Window w, int create)
{
  if (w == 0 || getscreen(w)) {
    return 0;
  }

  for (Client* c = clients; c != nullptr; c = c->next) {
    if (c->window == w || c->parent == w)
      return c;
  }

  if (!create)
    return 0;

  Client* c = (Client*)malloc(sizeof(Client));
  memset(c, 0, sizeof(Client));
  c->window = w;
  c->parent = None; // c->parent will be set by the caller
  c->reparenting = 0;
  c->state = WithdrawnState;
  c->init = 0;
  c->cmap = None;
  c->label = c->class_ = 0;
  c->revert = 0;
  c->is9term = 0;
  c->hold = 0;
  c->ncmapwins = 0;
  c->cmapwins = 0;
  c->wmcmaps = 0;
  c->next = clients;
  clients = c;
  return c;
}

void rmclient(Client* c)
{
  for (Client* cc = current; cc && cc->revert; cc = cc->revert) {
    if (cc->revert == c)
      cc->revert = cc->revert->revert;
  }

  if (c == clients) {
    clients = c->next;
  }

  for (Client* cc = clients; cc && cc->next; cc = cc->next) {
    if (cc->next == c) {
      cc->next = cc->next->next;
    }
  }

  if (hidden(c)) {
    unhidec(c, 0);
  }

  if (c->parent != c->screen->root) {
    XDestroyWindow(dpy, c->parent);
  }

  c->parent = c->window = None; /* paranoia */
  if (current == c) {
    current = c->revert;
    if (current == 0) {
      nofocus();
    }
    else {
      if (current->screen != c->screen) {
        cmapnofocus(c->screen);
      }
      setactive(current, 1);
    }
  }
  if (c->ncmapwins != 0) {
    XFree((char*)c->cmapwins);
    free((char*)c->wmcmaps);
  }
  if (c->iconname != 0)
    XFree((char*)c->iconname);
  if (c->name != 0)
    XFree((char*)c->name);
  if (c->instance != 0)
    XFree((char*)c->instance);
  if (c->class_ != 0)
    XFree((char*)c->class_);
  memset(c, 0, sizeof(Client)); /* paranoia */
  free(c);
}

void dump_revert()
{
#ifdef DEBUG
  int i = 0;
  for (Client* c = current; c != nullptr; c = c->revert) {
    fprintf(stderr, "%s(%lx:%d)", c->label ? c->label : "?", c->window, c->state);
    if (i++ > 100) {
      break;
    }
    if (c->revert) {
      fprintf(stderr, " -> ");
    }
  }
  if (current == nullptr) {
    fprintf(stderr, "empty");
  }
  fprintf(stderr, "\n");
#endif
}

void dump_clients(void)
{
#ifdef DEBUG
  for (Client* c = clients; c; c = c->next) {
    fprintf(stderr, "w 0x%lx parent 0x%lx @ (%d, %d)\n", c->window, c->parent, c->x, c->y);
  }
#endif
}
