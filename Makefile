# Makefile for the GKrellKam gkrellm plugin
# Copyright (C) 2001-2015  paul cannon <pik@debian.org>
#
# Distributed under the GNU Public License- see COPYING
# for details.

TARGET = gkrellkam

# To facilitate packaging- leave blank for normal installation
DESTDIR =

# This should point to the GKrellM headers
GKRELLM_HDRS = /usr/include

CC := gcc
GTKFLAGS := $(shell pkg-config gtk+-2.0 --cflags)
CFLAGS += -fPIC -Wall $(GTKFLAGS) -I$(GKRELLM_HDRS)
LDFLAGS += -shared
INST_DIR := $(DESTDIR)/usr/lib/gkrellm2/plugins
USER_INST_DIR := $(DESTDIR)$(HOME)/.gkrellm2/plugins
MANPAGES := gkrellkam-list.5
MANPAGE_DIR := $(DESTDIR)/usr/share/man/man5

.PHONY: clean install

all: $(TARGET).so

%.so: %.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	$(RM) $(TARGET).so $(TARGET).o

install:
	mkdir -p $(INST_DIR)
	install -d -m 755 $(INST_DIR) $(MANPAGE_DIR)
	install -m 755 $(TARGET).so $(INST_DIR)
	install -m 644 $(MANPAGES) $(MANPAGE_DIR)

userinstall:
	mkdir -p $(USER_INST_DIR)
	cp -f $(TARGET).so $(USER_INST_DIR)
