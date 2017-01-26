///
/// Copyright (c) 1994-1996 David Hogan, see README for licence details
///

#pragma once

#include <cstdint>

static constexpr char const* VERSION = "9wm version 1.3.7, Copyright (c) 2016 multiple authors";

static constexpr std::size_t MAXHIDDEN = 32;
static constexpr std::size_t B3FIXED = 5;

#define AllButtonMask (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask)
#define ButtonMask (ButtonPressMask | ButtonReleaseMask)
#define MenuMask (ButtonMask | ButtonMotionMask | ExposureMask)
#define MenuGrabMask (ButtonMask | ButtonMotionMask | StructureNotifyMask)

static constexpr char const* DEFSHELL = "/bin/sh";

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

inline bool hidden(Client* c) { return c->state == IconicState; }
inline bool withdrawn(Client* c) { return c->state == WithdrawnState; }
inline bool normal(Client* c) { return c->state == NormalState; }

/* c->proto */
#define Pdelete 1
#define Ptakefocus 2

struct Menu {
  char const** item;
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
extern int initting;
extern XFontStruct* font;
extern int nostalgia;
extern char** myargv;
extern Bool shape;
extern char* termprog;
extern char const* shell;
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

/* 9wm.c */
ScreenInfo* getscreen(Window w);
Time timestamp(Display* dpy);

/* event.c */
int mainloop();

/* manage.c */
int manage(Client* c, int mapped);
void scanwins(ScreenInfo* s);
void withdraw(Client* c);
void gravitate(Client* c, int invert);
void cmapfocus(Client* c);
void cmapnofocus(ScreenInfo* s);
void getcmaps(Client* c);
char* getprop(Window w, Atom a);
int getiprop(Window w, Atom a);
void setwstate(Client* c, int state);
void setlabel(Client* c);
void getproto(Client* c);
void gettrans(Client* c);

/* menu.c */
void button(XButtonEvent* e);
void move(Client* c);
void hide(Client* c);
void unhide(int n, int map);
void unhidec(Client* c, int map);
void renamec(Client* c, char* name);

/* client.c */
void setactive(Client* c, int on);
void draw_border(Client* c, int active);
void active(Client* c);
void nofocus();
void top(Client* c);
Client* getclient(Window w, int create);
void rmclient(Client* c);
void sendconfig(Client* c);
void sendcmessage(Window w, Atom a, long x, int isroot);
void dump_revert();
void dump_clients();

/* grab.c */
int menuhit(Menu* m, XButtonEvent* e);
Client* selectwin(ScreenInfo* s, int release, int* shift);
int sweep(Client* c);
int drag(Client* c);
void getmouse(ScreenInfo* s, int* x, int* y);
void setmouse(ScreenInfo* s, int x, int y);

/* error.c */
void fatal(char const* s);
void graberror(char const* f, int err);
void trace(Client* c, char const* s, XEvent* e);

/* cursor.c */
void initcurs(ScreenInfo* s);
