# Makefile for a gkrellm plugin
# Copyright (C) 2001  paul cannon <paul@cannon.cs.usu.edu>
#
# Distributed under the GNU Public License- see COPYING
# for details.
#
# You can modify this makefile for about any gkrellm plugin that
# you're writing; just change the TARGET= line below.

TARGET=gkrellkam

CC := gcc
GTKFLAGS := $(shell gtk-config --cflags)
IMLIBFLAGS := $(shell imlib-config --cflags-gdk)
CFLAGS := $(CFLAGS) -fPIC -Wall $(GTKFLAGS) $(IMLIBFLAGS)
LDFLAGS := -shared -Wl
INST_DIR := $(HOME)/.gkrellm/plugins
INST_DIR_EX := inst$(shell test -w ${INST_DIR} && echo copy)
.PHONY: clean install

all: $(TARGET).so

%.so: %.o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	-rm -f $(TARGET).so $(TARGET).o

install: $(INST_DIR_EX)

instcopy:
	rm -f $(INST_DIR)/$(TARGET).so
	cp -fv $(TARGET).so $(INST_DIR)

inst:
	@echo "Can't find $(INST_DIR). To install manually, copy $(TARGET).so"
	@echo "into your GKrellM plugins directory."

