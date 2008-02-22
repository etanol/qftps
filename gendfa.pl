#! /usr/bin/perl -w
#
# User FTP Server
# Author : C2H5OH
# License: GPL v2
#
# gendfa.pl - Utility to generate a DFA to recognize a particular command set
#             with the ASCII uppercase alphabet [A-Z].
#
# In order to obtain a DFA which recognizes all the commands in the @COMMANDS
# array (it should be the same, and same order, as the "enum _Commands" in
# uftps.h) what we do is the same algorithm as if populating a trie data
# structure.
#
# Instead of using a linked structure a growing matrix is used, which will be
# the resulting DFA. The first index indicates the current state and the second
# index corresponds to the next input symbol, the value in that position is the
# next state of the DFA. Then the matrix has always 26 columns ([A-Z]) and as
# many rows as states are needed. New states are created as needed, incremetally
# and appended as new rows.
#
# In order to save space, two approaches are taken. First, 8 bit unsigned
# integers are used instead of 32 bit. That reduces the size of the table four
# times; of course there is a limit of 256 states. Remember that saving space
# here is important to minimize cache misses.
#
# The contents of the table are the next states according to the current state
# and the next input character EXCEPT for the last charater. The meaning of the
# table for the last transition is different so new accepting states could be
# saved (smaller table) and a simpler algorithm to parse a word with the
# generated DFA. Instead of storing the next state, the return value is placed
# (the numeric value of the corresponding FTP_* enum constant, that's why is
# important to keep orders). Then the parsing altorithm is as simple as this:
#
# char c;     /* Read character */
# int  pc;    /* Column index, obtained from c */
# int  state; /* Current state and return value */
#
# c     = get_first_char();
# state = 1;
# while (is_letter(c)) {
#       pc    = toupper(c);
#       pc   -= 'A';  /* Uppercase and shift index */
#       state = next_state[state][pc];
#       c     = get_next_char();
# }
# /* No more letters to parse, then state holds the FTP_* numeric value */
# return state;
#
# State zero is reserved as an always failing state. Because of that, the
# FTP_NONE enum constant was reserved as an unrecognized command.
#

use strict;

#
# Known commands list. Keep it sorted, please (at least, with the same order as
# defined in uftps.h).
#
my @COMMANDS = (
        'ABOR', 'ACCT', 'ALLO', 'APPE', 'CDUP', 'CWD',  'DELE', 'FEAT', 'HELP',
        'LIST', 'MDTM', 'MKD',  'MODE', 'NLST', 'NOOP', 'OPTS', 'PASS', 'PASV',
        'PORT', 'PWD',  'QUIT', 'REIN', 'REST', 'RETR', 'RMD',  'RNFR', 'RNTO',
        'SITE', 'SIZE', 'SMNT', 'STAT', 'STOR', 'STOU', 'STRU', 'SYST', 'TYPE',
        'USER'
);

#
# DFA generation.
#
my @next_state;
my ($command, $new_result);
my ($state, $old_state, $need_new, $new_state);
my ($c, $n);

$next_state[0] = [];
$next_state[1] = [];
$new_state     = 2;
$new_result    = 1;

foreach $command (@COMMANDS) {
        $command  = reverse $command;
        $state    = 1;
        $need_new = 0;

        while ($c = chop $command) {
                $next_state[$state] = [] unless (defined $next_state[$state]);
                if ($need_new) {
                        # Delayed state creation
                        $new_state++;
                        $need_new = 0;
                }

                $c = uc $c;
                $n = ord($c) - ord('A');

                unless ($next_state[$state][$n]) {
                        # A new state is needed, but delay its creation until
                        # next iteration in case is the last transition.
                        $next_state[$state][$n] = $new_state;
                        $need_new = 1;
                }
                $old_state = $state;
                $state     = $next_state[$state][$n];
        }

        # As we kept the old state, go back and place the result value there
        # overwriting the next state
        $next_state[$old_state][$n] = $new_result;
        $new_result++;
}

#
# Print result table.
#
my $LAST = (ord('Z') - ord('A')) + 1;
my ($i, $j);

print "/* Automatically generated file. DO NOT EDIT! */\n";
print "unsigned char next_state[$new_state][$LAST] = {\n";
for ($i = 0; $i < $new_state; $i++) {
        print "/* $i */ {";
        for ($j = 0; $j < $LAST; $j++) {
                $next_state[$i][$j] = 0 unless (defined $next_state[$i][$j]);
                print "$next_state[$i][$j]";
                print ',' if ($j < $LAST - 1);
        }
        print '}';
        print ',' if ($i < $new_state - 1);
        print "\n";
}
print "};\n\n";

#
# And an utility function to parse a string.
#
print <<end_function;
static inline int parse_command (char *s)
{
        int c     = (int) *s;
        int state = 1;

        while (c != ' ' && c != '\\0') {
                c  = toupper(c);
                c -= 'A';
                state = next_state[state][c];
                s++;
                c = (int) *s;
        }

        return state;
}

end_function

