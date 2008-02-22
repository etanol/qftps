# User FTP Server
# Author : C2H5OH
# License: GPL v2
#
# Makefile - To display this Makefile usage use: make help

CC      ?= gcc
CFLAGS  := -O2 -Wall -pipe -fomit-frame-pointer
LDFLAGS := -Wl,-s,-O1

Header      := uftps.h
Sources     := change_dir.c client_port.c command_loop.c file_stats.c \
               init_session.c list_dir.c misc.c next_command.c send_file.c \
               session_globals.c uftps.c
Objects     := $(Sources:.c=.o)

# Phony targets
.PHONY: linux debug clean dclean help

# Target aliases
linux: uftps
debug: uftps.dbg

# Binaries
uftps: $(Objects)
	@echo ' Linking           $@' && $(CC) $(LDFLAGS) -o $@ $^

uftps.dbg: $(Sources) $(Header) dfa.h
	@echo ' Building  [debug] $@' && \
	$(CC) -Wall -pipe -O0 -g -pg -DDEBUG -o $@ $(filter %.c, $^)

# Special dependencies and rules
next_command.o: next_command.c dfa.h

dfa.h: gendfa.pl
	@echo ' Generating        $@' && ./gendfa.pl >$@

# Pattern rules
%.o: %.c $(Header)
ifdef ARCH
	@echo ' Compiling [tuned] $@' && $(CC) $(CFLAGS) $(ARCH) -c $<
else
	@echo ' Compiling         $@' && $(CC) $(CFLAGS) -c $<
endif

%.s: %.c $(Header)
	@echo ' Assembling        $@' && $(CC) $(CFLAGS) -S $<

# Cleaning and help
clean:
	@-rm -fv $(Objects) gmon.out
dclean: clean
	@-rm -fv uftps uftps.exe uftps.dbg dfa.h

help:
	@echo 'User targets:'
	@echo ''
	@echo '	linux  - Default target. Build the GNU/Linux binary.'
	@echo '	debug  - Build the GNU/Linux binary with debugging support.'
	@echo '	clean  - Clean object files.'
	@echo '	dclean - Clean binaries and DFA header (clean implied).'
	@echo ''
	@echo 'NOTE: Enable custom optimization flags for the UNIX binary'
	@echo '      defining the ARCH make variable. For example:'
	@echo ''
	@echo '      make "ARCH=-march=pentium-m -mfpmath=sse" unix'
	@echo ''
	
