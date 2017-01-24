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
#ifdef DEBUG
  // sick of "bug" reports; grab errors "just happen"
  fprintf(stderr, "9wm: %s: grab error: ", f);
  switch (err) {
  case GrabNotViewable:
    fprintf(stderr, "not viewable");
    break;
  case AlreadyGrabbed:
    fprintf(stderr, "already grabbed");
    break;
  case GrabFrozen:
    fprintf(stderr, "grab frozen");
    break;
  case GrabInvalidTime:
    fprintf(stderr, "invalid time");
    break;
  case GrabSuccess:
    return;
  default:
    fprintf(stderr, "%d", err);
    return;
  }
  fprintf(stderr, "\n");
#endif
}

void trace(Client* c, char const* s, XEvent* e)
{
#ifdef DEBUG
  if (c != nullptr) {
    fprintf(stderr, "9wm: %s: c=0x%p x %d y %d dx %d dy %d w 0x%lx parent 0x%lx\n", s, c, c->x, c->y, c->dx, c->dy,
            c->window, c->parent);
  }
  else {
    fprintf(stderr, "9wm: %s: c=0x%p\n", s, c);
  }
#endif
}
