# Makefile for the GKrellKam gkrellm plugin
# Copyright (C) 2001  paul cannon <paul@cannon.cs.usu.edu>
#
# Distributed under the GNU Public License- see COPYING
# for details.

TARGET = gkrellkam

# To facilitate packaging- leave blank for normal installation
DESTDIR =

CC := gcc
GTKFLAGS := $(shell gtk-config --cflags)
IMLIBFLAGS := $(shell imlib-config --cflags-gdk)
CFLAGS := $(CFLAGS) -fPIC -Wall $(GTKFLAGS) $(IMLIBFLAGS)
LDFLAGS := -shared -Wl
INST_DIR := $(DESTDIR)/usr/lib/gkrellm/plugins
USER_INST_DIR := $(DESTDIR)$(HOME)/.gkrellm/plugins
MANPAGES := gkrellkam-list.5
MANPAGE_DIR := $(DESTDIR)/usr/share/man/man5

.PHONY: clean install

all: $(TARGET).so

%.so: %.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	-rm -f $(TARGET).so $(TARGET).o

install:
	mkdir -p $(INST_DIR)
	cp -f $(TARGET).so $(INST_DIR)
	cp -f $(MANPAGES) $(MANPAGE_DIR)

userinstall:
	mkdir -p $(USER_INST_DIR)
	cp -f $(TARGET).so $(USER_INST_DIR)
