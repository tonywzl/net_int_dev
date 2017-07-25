.KEEP_STATE:

INCLUDES = -I./ -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LIBPATH = -lglib-2.0
SYSLIBS = -lpthread 

CFLAGS = -g $(INCLUDES) $(LIBPATH) -Werror -W -Wall -Wshadow -Wcast-qual -D_FILE_OFFSET_BITS=64
CFLAGS += -ggdb -DDEBUG_LEVEL=7
SRCS =  ini.c sm.c ut_main.c

OBJS = $(SRCS:.c=.o)

SYMLINKS = list.h nid_log.h \
	ini_if.h ini.c \
	sm_if.h sm.c

BASEDIR = ../../..

all: sm_unit
	
list.h:
	ln -f -s $(BASEDIR)/list.h list.h

nid_log.h:
	ln -f -s $(BASEDIR)/nid_log.h nid_log.h

ini_if.h:
	ln -f -s $(BASEDIR)/ini_if.h ini_if.h

ini.c:
	ln -f -s $(BASEDIR)/ini.c ini.c

sm_if.h:
	ln -f -s $(BASEDIR)/sm_if.h sm_if.h

sm.c:
	ln -f -s $(BASEDIR)/sm.c sm.c
	

sm_unit: $(SYMLINKS) $(OBJS)
	$(LINK.c) -o $@ $(OBJS) $(LIBPATH) $(SYSLIBS)

tags:
	ctags *.[ch] 

clean:
	rm -f sm_unit *.a *.o core *~ TAGS .make.state

realclean: clean
	rm -f $(SYMLINKS)
