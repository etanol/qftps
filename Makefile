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
HCC64       := amd64-mingw32msvc-gcc
CC          := gcc
CFLAGS      := -Wall -pipe -O3 -fomit-frame-pointer
LDFLAGS     := -Wall -pipe -Wl,-s
CFLAGS_DBG  := -Wall -pipe -g -DDEBUG
LDFLAGS_DBG := -Wall -pipe -g


#
# Commercial UNIX flavours do not use GCC.  Try their provided compiler instead.
#

ifeq ($(UNAME),SunOS)
	CC          := cc
	CFLAGS      := -xO3
	LDFLAGS     := -s
	CFLAGS_DBG  := -g -DDEBUG
	LDFLAGS_DBG := -g
	LIBS        := -lsocket -lnsl
endif

ifeq ($(UNAME),HP-UX)
	CC          := cc
	CFLAGS      := +O3 #+DAportable
#	CFLAGS      := -D_XOPEN_SOURCE_EXTENDED +O3
	LDFLAGS     := -s
	CFLAGS_DBG  := +O0 -g -DDEBUG
	LDFLAGS_DBG := -g
endif

ifeq ($(UNAME),OSF1)
	CC          := cc
	CFLAGS      :=
	LDFLAGS     := -s
	CFLAGS_DBG  := -g -DDEBUG
	LDFLAGS_DBG := -g
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

.PHONY: all debug clean dist bdist help

all    : uftps
debug  : uftps.dbg
hase   : uftps.exe
hase64 : uftps64.exe
dhase  : uftps.dbg.exe
dhase64: uftps.dbg64.exe


#
# Binaries (release and debug)
#

uftps: $(SOURCES:.c=.o) main-unix.o
	@echo ' Linking           $@' && $(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

uftps.dbg: $(SOURCES:.c=.dbg.o) main-unix.dbg.o
	@echo ' Linking   [debug] $@' && $(CC) $(LDFLAGS_DBG) -o $@ $^ $(LIBS)

uftps.exe: $(SOURCES:.c=.obj) main-hase.obj
	@echo ' Linking   [win32]         $@' && $(HCC) $(LDFLAGS) -o $@ $^ $(HLIBS)

uftps64.exe: $(SOURCES:.c=.obj64) main-hase.obj64
	@echo ' Linking   [win64]         $@' && $(HCC64) $(LDFLAGS) -o $@ $^ $(HLIBS)

uftps.dbg.exe: $(SOURCES:.c=.dbg.obj) main-hase.dbg.obj
	@echo ' Linking   [win32] [debug] $@' && $(HCC) $(LDFLAGS_DBG) -o $@ $^ $(HLIBS)

uftps.dbg64.exe: $(SOURCES:.c=.dbg.obj64) main-hase.dbg.obj64
	@echo ' Linking   [win64] [debug] $@' && $(HCC64) $(LDFLAGS_DBG) -o $@ $^ $(HLIBS)


#
# Pattern rules
#

%.o: %.c uftps.h
	@echo ' Compiling         $@' && $(CC) $(CFLAGS) -c -o $@ $<

%.obj: %.c uftps.h hase.h
	@echo ' Compiling [win32]         $@' && $(HCC) $(CFLAGS) -c -o $@ $<

%.obj64: %.c uftps.h hase.h
	@echo ' Compiling [win64]         $@' && $(HCC64) $(CFLAGS) -c -o $@ $<

%.dbg.o: %.c uftps.h
	@echo ' Compiling [debug] $@' && $(CC) $(CFLAGS_DBG) -c -o $@ $<

%.dbg.obj: %.c uftps.h hase.h
	@echo ' Compiling [win32] [debug] $@' && $(HCC) $(CFLAGS_DBG) -c -o $@ $<

%.dbg.obj64: %.c uftps.h hase.h
	@echo ' Compiling [win64] [debug] $@' && $(HCC64) $(CFLAGS_DBG) -c -o $@ $<


#
# Special rules
#

next_command.o        : command_parser.h
next_command.dbg.o    : command_parser.h
next_command.obj      : command_parser.h
next_command.obj64    : command_parser.h
next_command.dbg.obj  : command_parser.h
next_command.dbg.obj64: command_parser.h

command_parser.h: command_parser.gperf
	@echo ' Generating        $@' && gperf --output-file=$@ $<


#
# Distributing source code (run only from repository checkouts)
#

dist bdist:
	@v=`hg id -t | head -1`                                          ; \
	if [ -z "$$v" ] || [ "$$v" = 'tip' ]                             ; \
	then                                                               \
	        t=`hg tags | sed -n -e '2 s/ .*$$// p'`                  ; \
	        rb=`hg id -nr $$t`                                       ; \
	        rc=`hg id -n | sed -e 's/[^0-9][^0-9]*//g'`              ; \
	        v="$$t+`expr $$rc - $$rb`"                               ; \
	fi                                                               ; \
	                                                                   \
	if [ '$@' = 'dist' ]                                             ; \
	then                                                               \
	        d="uftps-$$v"                                            ; \
	else                                                               \
	        d="bdist_uftps-$$v"                                      ; \
	fi                                                               ; \
	                                                                   \
	hg archive -X .hg_archival.txt -X .hgtags -X makebins.sh.in $$d  ; \
	$(MAKE) -C $$d command_parser.h                                  ; \
	chmod o+r $$d/command_parser.h                                   ; \
	                                                                   \
	if [ '$@' = 'dist' ]                                             ; \
	then                                                               \
	        tar cvf - $$d | gzip -vfc9 >$$d.tar.gz                   ; \
	        rm -rf $$d                                               ; \
	else                                                               \
	        sed "s/@@@V@@@/$$v/g" makebins.sh.in >$$d/makebins.sh    ; \
	        chmod ug+x $$d/makebins.sh                               ; \
	fi


#
# Cleaning and help
#

clean:
	-rm -f *.o *.obj *.obj64
	-rm -f uftps uftps.dbg uftps.exe uftps64.exe uftps.dbg.exe uftps.dbg64.exe

help:
	@echo 'User targets:'
	@echo ''
	@echo '	all       - Default target.  Build the UNIX binary.'
	@echo '	debug     - Build the UNIX binary with debugging support.'
	@echo '	hase      - Build the Hasefroch binary.'
	@echo '	hase64    - Build the Hasefroch 64 bit binary.'
	@echo '	dhase     - Build the Hasefroch binary with debugging support.'
	@echo '	dhase64   - Build the Hasefroch 64 bit binary with debugging support.'
	@echo '	clean     - Clean object and binary files.'
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
	@echo '	linux  - Linux (2.2 and above) version using the sendfile()'
	@echo '	         system call.  This is the recommended choice for'
	@echo '	         GNU/Linux systems.'
	@echo '	hase   - Hasefroch tuned implementation using TransmitFile().'
	@echo '	         Requires WinSock 2.2'
	@echo ''
	@echo 'For example, to select the mmap RETR implementation, the compilation'
	@echo 'command line can be:'
	@echo ''
	@echo '		$(MAKE) RETR=mmap debug'
	@echo ''
	@echo 'You can also add custom compilation and linking flags by defining the'
	@echo 'following make variables EXTRA_CFLAGS and EXTRA_LDFLAGS.  For example:'
	@echo ''
	@echo '		$(MAKE) EXTRA_CFLAGS="-funroll-loops -march=core2"'
	@echo ''

