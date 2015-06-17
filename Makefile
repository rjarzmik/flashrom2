PROGRAM = flashrom2

###############################################################################
# Defaults for the toolchain.

# If you want to cross-compile, just run e.g.
# make CC=i586-pc-msdosdjgpp-gcc
# You may have to specify STRIP/AR/RANLIB as well.
#
# Note for anyone editing this Makefile: gnumake will happily ignore any
# changes in this Makefile to variables set on the command line.
STRIP   ?= strip
INSTALL = install
DIFF    = diff
PREFIX  ?= /usr/local
MANDIR  ?= $(PREFIX)/share/man
CFLAGS  ?= -O2 -Wall -Wshadow
CFLAGS	+= $(EXTRA_CFLAGS)
INCLUDES = -Iinclude
EXPORTDIR ?= .
RANLIB  ?= ranlib
LIBS := -lusb

SRCS := $(wildcard src/*.c src/*/*.c)
OBJS := $(patsubst src/%.c,obj/%.o,$(SRCS))

all: $(PROGRAM)

clean:
	rm -rf obj $(PROGRAM)

.PHONY: all install clean distclean compiler hwlibs features export tarball

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $$(dirname $@)
	$(CC) -MMD $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ -c $<

-include $(OBJS:.o=.d)
