CXX				= g++
TARGET 		= 9wm
CXXFLAGS += -std=c++14 -Wall -pedantic
LDFLAGS 	= -lXext -lX11
SRCS			= 9wm.cc event.cc manage.cc menu.cc client.cc grab.cc cursor.cc error.cc
OBJS			= $(SRCS:.cc=.o)

BIN = $(DESTDIR)/usr/bin/
MANDIR = $(DESTDIR)/usr/share/man/man1
MANSUFFIX = 1

SERVICE := vncserver@:2.service

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cc
	$(CXX) -c $< $(CXXFLAGS)

install: 9wm
	mkdir -p $(BIN)
	cp 9wm $(BIN)/9wm

install.man:
	mkdir -p $(MANDIR)
	cp 9wm.man $(MANDIR)/9wm.$(MANSUFFIX)

clean:
	rm -f 9wm *.o

format:
	clang-format -i *.cc *.h

restart:
	systemctl --user restart $(SERVICE) || true

.PHONY: all 9wm install install.man clean format restart
