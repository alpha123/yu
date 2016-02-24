#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <ncurses.h>

#define WINHEIGHT 4

typedef struct {
    WINDOW *win;
    uint32_t winnum;
    char *in;
    char *out;
} t_win;

t_win *create_window(uint32_t yloc, uint32_t winnum, const char *in, const char *out) {
    WINDOW *win = newwin(WINHEIGHT, COLS, yloc, 0);
    t_win *twin = calloc(1, sizeof(t_win));
    box(win, 0, 0);
    wrefresh(win);
    mvprintw(yloc + 1, 1, "IN [%d]: %s", winnum, in);
    if (out)
        mvprintw(yloc + 2, 1, "OUT [%d]: %s", winnum, out);
    refresh();
    twin->win = win;
    twin->winnum = winnum;
    twin->in = in;
    twin->out = out;
    return twin;
}

int main(int argc, char **argv) {
    uint32_t yloc = 0, winnum = 1;
    int ch;

    initscr();
    start_color();
    cbreak();
    keypad(stdscr, true);
    noecho();
    refresh();

    while ((ch = getch()) != KEY_UP) switch (ch) {
    case KEY_DOWN:
        create_window(yloc, winnum, "", NULL);
        yloc += WINHEIGHT;
        ++winnum;
        break;
    }

    endwin();
    return 0;
}
