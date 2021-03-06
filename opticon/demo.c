/* Item tracking application for Opticon OPH-1005 / OPH-3001
	    Copyright (C) 2016 Joseph H. Allen

This file is part of Cartscan

Cartscan is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 1, or (at your option) any later
version.

Cartscan is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along with
Cartscan; see the file COPYING.  If not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "4"

/* Serial port handle */

int port;

/* Screen size */

#define SCRN_WIDTH 15
#define SCRN_HEIGHT 10

/* Number of table lines which fit on the screen */
#define TABLE_LINES (SCRN_HEIGHT - 2)

/* Attributes */

#define SCRN_INVERSE 0x100

#define SCRN_FG_SHIFT 16
#define SCRN_BG_SHIFT 24
#define SCRN_COLOR_MASK 255

#define SCRN_FG_WHITE (0 << SCRN_FG_SHIFT)
#define SCRN_FG_BLUE (1 << SCRN_FG_SHIFT)
#define SCRN_FG_GREEN (2 << SCRN_FG_SHIFT)
#define SCRN_FG_RED (3 << SCRN_FG_SHIFT)
#define SCRN_FG_MAGENTA (4 << SCRN_FG_SHIFT)
#define SCRN_FG_CYAN (5 << SCRN_FG_SHIFT)
#define SCRN_FG_YELLOW (6 << SCRN_FG_SHIFT)
#define SCRN_FG_BLACK (7 << SCRN_FG_SHIFT)

#define SCRN_BG_WHITE (7 << SCRN_BG_SHIFT) 
#define SCRN_BG_BLUE (6 << SCRN_BG_SHIFT)
#define SCRN_BG_GREEN (5 << SCRN_BG_SHIFT)
#define SCRN_BG_RED (4 << SCRN_BG_SHIFT)
#define SCRN_BG_MAGENTA (3 << SCRN_BG_SHIFT)
#define SCRN_BG_CYAN (2 << SCRN_BG_SHIFT)
#define SCRN_BG_YELLOW (1 << SCRN_BG_SHIFT)
#define SCRN_BG_BLACK (0 << SCRN_BG_SHIFT)

/* Shortened-UPC check */

int upc_ok(char *s)
{
	int x;
	// Must be five digits
	if (strlen(s) != 5)
		return -1;

	for (x = 0; x != 5; ++x)
		if (s[x] < '0' || s[x] >'9')
			return -1;

	// Calculate
	x = ((s[0]-'0'+s[2]-'0')*3 + (s[1]-'0' + s[3]-'0')) % 10;
	if (x)
		x = 10 - x;

	// Check
	if (s[4] - '0' == x)
		return 0;
	else
		return -1;
}

/* Code 39 check */

int code39_ok(char *s)
{
	int val = 0;
	int ok = 0;
	while (*s) {
		int digval = 0;
		if (*s >= '0' && *s <= '9')
			digval = *s - '0';
		else if (*s >= 'A' && *s <= 'Z')
			digval = *s - 'A' + 10;
		else if (*s == '-')
			digval = 36;
		else if (*s == '.')
			digval = 37;
		else if (*s == '_')
			digval = 38;
		else if (*s == '$')
			digval = 39;
		else if (*s == '/')
			digval = 40;
		else if (*s == '+')
			digval = 41;
		else if (*s == '%')
			digval = 42;
		else
			return -1;
		++s;
		if (*s) {
			// This is a regular digit
			val += digval;
			ok = 1; // We have at least on regular digit
		} else {
			// This is the check digit
			if (val % 43 == digval && ok) {
				s[-1] = 0; // Remove the check digit
				return 0;
			} else
				return -1;
		}
	}
	return -1; // Empty string?
}

/* Flag: when set, screen update is needed */

int update_needed = 1;

/* Current screen contents */

int scrn_old[SCRN_WIDTH * SCRN_HEIGHT];

/* New screen contents */

int scrn[SCRN_WIDTH * SCRN_HEIGHT];

/* Update the screen */

void scrn_update()
{
	int y;
	int idx = 0;
	for (y = 0; y != SCRN_HEIGHT; ++y) {
		int x;
		for (x = 0; x != SCRN_WIDTH; ++x) {
			if (scrn_old[idx] != scrn[idx]) {
				int fg = (SCRN_COLOR_MASK & (scrn[idx] >> SCRN_FG_SHIFT));
				int bg = 7 - (SCRN_COLOR_MASK & (scrn[idx] >> SCRN_BG_SHIFT));
				int tmp;
				gotoxy(x, y);
				if (scrn[idx] & SCRN_INVERSE) {
					tmp = fg;
					fg = bg;
					bg = tmp;
				}
				switch (fg) {
					case 0: SetTextColor(RGB_WHITE); break;
					case 1: SetTextColor(RGB_BLUE); break;
					case 2: SetTextColor(RGB_GREEN); break;
					case 3: SetTextColor(RGB_RED); break;
					case 4: SetTextColor(RGB_MAGENTA); break;
					case 5: SetTextColor(RGB_CYAN); break;
					case 6: SetTextColor(RGB_YELLOW); break;
					case 7: SetTextColor(RGB_BLACK); break;
				}
				switch (bg) {
					case 0: SetBackColor(RGB_WHITE); break;
					case 1: SetBackColor(RGB_BLUE); break;
					case 2: SetBackColor(RGB_GREEN); break;
					case 3: SetBackColor(RGB_RED); break;
					case 4: SetBackColor(RGB_MAGENTA); break;
					case 5: SetBackColor(RGB_CYAN); break;
					case 6: SetBackColor(RGB_YELLOW); break;
					case 7: SetBackColor(RGB_BLACK); break;
				}
				printsymbol(0xFF & scrn[idx]);
				scrn_old[idx] = scrn[idx];
			}
			++idx;
		}
	}
}

/* Screen writing functions */

/* Clear screen */
void scrn_clr()
{
	int idx;
	for (idx = 0; idx != SCRN_WIDTH * SCRN_HEIGHT; ++idx)
		scrn[idx] = ' ';
}

/* Initialize screen */
void scrn_init()
{
	int idx;
	setfont(HUGE_FONT, NULL); // Select 16x32 font (15 chars x 10 lines)
	// Defaut was LARGE_FONT: 12x24, 20 chars x 13 lines
	for (idx = 0; idx != SCRN_WIDTH * SCRN_HEIGHT; ++idx)
		scrn_old[idx] = -1;
	scrn_clr();
}

/* Draw text with attributes */
void scrn_text(int y, int x, int attr, char *s)
{
	if (y < 0 || y >= SCRN_HEIGHT)
		return;
	while (*s) {
		if (x >= 0 && x < SCRN_WIDTH) {
			scrn[y * SCRN_WIDTH + x] = *(unsigned char *)s | attr;
			++x;
		}
		++s;
	}
}

/* Draw horizontal line (graphical separator) */
void scrn_gsep(int y)
{
	int x;
	if (y < 0 || y >= SCRN_HEIGHT)
		return;
	for (x = 0; x != SCRN_WIDTH; ++x)
		scrn[y * SCRN_WIDTH + x] = 0xC4; // '-';
}


/* Location code to location name table */

struct location {
	struct location *next, *prev;
	char *code;
	char *name;
};

struct location locations[1] = { { locations, locations, "End of file" } };
struct location *loc_top = locations;
struct location *loc_cur = locations;

void free_loc(struct location *r)
{
	free(r->code);
	free(r->name);
	free(r);
}

void add_loc(char *code, char *name)
{
	struct location *loc = (struct location *)malloc(sizeof(struct location));
	int empty = (loc_top == loc_cur);
	loc->code = strdup(code);
	loc->name = strdup(name);
	loc->next = locations;
	loc->prev = locations->prev;
	locations->prev->next = loc;
	locations->prev = loc;
	if (empty) {
		loc_top = loc;
	}
	loc_cur = loc->next;
	update_needed = 1;
}

/* Display mode */
int mode; /* 0 = main screen, 1 = location selection screen, 2 = status screen, 3 = scan error */

char *find_loc(char *code)
{
	struct location *loc;
	for (loc = locations->next; loc != locations; loc = loc->next)
		if (!strcmp(loc->code, code)) {
			sound(TSTANDARD, VHIGH, SLOW, 0);
			return loc->name;
		}
	/* Unknown location */
	sound(TSTANDARD, VHIGH, SERROR, SPAUSE, SLOW, SPAUSE, SERROR, SPAUSE, SLOW, SPAUSE, SERROR, SPAUSE, SERROR, 0);
	mode = 3;
	update_needed = 1;
	return "<unknown>";
}

/* Location cursor top */

void loc_cursor_top()
{
	loc_cur = locations->next;
	loc_top = locations->next;
	update_needed = 1;
}

/* Location cursor up */

void loc_cursor_up()
{
	if (loc_cur->prev != locations) {
		if (loc_cur == loc_top)
			loc_top = loc_cur;
		loc_cur = loc_cur->prev;
		update_needed = 1;
	}
}

/* Location cursor down */

void loc_cursor_down()
{
	if (loc_cur->next != locations) {
		loc_cur = loc_cur->next;
		update_needed = 1;
	}
}

/* Erase all locations */

void loc_erase()
{
	while (locations->next != locations) {
		struct location *r = locations->next;
		r->next->prev = r->prev;
		r->prev->next = r->next;
		free_loc(r);
	}
	loc_cursor_top();
}

/* Generate location screen */

/* Message to display at top of screen if msg[0] != 0 */
char msg[80];

void loc_scrn_gen()
{
	struct location *r;
	int count = 0;
	scrn_clr();
	if (msg[0]) {
		scrn_text(0, 0, 0, msg);
	} else {
		scrn_text(0, 0, 0, "Select location");
	}
	scrn_gsep(1);
	/* Follow cursor */
	/* How many lines between cursor and top? */
	count = 0;
	for (r = loc_cur; r != loc_top; r = r->prev) {
		++count;
		if (count == TABLE_LINES) {
			break;
		}
		if (r->prev == locations)
			break;
	}
	if (r != loc_top)
		loc_top = r;
	/* Print lines */
	count = 0;
	for (r = loc_top; r != locations && count != TABLE_LINES; r = r->next) {
		char buf[80];
		sprintf(buf, "%2s %s", r->code, r->name);
		if (r == loc_cur)
			scrn_text(2 + count, 0, SCRN_INVERSE, buf);
		else
			scrn_text(2 + count, 0, 0, buf);
		++count;
	}
}

char *cur_version;
char *cur_prefix;

/* Generate error screen */

void error_scrn_gen()
{
	scrn_clr();
	scrn_text(0, 0, SCRN_BG_RED | SCRN_FG_BLACK, "   Bad Scan!   ");
	scrn_text(1, 0, SCRN_BG_RED | SCRN_FG_BLACK, " Hit F4 to     ");
	scrn_text(2, 0, SCRN_BG_RED | SCRN_FG_BLACK, " continue      ");
}

/* Generate status screen */

void status_scrn_gen()
{
	struct date date;
	struct time time;
	char buf[80];

	scrn_clr();
	scrn_text(0, 0, 0, "Status");
	scrn_gsep(1);
	/* Follow cursor */
	/* How many lines between cursor and top? */

	scrn_text(2, 0, 0, "Prefix:");
	sprintf(buf, " %s", cur_prefix ? cur_prefix : "<not set>");
	scrn_text(3, 0, 0, buf);

	scrn_text(4, 0, 0, "Locations ver:");
	sprintf(buf, " %s", cur_version ? cur_version : "<not set>");
	scrn_text(5, 0, 0, buf);

	getdatetime(&date, &time);
	scrn_text(6, 0, 0, "Date/Time:");
	sprintf(buf, " %d/%d/%2.2d %2.2d:%2.2d", date.da_mon, date.da_day, date.da_year%100, time.ti_hour, time.ti_min);
	// 1+2+1+2+1+2 = 9
	// 1+2+1+2 = 6
	scrn_text(7, 0, 0, buf);

	//scrn_text(8, 0, 0, "Time:");
	//sprintf(buf, " %2.2d:%2.2d:%2.2d", time.ti_hour, time.ti_min, time.ti_sec);
	//scrn_text(9, 0, 0, buf);

	scrn_text(8, 0, 0, "Software ver:");
	sprintf(buf, " %s", VERSION);
	scrn_text(9, 0, 0, buf);
}

/* Scanned items table */

struct record {
	struct record *next, *prev;
	char *loc; /* Location code */
	char *item; /* Item code */
	struct date date;
	struct time time;
};

struct record records[1] = { { records, records, "End of file" } };
struct record *top = records; /* First line on screen */
struct record *cur = records; /* Line with cursor */

/* Free an item */

void free_rec(struct record *r)
{
	free(r->loc);
	free(r->item);
	free(r);
}

/* Add record to end of database */

void add_rec(char *loc, char *item, struct date date, struct time time)
{
	struct record *r = (struct record *)malloc(sizeof(struct record));
	int empty = (top == records);
	r->loc = strdup(loc);
	r->item = strdup(item);
	r->date = date;
	r->time = time;
	r->next = records;
	r->prev = records->prev;
	records->prev->next = r;
	records->prev = r;
	if (empty) {
		top = r;
	}
	cur = r->next;
	update_needed = 1;
	sound(TSTANDARD, VHIGH, SLOW, 0);
}

/* Delete record at cursor */

void del_rec(void)
{
	if (cur != records) {
		struct record *r = cur;
		r->next->prev = r->prev;
		r->prev->next = r->next;
		cur = r->next;
		if (top == r)
			top = r->next;
		free_rec(r);
		update_needed = 1;
	}
}

/* Cursor top */

void cursor_top()
{
	cur = records->next;
	top = records->next;
	update_needed = 1;
}

/* Cursor up */

void cursor_up()
{
	if (cur->prev != records) {
		if (cur == top)
			top = cur->prev;
		cur = cur->prev;
		update_needed = 1;
	}
}

/* Cursor down */

void cursor_down()
{
	if (cur != records) {
		cur = cur->next;
		update_needed = 1;
	}
}

/* Erase all records */

void erase()
{
	while (records->next != records) {
		struct record *r = records->next;
		r->next->prev = r->prev;
		r->prev->next = r->next;
		free_rec(r);
	}
	cursor_top();
}

/* Generate screen */

char *current_location_code = 0;
char *current_location_name = 0;

void scrn_gen()
{
	struct record *r;
	int count = 0;

	scrn_clr();

	/* Generate first line */
	if (msg[0]) {
		scrn_text(0, 0, 0, msg);
	} else if (current_location_code) {
		char buf[80];
		sprintf(buf, "%s %s", current_location_code, current_location_name);
		if (current_location_code && atoi(current_location_code) < 10)
			scrn_text(0, 0, SCRN_INVERSE + SCRN_FG_GREEN, buf);
		else
			scrn_text(0, 0, SCRN_INVERSE + SCRN_FG_YELLOW, buf);
	} else {
		scrn_text(0, 0, 0, "<Location>");
	}

	/* Generate graphical separator */
	scrn_gsep(1);

	/* Follow cursor */
	/* How many lines between cursor and top? */
	count = 0;
	for (r = cur; r != top; r = r->prev) {
		++count;
		if (count == TABLE_LINES) {
			break;
		}
		if (r->prev == records)
			break;
	}
	if (r != top)
		top = r;

	/* Generate table lines */
	count = 0;
	for (r = top; r != records && count != TABLE_LINES; r = r->next) {
		char buf[80];
		// sprintf(buf, "%2d/%2d %+5s %+8s", r->date.da_mon, r->date.da_day, r->loc, r->item);
		// 20 chars
		sprintf(buf, "%2d/%2d %+3s %+5s", r->date.da_mon, r->date.da_day, r->loc, r->item);
		// 15 chars
		if (r == cur)
			scrn_text(2 + count, 0, SCRN_INVERSE + ((atoi(r->loc) < 10) ? SCRN_FG_GREEN : SCRN_FG_YELLOW), buf);
		else
			scrn_text(2 + count, 0, ((atoi(r->loc) < 10) ? SCRN_FG_GREEN : SCRN_FG_YELLOW), buf);
		++count;
	}
}

/* Change current location */

void set_loc(char *loc)
{
	if (current_location_code) {
		free(current_location_code);
		free(current_location_name);
	}
	current_location_code = strdup(loc);
	current_location_name = strdup(find_loc(loc));
	update_needed = 1;
}

/* Parse comma quoted line into an array
 * Handles a mix of quoted and non-quoted fields like: 80,"foo bar",fred
 */

int csv(char *s, char **array, int lim)
{
	int x;
	int extra = 0;
	for (x = 0; ;) {
		if (*s == '"') {
			char *o;
			extra = 0;
			++s;
			o = s;
			while (*s && *s != '"')
				++s;
			if (*s == '"') {
				*s++ = 0;
				if (x < lim)
					*array++ = o;
				++x;
				if (*s == ',')
					++s;
				continue;
			}
		} else if (*s || extra) {
			char *o = s;
			extra = 0;
			while (*s && *s != ',')
				++s;
			if (*s == ',') {
				*s++ = 0;
				extra = 1;
				if (x < lim)
					*array++ = o;
				++x;
				continue;
			} else {
				if (x < lim)
					*array++ = o;
				++x;
				continue;
			}
		}
		break;
	}
	*array = 0;
	return x;
}

/* Write a string to the serial port */

void outs(char *s)
{
	while (*s) {
		putcom(*s);
		++s;
	}
}

/* Format a record in csv and send it to the serial port */

void send_rec(struct record *r)
{
	char buf[80];
	sprintf(buf, "\"%d/%d/%4.4d %2.2d:%2.2d:%2.2d\",\"%s\",\"%s\"\r\n",
		r->date.da_mon,
		r->date.da_day,
		r->date.da_year,
		r->time.ti_hour,
		r->time.ti_min,
		r->time.ti_sec,
		r->loc,
		r->item
	);
	outs(buf);
}

#define sz(x) (x), (sizeof(x) - 1)

/* Process a command received from the serial port */

void command(char *cmd)
{
	// strcpy(msg, cmd);
	update_needed = 1;
	if (!strncmp(cmd, sz("ATH"))) { /* Hello */
		outs("OK\r\n");
	} else if (!strncmp(cmd, sz("ATE"))) { /* Erase locations */
		loc_erase();
		outs("OK\r\n");
	} else if (!strncmp(cmd, sz("ATL"))) { /* Add a location */
		char *array[3];
		if (2 == csv(cmd + 3, array, 2)) {
			add_loc(array[0], array[1]);
			loc_cursor_top();
		}
		outs("OK\r\n");
	} else if (!strncmp(cmd, sz("ATV"))) { /* Read or set version number (of locations table) */
		if (cmd[3]) { /* Set version */
			if (cur_version)
				free(cur_version);
			cur_version = strdup(cmd + 3);
		}
		if (cur_version) {
			outs(cur_version);
			outs("\r\n");
		} else {
			outs("No version\r\n");
		}
	} else if (!strncmp(cmd, sz("ATP"))) { /* Read or set prefix */
		if (cmd[3]) { /* Set prefix */
			if (cur_prefix)
				free(cur_prefix);
			cur_prefix = strdup(cmd + 3);
		}
		if (cur_prefix) {
			outs(cur_prefix);
			outs("\r\n");
		} else {
			outs("No version\r\n");
		}
	} else if (!strncmp(cmd, sz("ATD"))) { /* Set date/time */
		int year;
		int month;
		int day;
		int hour;
		int minute;
		int second;
		if (6 == sscanf(cmd + 3,"%d-%d-%d %d:%d:%d",
			&year,&month,&day,
			&hour,&minute,&second)
		) {
			struct date date;
			struct time time;
			date.da_year = year;
			date.da_mon = month;
			date.da_day = day;
			time.ti_hour = hour;
			time.ti_min = minute;
			time.ti_sec = second;
			setdate(&date);
			settime(&time);
		}
		outs("OK\r\n");
	} else if (!strncmp(cmd, sz("ATF"))) { /* Read first scan */
		cursor_top();
		if (cur != records) {
			send_rec(cur);
			cursor_down();
		} else {
			outs("END\r\n");
		}
	} else if (!strncmp(cmd, sz("ATN"))) { /* Read next scan */
		if (cur != records) {
			send_rec(cur);
			cursor_down();
		} else {
			outs("END\r\n");
		}
	} else if (!strncmp(cmd, sz("ATX"))) { /* Erase scans */
		erase();
		outs("OK\r\n");
	} else {
		outs("Huh?\r\n");
	}
}


/* User hit enter: submit item or change location depending on what
 * was entered */

void enter(char *buf)
{
//	if (strlen(buf) < 4) { /* 3 digits or less: assume it's a location */
	if (mode == 1) { /* If we entered it on the location screen it's a location */
		set_loc(buf);
	} else if (current_location_code) { /* 4 digits or more: assume it's an item */
		char bf[80];
		struct date date;
		struct time time;
		getdatetime(&date, &time);
		// sprintf(bf,"%s%s", (cur_prefix ? cur_prefix : ""), buf); /* Prefix item with cur_prefix */
		sprintf(bf, "%s", buf); // We no longer have a prefix...
		if (upc_ok(bf)) {
			// vibrate(15);
			sound(TSTANDARD, VHIGH, SERROR, SPAUSE, SLOW, SPAUSE, SERROR, SPAUSE, SLOW, SPAUSE, SERROR, SPAUSE, SERROR, 0);
			mode = 3;
			update_needed = 1;
		} else {
			add_rec(current_location_code,
				bf,
				date,
				time
			);
		}
	}
}

void main(void)
{
	/* Serial port input buffer */
	char inbuf[80];
	int inbufx = 0;

	/* Repeat for arrow keys */
	unsigned int start_time;
	unsigned int cur_time;
	int repeat = 0; /* 1 = up, 2 = down */
	int repeat_fast = 0; /* set after first repeat */

	scrn_init();

	port = comopen(COM1);

	/* Add some fake locations for now */
	add_loc("00", "Home");
	loc_cursor_top();
	
	/* Main loop.. */
	for (;;) {
	        int ch;

		/* Update screen if necessary */
		if (update_needed) {
			if (mode == 1)
				loc_scrn_gen();
			else if (mode == 0)
				scrn_gen();
			else if (mode == 3)
				error_scrn_gen();
			else
				status_scrn_gen();
			scrn_update();
			update_needed = 0;
		}

		/* Any data from serial port? */
		ch = getcom(0);
		if (ch >= 0) {
			if (ch == '\n' || ch == '\r') {
				inbuf[inbufx] = 0;
				if (inbuf[0])
					command(inbuf);
				inbufx = 0;
			} else if (ch >= 32 && ch <= 126 && inbufx < sizeof(inbuf) - 1) {
				inbuf[inbufx++] = ch;
			}
		}

		/* Any barcodes? */
		if (mode == 0 || mode == 1) {
			char barbuf[80];
			struct barcode barcode;
			unsigned int status;
			barcode.text = barbuf;
			barcode.length = 0;
			barcode.id = 0;
			barcode.min = 2;
			barcode.max = 10;
			status = readbarcode(&barcode);
			if (status == OK) {
				scannerpower(OFF, 0);
				if (mode) {
					/* Switch to cart entry mode in case we were in another mode */
					mode = 0;
					update_needed = 1;
				}
				if (msg[0]) {
					/* Clear top of screen message */
					msg[0] = 0;
					update_needed = 1;
				}
				if (strlen(barbuf) < 4) {
					/* Assume it's a customer */
					set_loc(barbuf);
				} else {
					/* Assume it's a cart */
					if (!upc_ok(barbuf)) {
						/* Good check digit */
						if (current_location_code) {
							struct date date;
							struct time time;
							getdatetime(&date, &time);
							add_rec(current_location_code,
								barbuf,
								date,
								time
							);
						} else {
							/* Indicate bad scan if location was not set */
							sound(TSTANDARD, VHIGH, SERROR, SPAUSE, SLOW, SPAUSE, SERROR, SPAUSE, SLOW, SPAUSE, SERROR, SPAUSE, SERROR, 0);
							mode = 3;
							update_needed = 1;
						}
					} else {
						/* Indicate bad scan */
						sound(TSTANDARD, VHIGH, SERROR, SPAUSE, SLOW, SPAUSE, SERROR, SPAUSE, SLOW, SPAUSE, SERROR, SPAUSE, SERROR, 0);
						mode = 3;
						update_needed = 1;
					}
				}
			}
		}

		/* Arrow key repeat? */
		if (repeat) {
		        int diff;
		        cur_time = GetTickCount();
		        diff = cur_time - start_time;
		        if (diff < 0)
		                diff += 65536;
                        if (repeat == 1) {
                                if (!uppressed()) {
                                        repeat = 0;
                                } else if (repeat_fast ? diff >= 2 : diff >= 12) {
                                        repeat_fast = 1;
                                        start_time = cur_time;
                                        if (mode == 1) {
                                                loc_cursor_up();
                                        } else {
                                                cursor_up();
                                        }
                                }
                        } else if (repeat == 2) {
                                if (!downpressed()) {
                                        repeat = 0;
                                } else if (repeat_fast ? diff >= 2 : diff >= 12) {
                                        repeat_fast = 1;
                                        start_time = cur_time;
                                        if (mode == 1) {
                                                loc_cursor_down();
                                        } else {
                                                cursor_down();
                                        }
                                }
                        }
		}

		/* Any keyboard characters? */
		ch = getchar();
		if (ch != -1)
		        repeat = 0;
		if (mode == 3) { /* Error screen */
			switch (ch) {
				case F4_KEY: { /* Cancel error screen */
					mode = 0;
					update_needed = 1;
					break;
				}
			}
		} else if (mode == 2) { /* Status screen */
			switch (ch) {
				case F1_KEY: case F2_KEY: case TRIGGER_KEY: case ENT_KEY: { /* Cancel location selection */
					mode = 0;
					update_needed = 1;
					break;
				}
			}
		} else if (mode == 1) { /* Location selection mode */
			switch (ch) {
				case F1_KEY: { /* Cancel location selection */
					msg[0] = 0;
					mode = 0;
					update_needed = 1;
					break;
				} case F2_KEY: { /* Status screen */
					msg[0] = 0;
					mode = 2;
					update_needed = 1;
					break;
				} case TRIGGER_KEY: case ENT_KEY: { /* Select new location */
					if (msg[0]) {
						enter(msg);
						msg[0] = 0;
						update_needed = 1;
						mode = 0;
					} else {
						set_loc(loc_cur->code);
						mode = 0;
						update_needed = 1;
					}
					break;
				} case UP_KEY: {
					loc_cursor_up();
					repeat = 1;
					repeat_fast = 0;
					start_time = GetTickCount();
					break;
				} case DOWN_KEY: {
					loc_cursor_down();
					repeat = 2;
					repeat_fast = 0;
					start_time = GetTickCount();
					break;
				} case '0': case '1': case '2': case '3': case '4':
				  case '5': case '6': case '7': case '8': case '9': {
				  	int n = strlen(msg);
				  	if (n < 8) {
				  		msg[n++] = ch;
				  	}
				  	msg[n] = 0;
				  	update_needed = 1;
				  	break;
				} case BS_KEY: {
					int n = strlen(msg);
					if (n) {
						--n;
						msg[n] = 0;
						update_needed = 1;
					}
					break;
				} case CLR_KEY: {
					if (msg[0]) {
						msg[0] = 0;
						update_needed = 1;
					}
					break;
				}
			}
		} else if (mode == 0) { /* Main mode */
			switch (ch) {
				case F1_KEY: { /* Select new location manually */
					mode = 1;
					if (msg[0]) {
						msg[0] = 0;
						update_needed = 1;
					}
					update_needed = 1;
					break;
				} case F2_KEY: { /* Status scree */
					mode = 2;
					if (msg[0]) {
						msg[0] = 0;
						update_needed = 1;
					}
					update_needed = 1;
					break;
				} case UP_KEY: {
					if (msg[0]) {
						msg[0] = 0;
						update_needed = 1;
					}
					cursor_up();
					repeat = 1;
					repeat_fast = 0;
					start_time = GetTickCount();
					break;
				} case '0': case '1': case '2': case '3': case '4':
				  case '5': case '6': case '7': case '8': case '9': {
				  	int n = strlen(msg);
				  	if (n < 8) {
				  		msg[n++] = ch;
				  	}
				  	msg[n] = 0;
				  	update_needed = 1;
				  	break;
				} case ENT_KEY: {
					if (msg[0]) {
						enter(msg);
						msg[0] = 0;
						update_needed = 1;
					}
					break;
				} case DOWN_KEY: {
					if (msg[0]) {
						msg[0] = 0;
						update_needed = 1;
					}
					cursor_down();
					repeat = 2;
					repeat_fast = 0;
					start_time = GetTickCount();
					break;
				} case BS_KEY: {
					int n = strlen(msg);
					if (n) {
						--n;
						msg[n] = 0;
						update_needed = 1;
					}
					break;
				} case CLR_KEY: {
					if (msg[0]) {
						msg[0] = 0;
						update_needed = 1;
					} else {
						del_rec();
					}
					break;
				} case TRIGGER_KEY: {
					if (msg[0]) {
						enter(msg);
						msg[0] = 0;
						update_needed = 1;
					} else {
						/* Start scanning */
						scannerpower(ON, 150);
					}
					break;
				}
			}
		}

		/* Give OS some time */
		idle();
	}
}
