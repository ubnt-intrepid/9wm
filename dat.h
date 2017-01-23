/* Copyright (c) 1994-1996 David Hogan, see README for licence details */

#include <cstdint>

static constexpr std::size_t MAXHIDDEN = 32;
static constexpr std::size_t B3FIXED = 5;

#define AllButtonMask (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)
#define ButtonMask (ButtonPressMask | ButtonReleaseMask)
#define MenuMask (ButtonMask | ButtonMotionMask | ExposureMask)
#define MenuGrabMask (ButtonMask | ButtonMotionMask | StructureNotifyMask)

#ifdef Plan9
#define DEFSHELL "/bin/rc"
#else
#define DEFSHELL "/bin/sh"
#endif

struct Client {
  Window window;
  Window parent;
  Window trans;
  Client* next;
  Client* revert;

  int x;
  int y;
  int dx;
  int dy;
  int border;

  XSizeHints size;
  int min_dx;
  int min_dy;

  int state;
  int init;
  int reparenting;
  int is9term;
  int hold;
  int proto;

  char* label;
  char* instance;
  char* class_;
  char* name;
  char* iconname;

  Colormap cmap;
  int ncmapwins;
  Window* cmapwins;
  Colormap* wmcmaps;
  struct ScreenInfo* screen;
};

#define hidden(c) ((c)->state == IconicState)
#define withdrawn(c) ((c)->state == WithdrawnState)
#define normal(c) ((c)->state == NormalState)

/* c->proto */
#define Pdelete 1
#define Ptakefocus 2

struct Menu {
  char const** item;
  char* (*gen)();
  int lasthit;
};

struct ScreenInfo {
  int num;
  Window root;
  Window menuwin;
  Colormap def_cmap;
  GC gc;
  unsigned long black;
  unsigned long white;
  int min_cmaps;
  Cursor target;
  Cursor sweep0;
  Cursor boxcurs;
  Cursor arrow;
  Pixmap root_pixmap;
  char display[256]; /* arbitrary limit */
};

// Nostalgia options
enum { MODERN = 0, V1, BLIT };

/* main.c */
extern Display* dpy;
extern ScreenInfo* screens;
extern int num_screens;
extern int initting;
extern XFontStruct* font;
extern int nostalgia;
extern char** myargv;
extern Bool shape;
extern char* termprog;
extern char const* shell;
extern char const* version[];
extern int _border;
extern int _inset;
extern int curtime;
extern int debug;
extern unsigned long bordercolor;

extern Atom exit_9wm;
extern Atom restart_9wm;
extern Atom wm_state;
extern Atom wm_change_state;
extern Atom _9wm_hold_mode;
extern Atom wm_protocols;
extern Atom wm_delete;
extern Atom wm_take_focus;
extern Atom wm_colormaps;
extern Atom utf8_string;
extern Atom wm_moveresize;
extern Atom active_window;

/* client.c */
extern Client* clients;
extern Client* current;

/* menu.c */
extern Client* hiddenc[];
extern int numhidden;
extern char const* b3items[];
extern Menu b3menu;

/* error.c */
extern int ignore_badwindow;
