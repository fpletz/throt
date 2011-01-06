/*
 * throt - throttles data flow through a pipe
 * Copyright (C) 2011 Franz Pletz <fpletz@fnordicwalking.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#define BUFSIZE 16384

/* Convert tera/giga/mega/kilo-bytes expression to bytes
 * Thanks to GNU wget for the inspiration!
 */
int parse_bytes(const char *val, unsigned long *result)
{
    double number, mult;
    const char *end = val + strlen(val);
    char *endptr;

    /* Check for infinity */
    if (!strcmp (val, "inf")) {
        *result = 0;
        return 1;
    }

    /* Strip trailing whitespace */
    while (val < end && isspace(end[-1]))
        --end;
    if (val == end)
        return 0;

    switch (tolower(end[-1]))
    {
        case 'k':
            --end, mult = 1024.0;
            break;
        case 'm':
            --end, mult = 1048576.0;
            break;
        case 'g':
            --end, mult = 1073741824.0;
            break;
        case 't':
            --end, mult = 1099511627776.0;
            break;
        default:
            /* Not a recognized suffix: assume it's a digit */
            mult = 1;
    }

    /* Skip leading and trailing whitespace */
    while (val < end && isspace(*val))
        ++val;
    while (val < end && isspace(end[-1]))
        --end;
    if (val == end)
        return 0;

    number = strtod(val, &endptr);
    if(val == endptr)
        return 0;

    *result = (unsigned long)(number * mult);
    return 1;
}

/* Returns current time in ms */
long mstime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec*1000 + tv.tv_usec/1000;
}

/* Reads from stdin and writes to stdout honoring the rate */
void rw_loop(unsigned long rate) {
    char buf[BUFSIZE];
    ssize_t n;
    unsigned long start, end, delta, adjust;

    adjust = 0;

    while(1) {
	start = mstime();
	n = read(STDIN_FILENO, buf, BUFSIZE);
	
	/* No more input, quit */
	if(n == 0)
	    break;

	/* FIXME: Write all bytes! */
	write(STDOUT_FILENO, buf, n);

	end = mstime();

	/* Check how long we need to sleep
	 * Note: The delta is calculated according to how many
	 * bytes were really read (possibly != BUFSIZE)
	 */
	delta = 1000*n/rate - (end - start) + adjust;

	/* Sleeping less than 100ms is useless */
	if(delta > 100) {
	    start = mstime();
	    usleep(1000*delta);
	    end = mstime();

	    /* We probably slept more or less than specified, so
	     * save the difference to adjust the time delta later
	     */
	    adjust = delta - (end - start);
	} else {
	    adjust += delta;
	}
    }
}

/* Prints help */
void print_usage(char **argv) {
    printf("Usage: %s RATE\n"
	   "Throttles data flow through a pipe (stdin -> stdout)\n\n"
	   "RATE may be a float followed by (case-insensitive): K, M, G, T\n" 
	    , argv[0]);
}

int main(int argc, char **argv) {
    unsigned long rate;

    /* FIXME: Simple cmdline parsing for now */
    if(argc != 2) {
	print_usage(argv);
	return 1;
    }

    if(!parse_bytes(argv[1], &rate)) {
	/* It's over 9000! */
	fprintf(stderr, "%s: error parsing rate\n", argv[0]);
	print_usage(argv);
	return 1;
    }

    /* Let's get started */
    rw_loop(rate);

    return 0;
}
