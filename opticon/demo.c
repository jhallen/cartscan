#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Serial port handle */

int port;
int port_byte;

/* Screen size */

#define SCRN_WIDTH 20
#define SCRN_HEIGHT 13

/* Number of database lines which fit on the screen */
#define DB_LINES (SCRN_HEIGHT - 2)

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

/* Draw text */
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


/* Customer code to customer name table */

struct customer {
	struct customer *next, *prev;
	char *code;
	char *name;
};

struct customer customers[1] = { { customers, customers, "End of file" } };
struct customer *cust_top = customers;
struct customer *cust_cur = customers;

void add_cust(char *code, char *name)
{
	struct customer *cust = (struct customer *)malloc(sizeof(struct customer));
	int empty = (cust_top == cust_cur);
	cust->code = strdup(code);
	cust->name = strdup(name);
	cust->next = customers;
	cust->prev = customers->prev;
	customers->prev->next = cust;
	customers->prev = cust;
	if (empty) {
		cust_top = cust;
	}
	cust_cur = cust->next;
	update_needed = 1;
}

char *find_cust(char *code)
{
	struct customer *cust;
	for (cust = customers->next; cust != customers; cust = cust->next)
		if (!strcmp(cust->code, code))
			return cust->name;
	return "<unknown>";
}

/* Customer cursor top */

void cust_cursor_top()
{
	cust_cur = customers->next;
	cust_top = customers->next;
	update_needed = 1;
}

/* Customer cursor up */

void cust_cursor_up()
{
	if (cust_cur->prev != customers) {
		if (cust_cur == cust_top)
			cust_top = cust_cur;
		cust_cur = cust_cur->prev;
		update_needed = 1;
	}
}

/* Customer cursor down */

void cust_cursor_down()
{
	if (cust_cur->next != customers) {
		cust_cur = cust_cur->next;
		update_needed = 1;
	}
}

/* Erase all customers */

void cust_erase()
{
	while (customers->next != customers) {
		struct customer *r = customers->next;
		r->next->prev = r->prev;
		r->prev->next = r->next;
		free(r->code);
		free(r->name);
		free(r);
	}
	cust_cursor_top();
}

/* Generate customer screen */

void cust_scrn_gen()
{
	struct customer *r;
	int count = 0;
	scrn_clr();
	scrn_text(0, 0, 0, "Select location");
	scrn_gsep(1);
	/* Follow cursor */
	/* How many lines between cursor and top? */
	count = 0;
	for (r = cust_cur; r != cust_top; r = r->prev) {
		++count;
		if (count == DB_LINES) {
			break;
		}
		if (r->prev == customers)
			break;
	}
	if (r != cust_top)
		cust_top = r;
	/* Print lines */
	count = 0;
	for (r = cust_top; r != customers && count != DB_LINES; r = r->next) {
		char buf[80];
		sprintf(buf, "%2s %s", r->code, r->name);
		if (r == cust_cur)
			scrn_text(2 + count, 0, SCRN_INVERSE, buf);
		else
			scrn_text(2 + count, 0, 0, buf);
		++count;
	}
}


/* Scan events */

struct record {
	struct record *next, *prev;
	char *cust; /* Customer code */
	char *cart; /* Cart code */
	struct date date;
	struct time time;
};

struct record database[1] = { { database, database, "End of file" } };
struct record *top = database; /* First line on screen */
struct record *cur = database; /* Line with cursor */

/* Add record to end of database */

void add_rec(char *cust, char *cart, struct date date, struct time time)
{
	struct record *r = (struct record *)malloc(sizeof(struct record));
	int empty = (top == database);
	r->cust = strdup(cust);
	r->cart = strdup(cart);
	r->date = date;
	r->time = time;
	r->next = database;
	r->prev = database->prev;
	database->prev->next = r;
	database->prev = r;
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
	if (cur != database) {
		struct record *r = cur;
		r->next->prev = r->prev;
		r->prev->next = r->next;
		cur = r->next;
		if (top == r)
			top = r->next;
		free(r->cust);
		free(r->cart);
		free(r);
		update_needed = 1;
	}
}

/* Cursor top */

void cursor_top()
{
	cur = database->next;
	top = database->next;
	update_needed = 1;
}

/* Cursor up */

void cursor_up()
{
	if (cur->prev != database) {
		if (cur == top)
			top = cur->prev;
		cur = cur->prev;
		update_needed = 1;
	}
}

/* Cursor down */

void cursor_down()
{
	if (cur != database) {
		cur = cur->next;
		update_needed = 1;
	}
}

/* Erase database */

void erase()
{
	while (database->next != database) {
		struct record *r = database->next;
		r->next->prev = r->prev;
		r->prev->next = r->next;
		free(r->cust);
		free(r->cart);
		free(r);
	}
	cursor_top();
}

/* Generate screen */

char *current_customer_code = 0;
char *current_customer_name = 0;

char msg[80];

void scrn_gen()
{
	struct record *r;
	int count = 0;
	scrn_clr();
	if (msg[0]) {
		scrn_text(0, 0, 0, msg);
	} else if (current_customer_code) {
		char buf[80];
		sprintf(buf, "%s %s", current_customer_code, current_customer_name);
		if (current_customer_code && atoi(current_customer_code) < 10)
			scrn_text(0, 0, SCRN_INVERSE + SCRN_FG_GREEN, buf);
		else
			scrn_text(0, 0, SCRN_INVERSE + SCRN_FG_YELLOW, buf);
	} else {
		scrn_text(0, 0, 0, "<Location>");
	}
//	if (port_byte) {
//		char buf[80];
//		sprintf(buf, "%2.2x", port_byte);
//		scrn_text(0, SCRN_WIDTH - 2, 0, buf);
//	}
	scrn_gsep(1);
//	if (msg[0]) {
//		scrn_text(1, 0, 0, msg);
//	}
	/* Follow cursor */
	/* How many lines between cursor and top? */
	count = 0;
	for (r = cur; r != top; r = r->prev) {
		++count;
		if (count == DB_LINES) {
			break;
		}
		if (r->prev == database)
			break;
	}
	if (r != top)
		top = r;
	/* Print lines */
	count = 0;
	for (r = top; r != database && count != DB_LINES; r = r->next) {
		char buf[80];
		sprintf(buf, "%2d/%2d %+8s %+5s", r->date.da_mon, r->date.da_day, r->cust, r->cart);
		if (r == cur)
			scrn_text(2 + count, 0, SCRN_INVERSE + ((atoi(r->cust) < 10) ? SCRN_FG_GREEN : SCRN_FG_YELLOW), buf);
		else
			scrn_text(2 + count, 0, ((atoi(r->cust) < 10) ? SCRN_FG_GREEN : SCRN_FG_YELLOW), buf);
		++count;
	}
}

void set_cust(char *cust)
{
	if (current_customer_code) {
		free(current_customer_code);
		free(current_customer_name);
	}
	current_customer_code = strdup(cust);
	current_customer_name = strdup(find_cust(cust));
	update_needed = 1;
}

int mode; /* 0 = main screen, 1 = customer selection screen */

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


void outs(char *s)
{
	while (*s) {
		putcom(*s);
		++s;
	}
}

#define sz(x) (x), (sizeof(x) - 1)

void command(char *cmd)
{
	// strcpy(msg, cmd);
	update_needed = 1;
	if (!strncmp(cmd, sz("ATH"))) {
		outs("OK\r\n");
	} else if (!strncmp(cmd, sz("ATE"))) {
		cust_erase();
		outs("OK\r\n");
	} else if (!strncmp(cmd, sz("ATL"))) {
		char *array[3];
		if (2 == csv(cmd + 3, array, 2)) {
			add_cust(array[0], array[1]);
			cust_cursor_top();
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
		char buf[80];
		cursor_top();
		if (cur != database) {
			sprintf(buf, "\"%d/%d/%4.4d %2.2d:%2.2d:%2.2d\",\"%s\",\"%s\"\r\n",
				cur->date.da_mon,
				cur->date.da_day,
				cur->date.da_year,
				cur->time.ti_hour,
				cur->time.ti_min,
				cur->time.ti_sec,
				cur->cust,
				cur->cart
			);
			outs(buf);
			cursor_down();
		} else {
			outs("END\r\n");
		}
	} else if (!strncmp(cmd, sz("ATN"))) {
		char buf[80];
		if (cur != database) {
			sprintf(buf, "\"%d/%d/%4.4d %2.2d:%2.2d:%2.2d\",\"%s\",\"%s\"\r\n",
				cur->date.da_mon,
				cur->date.da_day,
				cur->date.da_year,
				cur->time.ti_hour,
				cur->time.ti_min,
				cur->time.ti_sec,
				cur->cust,
				cur->cart
			);
			outs(buf);
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

void main(void)
{
	char inbuf[80];
	int inbufx = 0;
	int x;

	/* Add some fake customers for now */
	add_cust("00", "Home");
	cust_cursor_top();

	scrn_init();

	port = comopen(COM1);
	
	for (;;) {
		if (mode == 1) {
			if (update_needed) {
				cust_scrn_gen();
				scrn_update();
				update_needed = 0;
			}
			x = getchar();
			switch (x) {
				case F1_KEY: { /* Cancel customer selection */
					mode = 0;
					update_needed = 1;
					break;
				} case TRIGGER_KEY: case ENT_KEY: { /* Select new customer */
					set_cust(cust_cur->code);
					mode = 0;
					update_needed = 1;
					break;
				} case UP_KEY: {
					cust_cursor_up();
					break;
				} case DOWN_KEY: {
					cust_cursor_down();
					break;
				} default: {
					break;
				}
			}
			idle();
		} else {
			if (update_needed) {
				scrn_gen();
				scrn_update();
				update_needed = 0;
			}
			x = getchar();
			switch (x) {
				case F1_KEY: { /* Select new customer manually */
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
					break;
				} case '0': case '1': case '2': case '3': case '4':
				  case '5': case '6': case '7': case '8': case '9': {
				  	int n = strlen(msg);
				  	if (n == 0) {
				  		msg[0] = 'M';
				  		msg[1] = 0;
				  		n = 1;
				  	}
				  	if (n < 8) {
				  		msg[n++] = x;
				  	}
				  	msg[n] = 0;
				  	update_needed = 1;
				  	break;
				} case ENT_KEY: {
					if (msg[0] && current_customer_code) {
						struct date date;
						struct time time;
						getdatetime(&date, &time);
						add_rec(current_customer_code,
							msg,
							date,
							time
						);
					}
					msg[0] = 0;
					update_needed = 1;
					break;
				} case DOWN_KEY: {
					if (msg[0]) {
						msg[0] = 0;
						update_needed = 1;
					}
					cursor_down();
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
					del_rec();
					break;
				} case TRIGGER_KEY: {
					if (msg[0]) {
						if (msg[0] && current_customer_code) {
							struct date date;
							struct time time;
							getdatetime(&date, &time);
							add_rec(current_customer_code,
								msg,
								date,
								time
							);
						}
						msg[0] = 0;
						update_needed = 1;
					} else {
						scannerpower(ON, 150);
					}
					break;
				} default: {
					/* Any data from serial port? */
					x = getcom(0);
					if (x >= 0) {
						if (x == '\n' || x == '\r') {
							inbuf[inbufx] = 0;
							if (inbuf[0])
								command(inbuf);
							inbufx = 0;
						} else if (x >= 32 && x <= 126 && inbufx < sizeof(inbuf) - 1) {
							inbuf[inbufx++] = x;
						}
					} else {
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
								set_cust(barbuf);
							} else if (barbuf[0] >= 'A' && barbuf[0] <= 'Z') {
								if (current_customer_code) {
									struct date date;
									struct time time;
									getdatetime(&date, &time);
									add_rec(current_customer_code,
										barbuf,
										date,
										time
									);
								}
							}
							
						}
					}
					break;
				}
			}
			idle();
		}
	}
}
