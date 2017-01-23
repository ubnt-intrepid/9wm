/* Copyright (c) 1994-1996 David Hogan, see README for licence details */

/* 9wm.c */
void usage();
void initscreen(ScreenInfo* s, int i);
ScreenInfo* getscreen(Window w);
Time timestamp();
void sendcmessage(Window w, Atom a, long x, int isroot);
void sendconfig(Client* c);
void sighandler(int signo);
void getevent(XEvent* e);
void cleanup();

/* event.c */
void mainloop(int shape_event);

/* manage.c */
int manage(Client* c, int mapped);
void scanwins(ScreenInfo* s);
void setshape(Client* c);
void withdraw(Client* c);
void gravitate(Client* c, int invert);
void cmapfocus(Client* c);
void cmapnofocus(ScreenInfo* s);
void getcmaps(Client* c);
int _getprop(Window w, Atom a, Atom type, long len, unsigned char** p);
char* getprop(Window w, Atom a);
Window getwprop();
int getiprop(Window w, Atom a);
int getwstate(Window w, int* state);
void setwstate(Client* c, int state);
void setlabel(Client* c);
void getproto(Client* c);
void gettrans(Client* c);

/* menu.c */
void button(XButtonEvent* e);
void spawn(ScreenInfo* s, char const* prog);
void reshape(Client* c);
void move(Client* c);
void delete_(Client* c, int shift);
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
void dump_revert();
void dump_clients();

/* grab.c */
int menuhit(XButtonEvent* e, Menu* m);
Client* selectwin(int release, int* shift, ScreenInfo* s);
int sweep(Client* c);
int drag(Client* c);
void getmouse(int* x, int* y, ScreenInfo* s);
void setmouse(int x, int y, ScreenInfo* s);

/* error.c */
int handler(Display* d, XErrorEvent* e);
void fatal(char const* s);
void graberror(char const* f, int err);
void showhints();
void trace(char const* s, Client* c, XEvent* e);

/* cursor.c */
void initcurs(ScreenInfo* s);
