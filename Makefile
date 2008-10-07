#
# User FTP Server
#
# To display this Makefile help use: make help
#

RETR  ?= generic
UNAME := $(shell uname)


#
# Compilation flags, GCC by default
#

HCC         := i586-mingw32msvc-gcc
CC          := gcc
CFLAGS      := -Wall -pipe -O3 -fomit-frame-pointer
LDFLAGS     := -Wall -pipe -Wl,-s
CFLAGS_DBG  := -Wall -pipe -g -DDEBUG
LDFLAGS_DBG := -Wall -pipe -g


#
# Solaris specific configuration, not using GCC
#

ifeq ($(UNAME),SunOS)
	CC          := cc
	CFLAGS      := -xO3
	LDFLAGS     := -s
	CFLAGS_DBG  := -g -DDEBUG
	LDFLAGS_DBG := -g
	LIBS        := -lsocket -lnsl
endif


#
# Hasefroch specific link arguments.  As we use WinSock 2 (and possibly other MS
# extensions) we need to link to special libraries.
#

HLIBS := -lws2_32
ifeq ($(RETR),hase)
	HLIBS += -lmswsock
endif


#
# Custom compilation and linking flags combination.
#

ifdef EXTRA_CFLAGS
	CFLAGS  += $(EXTRA_CFLAGS)
endif
ifdef EXTRA_LDFLAGS
	LDFLAGS += $(EXTRA_LDFLAGS)
endif


#
# Source files, except for main
#

SOURCES := change_dir.c
SOURCES += command_loop.c
SOURCES += enable_passive.c
SOURCES += expand_arg.c
SOURCES += file_stats.c
SOURCES += init_session.c
SOURCES += list_dir.c
SOURCES += log.c
SOURCES += next_command.c
SOURCES += open_data_channel.c
SOURCES += open_file.c
SOURCES += parse_port_argument.c
SOURCES += reply.c
SOURCES += send_file-$(RETR).c


#
# Phony targets and aliases
#

.PHONY: all debug clean distclean dist help

all  : uftps
debug: uftps.dbg
hase : uftps.exe
dhase: uftps.dbg.exe


#
# Binaries (release and debug)
#

uftps: $(SOURCES:.c=.o) main-unix.o
	@echo ' Linking           $@' && $(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

uftps.dbg: $(SOURCES:.c=.dbg.o) main-unix.dbg.o
	@echo ' Linking   [debug] $@' && $(CC) $(LDFLAGS_DBG) -o $@ $^ $(LIBS)

uftps.exe: $(SOURCES:.c=.obj) main-hase.obj
	@echo ' Linking   [win32]         $@' && $(HCC) $(LDFLAGS) -o $@ $^ $(HLIBS)

uftps.dbg.exe: $(SOURCES:.c=.dbg.obj) main-hase.dbg.obj
	@echo ' Linking   [win32] [debug] $@' && $(HCC) $(LDFLAGS_DBG) -o $@ $^ $(HLIBS)


#
# Pattern rules
#

%.o: %.c uftps.h
	@echo ' Compiling         $@' && $(CC) $(CFLAGS) -c -o $@ $<

%.obj: %.c uftps.h hase.h
	@echo ' Compiling [win32]         $@' && $(HCC) $(CFLAGS) -c -o $@ $<

%.dbg.o: %.c uftps.h
	@echo ' Compiling [debug] $@' && $(CC) $(CFLAGS_DBG) -c -o $@ $<

%.dbg.obj: %.c uftps.h hase.h
	@echo ' Compiling [win32] [debug] $@' && $(HCC) $(CFLAGS_DBG) -c -o $@ $<


#
# Special rules
#

next_command.o      : command_parser.h
next_command.dbg.o  : command_parser.h
next_command.obj    : command_parser.h
next_command.dbg.obj: command_parser.h

command_parser.h: command_parser.gperf
	@echo ' Generating        $@' && gperf --output-file=$@ $<


#
# Distributing source code (run only from repository checkouts)
#

dist:
	@v=`hg id -t | head -1` ; \
	{ test -z "$$v" || test "$$v" = 'tip' ; } && v=`hg id -i | head -1` ; \
	hg archive -X .hg_archival.txt -X .hgtags -X makebins.sh uftps-$$v ; \
	$(MAKE) -C uftps-$$v command_parser.h ; \
	tar cvf - uftps-$$v | gzip -vfc9 >uftps-$$v.tar.gz ; \
	rm -rf uftps-$$v


#
# Cleaning and help
#

clean:
	-rm -f *.o *.obj
	-rm -f uftps uftps.dbg uftps.exe uftps.dbg.exe

distclean: clean
	-rm -f command_parser.h

help:
	@echo 'User targets:'
	@echo ''
	@echo '	all       - Default target.  Build the UNIX binary.'
	@echo '	debug     - Build the UNIX binary with debugging support.'
	@echo '	hase      - Build the Hasefroch binary.'
	@echo '	dhase     - Build the Hasefroch binary with debugging support.'
	@echo '	clean     - Clean object and binary files.'
	@echo '	distclean - Clean the command parser (clean implied).'
	@echo ''
	@echo 'You can also choose which RETR implementation to use by setting the'
	@echo 'RETR make variable to one of these values:'
	@echo ''
	@echo '	generic- Generic multiplatform implementation using read()'
	@echo '	         and send().  Works everyhere but it is the least'
	@echo '	         memory efficient.  This is the default value when'
	@echo '	         none selected.'
	@echo '	mmap   - Multiplatform implementation using file mapping'
	@echo '	         primitives to save some memory copies.'
	@echo '	linux  - Linux (2.2 and above) version using the sendfile() system'
	@echo '	         call.  This is the recommended choice for Linux if'
	@echo '	splice - Linux (2.6.17 and above) version which uses the new '
	@echo '	         splice() system call.  Requires glibc 2.5 or higher.'
	@echo '	         This is the recommended choice for Linux if you satisfy'
	@echo '	         the requisites.'
	@echo '	hase   - Hasefroch tuned implementation using TransmitFile() and'
	@echo '	         file mapping as a fallback.  Requires WinSock 2.2'
	@echo ''
	@echo 'For example, to select the mmap RETR implementation, the compilation'
	@echo 'command line can be:'
	@echo ''
	@echo '		make RETR=mmap debug'
	@echo ''
	@echo 'You can also add custom compilation and linking flags by defining the'
	@echo 'following make variables EXTRA_CFLAGS and EXTRA_LDFLAGS.  For example:'
	@echo ''
	@echo '		make EXTRA_CFLAGS="-funroll-loops -march=core2"'
	@echo ''

