#
# This is the Makefile for randtype. If you don't have the random() function
# remove the apropriate define. The others are defaults for character delays 
# on output; they can be changed at runtime. 
#
CC		= gcc
CFLAGS		= -Wall -ansi -pedantic -O2 -g
DEFINES		= -DDEF_MS=18 -DDEF_MULT=20000 -DHAVE_RANDOM
LIBS		= -lz

#-------------------------------------------------------------------------------
# Shouldnt need to edit anything below here.
#-------------------------------------------------------------------------------
MANPAGE_DIR	= /usr/local/share/man
MANPAGE_SECTION	= 1
DEFINES		+= -D_GNU_SOURCE -DHAVE_ZLIB
TARGET		= randtype
OBJS		= randtype.o

all: $(TARGET)

.c.o:
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(TARGET) $(OBJS) $(LIBS)

install: all
	install -cp -m 0644 -o root $(TARGET).$(MANPAGE_SECTION).gz \
		$(MANPAGE_DIR)/man$(MANPAGE_SECTION)
	install -cps -m 0755 -o root $(TARGET) /usr/local/bin

tags:
	rm -f tags
	ctags *.c

clean:
	rm -f $(TARGET) $(OBJS) core

realclean: clean
	rm -f tags
