PROGRAM = flashrom2
VERSION = 0.1

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

.PHONY: all install clean debian

install:
	@mkdir -p $(DESTDIR)/usr/bin
	@mkdir -p $(DESTDIR)/usr/share/man/man8
	cp $(PROGRAM) $(DESTDIR)/usr/bin/
	cp $(PROGRAM).8 $(DESTDIR)/usr/share/man/man8/

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $$(dirname $@)
	$(CC) -MMD $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -o $@ -c $<

debian: $(PROGRAM)
	mkdir -p obj
	git archive --format=tar.gz -o obj/$(PROGRAM)-$(VERSION).orig.tar.gz v$(VERSION)
	debian/rules build
	fakeroot debian/rules binary

-include $(OBJS:.o=.d)
