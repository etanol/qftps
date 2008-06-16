# User FTP Server
#
# To display this Makefile help use: make help

CC          ?= gcc
CFLAGS      := -O2 -Wall -pipe -fomit-frame-pointer
CFLAGS_DBG  := -O0 -Wall -pipe -g -pg
LDFLAGS     := -Wall -pipe -Wl,-s,-O1
LDFLAGS_DBG := -Wall -pipe -g -pg

SOURCES := change_dir.c client_port.c command_loop.c file_stats.c \
           init_session.c list_dir.c misc.c next_command.c send_file.c \
           session_globals.c uftps.c


#
# Phony targets and aliases
#
.PHONY: all debug clean distclean help

all  : uftps
debug: uftps.dbg


#
# Binaries (release and debug)
#
uftps: $(SOURCES:.c=.o)
	@echo ' Linking           $^' && $(CC) $(LDFLAGS) -o $@ $^

uftps.dbg: $(SOURCES:.c=.dbg.o)
	@echo ' Linking   [debug] $^' && $(CC) $(LDFLAGS_DBG) -o $@ $^


#
# Pattern rules
#
%.o: %.c
ifdef ARCH
	@echo ' Compiling [tuned] $<' && $(CC) $(CFLAGS) $(ARCH) -c -o $@ $<
else
	@echo ' Compiling         $<' && $(CC) $(CFLAGS) -c -o $@ $<
endif

%.dbg.o: %.c
	@echo ' Compiling [debug] $<' && $(CC) $(CFLAGS_DBG) -c -o $@ $<

.%.d: %.c
	@echo ' Dependencies      $<' && $(CC) -MM $< \
	| awk -F: '{printf "$(<:.c=.o) $@:%s\n",$$2; \
	            printf "$(<:.c=.dbg.o) $@:%s\n",$$2}' >$@


#
# Special rules
#
.next_command.d: command_list.h

command_list.h: command_list.gperf
	@echo ' Generating        $@' && gperf --output-file=$@ $<


#
# Cleaning and help
#
clean:
	@-rm -fv *.o gmon.out

distclean: clean
	@-rm -fv .*.d uftps uftps.exe uftps.dbg command_list.h

help:
	@echo 'User targets:'
	@echo ''
	@echo '	all       - Default target. Build the GNU/Linux binary.'
	@echo '	debug     - Build the GNU/Linux binary with debugging support.'
	@echo '	clean     - Clean object files.'
	@echo '	distclean - Clean binaries and DFA header (clean implied).'
	@echo ''
	@echo 'NOTE: Enable custom optimization flags for the UNIX binary'
	@echo '      defining the ARCH make variable. For example:'
	@echo ''
	@echo '      make "ARCH=-march=pentium-m -mfpmath=sse" all'
	@echo ''


#
# Include dependecy information if necessary
#
ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
ifneq ($(findstring help,$(MAKECMDGOALS)),help)
-include $(patsubst %.c,.%.d,$(SOURCES))
endif
endif

