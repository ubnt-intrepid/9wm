/*
 * Copyright multiple authors, see README for licence details
 */
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include "9wm.h"
#include <vector>

char const* version[] = {
    "9wm version 1.3.7, Copyright (c) 2016 multiple authors", 0,
};

Display* dpy;

std::vector<ScreenInfo> screens;

XFontStruct* font;
int nostalgia;
char** myargv;
char* termprog;
char const* shell;
Bool shape;
int _border = 4;
int _inset = 1;
int curtime;
int debug;
bool signalled = false;
unsigned long bordercolor;

Atom exit_9wm;
Atom restart_9wm;
Atom wm_state;
Atom wm_change_state;
Atom wm_protocols;
Atom wm_delete;
Atom wm_take_focus;
Atom wm_colormaps;
Atom wm_moveresize;
Atom active_window;
Atom utf8_string;
Atom _9wm_running;
Atom _9wm_hold_mode;

char const* fontlist[] = {
    "-*-dejavu sans-bold-r-*-*-14-*-*-*-p-*-*-*",
    "-adobe-helvetica-bold-r-*-*-14-*-*-*-p-*-*-*",
    "lucm.latin1.9",
    "blit",
    "9x15bold",
    "lucidasanstypewriter-12",
    "fixed",
    "*",
    0,
};

void sigchld(int signum)
{
  while (waitpid(-1, NULL, WNOHANG) > 0) {
  }
}

void sighandler(int signum) { signalled = true; }

void usage(void)
{
  fprintf(stderr, "usage: 9wm [-version] [-nostalgia] [-font fname] [-term prog] [-border color] [exit|restart]\n");
  exit(1);
}

int ignore_badwindow;

int error_handler_init(Display* d, XErrorEvent* e)
{
  if ((e->request_code == X_ChangeWindowAttributes) && (e->error_code == BadAccess)) {
    fprintf(stderr, "9wm: it looks like there's already a window manager running;  9wm not started\n");
    exit(1);
  }

  if (ignore_badwindow && (e->error_code == BadWindow || e->error_code == BadColor)) {
    return 0;
  }

  char msg[80];
  XGetErrorText(d, e->error_code, msg, sizeof(msg));

  char number[80];
  sprintf(number, "%d", e->request_code);

  char req[80];
  XGetErrorDatabaseText(d, "XRequest", number, "", req, sizeof(req));
  if (req[0] == '\0') {
    sprintf(req, "<request-code-%d>", e->request_code);
  }

  fprintf(stderr, "9wm: %s(0x%x): %s\n", req, (int)e->resourceid, msg);

  fprintf(stderr, "9wm: failure during initialisation; aborting\n");
  exit(1);
}

int error_handler(Display* d, XErrorEvent* e)
{
  if (ignore_badwindow && (e->error_code == BadWindow || e->error_code == BadColor)) {
    return 0;
  }

  char msg[80];
  XGetErrorText(d, e->error_code, msg, sizeof(msg));

  char number[80];
  sprintf(number, "%d", e->request_code);

  char req[80];
  XGetErrorDatabaseText(d, "XRequest", number, "", req, sizeof(req));
  if (req[0] == '\0') {
    sprintf(req, "<request-code-%d>", e->request_code);
  }

  fprintf(stderr, "9wm: %s(0x%x): %s\n", req, (int)e->resourceid, msg);

  return 0;
}

void init_font(char const* fname)
{
  font = nullptr;
  if (fname != nullptr) {
    font = XLoadQueryFont(dpy, fname);
    if (font == 0) {
      fprintf(stderr, "9wm: warning: can't load font %s\n", fname);
    }
  }
  if (font == 0) {
    for (int i = 0;; i++) {
      char const* fname = fontlist[i];
      if (fname == 0) {
        fprintf(stderr, "9wm: warning: can't find a font\n");
        break;
      }
      font = XLoadQueryFont(dpy, fname);
      if (font != 0) {
        break;
      }
    }
  }
}

// Setup color for border
void init_borercolor(char const* borderstr)
{
  bordercolor = screens[0].black;
  if (borderstr != nullptr) {
    XColor color;
    Colormap cmap = DefaultColormap(dpy, screens[0].num);
    Status stpc = 0;
    if (cmap != 0)
      stpc = XParseColor(dpy, cmap, borderstr, &color);
    Status stac = 0;
    if (stpc != 0)
      stac = XAllocColor(dpy, cmap, &color);
    if (stac != 0)
      bordercolor = color.pixel;
  }
}

void init_atoms(bool do_exit, bool do_restart)
{
  exit_9wm = XInternAtom(dpy, "9WM_EXIT", False);
  if (do_exit) {
    sendcmessage(DefaultRootWindow(dpy), exit_9wm, 0L, 1);
    XSync(dpy, False);
    exit(0);
  }

  restart_9wm = XInternAtom(dpy, "9WM_RESTART", False);
  if (do_restart) {
    sendcmessage(DefaultRootWindow(dpy), restart_9wm, 0L, 1);
    XSync(dpy, False);
    exit(0);
  }

  wm_state = XInternAtom(dpy, "WM_STATE", False);
  wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
  wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
  wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
  wm_colormaps = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
  wm_moveresize = XInternAtom(dpy, "_NET_WM_MOVERESIZE", False);
  active_window = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
  utf8_string = XInternAtom(dpy, "UTF8_STRING", False);
  _9wm_running = XInternAtom(dpy, "_9WM_RUNNING", False);
  _9wm_hold_mode = XInternAtom(dpy, "_9WM_HOLD_MODE", False);
}

ScreenInfo new_screen(int i)
{
  ScreenInfo s;
  s.num = i;
  s.root = RootWindow(dpy, i);
  s.def_cmap = DefaultColormap(dpy, i);
  s.min_cmaps = MinCmapsOfScreen(ScreenOfDisplay(dpy, i));

  char* ds = DisplayString(dpy);
  char* colon = strrchr(ds, ':');
  if (colon && screens.size() > 1) {
    strcpy(s.display, "DISPLAY=");
    strcat(s.display, ds);
    colon = s.display + 8 + (colon - ds); /* use version in buf */

    char* dot1 = strchr(colon, '.'); /* first period after colon */
    if (!dot1) {
      dot1 = colon + strlen(colon); /* if not there, append */
    }
    sprintf(dot1, ".%d", i);
  }
  else {
    s.display[0] = '\0';
  }

  s.black = BlackPixel(dpy, i);
  s.white = WhitePixel(dpy, i);

  {
    XGCValues gv;
    gv.foreground = s.black ^ s.white;
    gv.background = s.white;
    gv.function = GXxor;
    gv.line_width = 0;
    gv.subwindow_mode = IncludeInferiors;

    unsigned long mask = GCForeground | GCBackground | GCFunction | GCLineWidth | GCSubwindowMode;
    if (font != 0) {
      gv.font = font->fid;
      mask |= GCFont;
    }
    s.gc = XCreateGC(dpy, s.root, mask, &gv);
  }

  initcurs(&s);

  {
    XSetWindowAttributes attr;
    attr.cursor = s.arrow;
    attr.event_mask = SubstructureRedirectMask | SubstructureNotifyMask | ColormapChangeMask | ButtonPressMask |
                      ButtonReleaseMask | PropertyChangeMask;
    unsigned long mask = CWCursor | CWEventMask;
    XChangeWindowAttributes(dpy, s.root, mask, &attr);
  }

  XSync(dpy, False);

  s.menuwin = XCreateSimpleWindow(dpy, s.root, 0, 0, 1, 1, 1, s.black, s.white);

  return s;
}

void parse_args(int argc, char** argv, bool& do_exit, bool& do_restart, char const*& fname, char const*& borderstr)
{
  do_exit = false;
  do_restart = false;
  fname = nullptr;
  borderstr = nullptr;

  nostalgia = 0;
  debug = 0;
  termprog = nullptr;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-nostalgia") == 0) {
      nostalgia++;
    }
    else if (strcmp(argv[i], "-debug") == 0) {
      debug++;
    }
    else if (strcmp(argv[i], "-font") == 0 && i + 1 < argc) {
      fname = argv[++i];
    }
    else if (strcmp(argv[i], "-term") == 0 && i + 1 < argc) {
      termprog = argv[++i];
    }
    else if (strcmp(argv[i], "-version") == 0) {
      fprintf(stderr, "%s\n", version[0]);
      exit(0);
    }
    else if (strcmp(argv[i], "-border") == 0 && i + 1 < argc) {
      borderstr = argv[++i];
    }
    else if (argv[i][0] == '-') {
      usage();
    }
    else {
      break;
    }
  }

  for (; i < argc; i++) {
    if (strcmp(argv[i], "exit") == 0) {
      do_exit = true;
    }
    else if (strcmp(argv[i], "restart") == 0) {
      do_restart = true;
    }
    else {
      usage();
    }
  }

  if (do_exit && do_restart) {
    usage();
  }
}

int main(int argc, char* argv[])
{
  // for restart
  myargv = argv;

  // parse command line options
  bool do_exit, do_restart;
  char const* fname;
  char const* borderstr;
  parse_args(argc, argv, do_exit, do_restart, fname, borderstr);

  // get the path of shell from environment variable
  shell = (char*)getenv("SHELL");
  if (shell == nullptr) {
    shell = DEFSHELL;
  }

  // set up signal handling
  if (signal(SIGTERM, sighandler) == SIG_IGN) {
    signal(SIGTERM, SIG_IGN);
  }
  if (signal(SIGINT, sighandler) == SIG_IGN) {
    signal(SIGINT, SIG_IGN);
  }
  if (signal(SIGHUP, sighandler) == SIG_IGN) {
    signal(SIGHUP, SIG_IGN);
  }
  signal(SIGCHLD, sigchld);

  // open display
  dpy = XOpenDisplay("");
  if (dpy == nullptr) {
    fatal("can't open display");
  }

  // initialize
  {
    XSetErrorHandler(error_handler_init);
    curtime = -1; /* don't care */

    init_atoms(do_exit, do_restart);
    init_font(fname);

    if (nostalgia == BLIT) {
      _border--;
      _inset--;
    }

    size_t num_screens = ScreenCount(dpy);
    screens = std::vector<ScreenInfo>();
    screens.reserve(num_screens);
    for (size_t i = 0; i < num_screens; ++i) {
      screens.push_back(new_screen(i));
    }

    init_borercolor(borderstr);

    /*
     * set selection so that 9term knows we're running
     */
    curtime = CurrentTime;
    XSetSelectionOwner(dpy, _9wm_running, screens[0].menuwin, timestamp(dpy));

    XSync(dpy, False);
  }

  XSetErrorHandler(error_handler);

  nofocus();
  for (auto& screen : screens) {
    scanwins(&screen);
  }

  return mainloop();
}

ScreenInfo* getscreen(Window w)
{
  for (auto& screen : screens) {
    if (screen.root == w) {
      return &screen;
    }
  }
  return nullptr;
}

Time timestamp(Display* dpy)
{
  if (curtime == CurrentTime) {
    XChangeProperty(dpy, screens[0].root, _9wm_running, _9wm_running, 8, PropModeAppend, (unsigned char*)"", 0);

    XEvent ev;
    XMaskEvent(dpy, PropertyChangeMask, &ev);

    curtime = ev.xproperty.time;
  }
  return curtime;
}

void sendcmessage(Window w, Atom a, long x, int isroot)
{
  XEvent ev;
  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = w;
  ev.xclient.message_type = a;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = x;
  ev.xclient.data.l[1] = timestamp(dpy);

  long mask = 0L;
  if (isroot) {
    mask = SubstructureRedirectMask; /* magic! */
  }

  int status = XSendEvent(dpy, w, False, mask, &ev);
  if (status == 0) {
    fprintf(stderr, "9wm: sendcmessage failed\n");
  }
}

void sendconfig(Client* c)
{
  XConfigureEvent ce;
  ce.type = ConfigureNotify;
  ce.event = c->window;
  ce.window = c->window;
  ce.x = c->x;
  ce.y = c->y;
  ce.width = c->dx;
  ce.height = c->dy;
  ce.border_width = c->border;
  ce.above = None;
  ce.override_redirect = 0;

  XSendEvent(dpy, c->window, False, StructureNotifyMask, (XEvent*)&ce);
}

bool select_event(Display* dpy, timeval* t)
{
  int const fd = ConnectionNumber(dpy);
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  return select(fd + 1, &rfds, nullptr, nullptr, t) == 1;
}

XEvent getevent(Display* dpy)
{
  if (signalled) {
    fprintf(stderr, "9wm: exiting on signal\n");
    cleanup();
    exit(1);
  }

  if (QLength(dpy) > 0) {
    XEvent e;
    XNextEvent(dpy, &e);
    return e;
  }

  timeval t;
  t.tv_sec = 0;
  t.tv_usec = 0;
  if (select_event(dpy, &t)) {
    XEvent e;
    XNextEvent(dpy, &e);
    return e;
  }

  XFlush(dpy);

  for (;;) {
    if (select_event(dpy, nullptr)) {
      XEvent e;
      XNextEvent(dpy, &e);
      return e;
    }

    if (errno != EINTR) {
      if (signalled) {
        fprintf(stderr, "9wm: exiting on signal\n");
        cleanup();
      }
      else {
        perror("9wm: select failed");
      }
      exit(1);
    }
  }
  // unreachable!()
}

void cleanup()
{
  // order of un-reparenting determines final stacking order...
  Client* cc[2] = {nullptr, nullptr};
  for (Client *c = clients, *next; c; c = next) {
    next = c->next;
    int i = normal(c);
    c->next = cc[i];
    cc[i] = c;
  }

  XWindowChanges wc;
  for (int i = 0; i < 2; i++) {
    for (Client* c = cc[i]; c; c = c->next) {
      if (!withdrawn(c)) {
        gravitate(c, 1);
        XReparentWindow(dpy, c->window, c->screen->root, c->x, c->y);
      }
      wc.border_width = c->border;
      XConfigureWindow(dpy, c->window, CWBorderWidth, &wc);
    }
  }

  XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, timestamp(dpy));
  for (auto& screen : screens) {
    cmapnofocus(&screen);
  }
  XCloseDisplay(dpy);
}

void mapreq(XMapRequestEvent* e)
{
  curtime = CurrentTime;

  Client* c = getclient(e->window, 0);
  trace(c, "mapreq", reinterpret_cast<XEvent*>(e));

  if (c == 0 || c->window != e->window) {
    // workaround for stupid NCDware
    fprintf(stderr, "9wm: bad mapreq c %p w %x, rescanning\n", (void*)c, (int)e->window);
    for (auto& screen : screens) {
      scanwins(&screen);
    }
    c = getclient(e->window, 0);
    if (c == 0 || c->window != e->window) {
      fprintf(stderr, "9wm: window not found after rescan\n");
      return;
    }
  }

  switch (c->state) {
  case WithdrawnState:
    if (c->parent == c->screen->root) {
      if (!manage(c, 0))
        return;
      break;
    }
    XReparentWindow(dpy, c->window, c->parent, _border - 1, _border - 1);
    XAddToSaveSet(dpy, c->window);
  /*
   * fall through...
   */
  case NormalState:
    XMapWindow(dpy, c->window);
    XMapRaised(dpy, c->parent);
    top(c);
    setwstate(c, NormalState);
    if (c->trans != None && current && c->trans == current->window)
      active(c);
    break;
  case IconicState:
    unhidec(c, 1);
    break;
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
  XSetInputFocus(dpy, w, RevertToPointerRoot, timestamp(dpy));
}
