/*
 * Copyright (c) 2014 multiple authors, see README for licence details
 */
#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include "9wm.h"

void fatal(char const* s)
{
  fprintf(stderr, "9wm: ");
  perror(s);
  fprintf(stderr, "\n");
  exit(1);
}

void graberror(char const* f, int err)
{
#ifdef DEBUG /* sick of "bug" reports; grab errors "just happen" */
  char* s;

  switch (err) {
  case GrabNotViewable:
    s = "not viewable";
    break;
  case AlreadyGrabbed:
    s = "already grabbed";
    break;
  case GrabFrozen:
    s = "grab frozen";
    break;
  case GrabInvalidTime:
    s = "invalid time";
    break;
  case GrabSuccess:
    return;
  default:
    fprintf(stderr, "9wm: %s: grab error: %d\n", f, err);
    return;
  }
  fprintf(stderr, "9wm: %s: grab error: %s\n", f, s);
#endif
}

void trace(Client* c, char const* s, XEvent* e)
{
#ifdef DEBUG
  fprintf(stderr, "9wm: %s: c=0x%p", s, c);
  if (c != nullptr) {
    fprintf(stderr, " x %d y %d dx %d dy %d w 0x%lx parent 0x%lx", c->x, c->y, c->dx, c->dy, c->window, c->parent);
  }
  fprintf(stderr, "\n");
#endif
}
