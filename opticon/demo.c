/* Item tracking application for Opticon OPH-1005 / OPH-3001
 *
 * by: Joseph Allen
 */

#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Serial port handle */

int port;

/* Screen size */

#define SCRN_WIDTH 20
#define SCRN_HEIGHT 13

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

#define SCRN_BG_WHITE (7 << SCRN_BG_SCRN) 
#define SCRN_BG_BLUE (6 << SCRN_BG_SCRN)
#define SCRN_BG_GREEN (5 << SCRN_BG_SCRN)
#define SCRN_BG_RED (4 << SCRN_BG_SCRN)
#define SCRN_BG_MAGENTA (3 << SCRN_BG_SCRN)
#define SCRN_BG_CYAN (2 << SCRN_BG_SCRN)
#define SCRN_BG_YELLOW (1 << SCRN_BG_SCRN)
#define SCRN_BG_BLACK (0 << SCRN_BG_SCRN)

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

char *find_loc(char *code)
{
	struct location *loc;
	for (loc = locations->next; loc != locations; loc = loc->next)
		if (!strcmp(loc->code, code))
			return loc->name;
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

void loc_scrn_gen()
{
	struct location *r;
	int count = 0;
	scrn_clr();
	scrn_text(0, 0, 0, "Select location");
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
	sound(TSTANDARD, VSTANDARD, SLOW, 0);
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

/* Message to display at top of screen if msg[0] != 0 */
char msg[80];

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
		sprintf(buf, "%2d/%2d %+8s %+5s", r->date.da_mon, r->date.da_day, r->loc, r->item);
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
	if (!strncmp(cmd, sz("ATH"))) {
		outs("OK\r\n");
	} else if (!strncmp(cmd, sz("ATE"))) {
		loc_erase();
		outs("OK\r\n");
	} else if (!strncmp(cmd, sz("ATL"))) {
		char *array[3];
		if (2 == csv(cmd + 3, array, 2)) {
			add_loc(array[0], array[1]);
			loc_cursor_top();
		}
		outs("OK\r\n");
	} else if (!strncmp(cmd, sz("ATD"))) {
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
	} else if (!strncmp(cmd, sz("ATF"))) {
		cursor_top();
		if (cur != records) {
			send_rec(cur);
			cursor_down();
		} else {
			outs("END\r\n");
		}
	} else if (!strncmp(cmd, sz("ATN"))) {
		if (cur != records) {
			send_rec(cur);
			cursor_down();
		} else {
			outs("END\r\n");
		}
	} else if (!strncmp(cmd, sz("ATX"))) {
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
	if (strlen(buf) < 4) { /* 3 digits or less: assume it's a location */
		set_loc(buf);
	} else if (current_location_code) { /* 4 digits or more: assume it's an item */
		char bf[80];
		struct date date;
		struct time time;
		getdatetime(&date, &time);
		sprintf(bf,"M%s", buf); /* Prefix item with 'M' */
		add_rec(current_location_code,
			bf,
			date,
			time
		);
	}
}

/* Display mode */
int mode; /* 0 = main screen, 1 = location selection screen */

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
			else
				scrn_gen();
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
		if (mode == 0) {
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
				if (msg[0]) {
					msg[0] = 0;
					update_needed = 1;
				}
				scannerpower(OFF, 0);
				sound(TSTANDARD, VSTANDARD, SHIGH, 0);
				if (barbuf[0] >= '0' && barbuf[0] <= '9') {
					/* If it begins with a number assume it's a location */
					set_loc(barbuf);
				} else if (barbuf[0] >= 'A' && barbuf[0] <= 'Z') {
					/* If it begins with a letter assume it's an item */
					if (current_location_code) {
						struct date date;
						struct time time;
						getdatetime(&date, &time);
						add_rec(current_location_code,
							barbuf,
							date,
							time
						);
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
		if (mode == 1) { /* Location selection mode */
			switch (ch) {
				case F1_KEY: { /* Cancel location selection */
					mode = 0;
					update_needed = 1;
					break;
				} case TRIGGER_KEY: case ENT_KEY: { /* Select new location */
					set_loc(loc_cur->code);
					mode = 0;
					update_needed = 1;
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
				}
			}
		} else { /* Main mode */
			switch (ch) {
				case F1_KEY: { /* Select new location manually */
					mode = 1;
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
