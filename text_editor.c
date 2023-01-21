#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ncurses.h>

#define CTRL_KEY(x) ((x) & 0x1f)

/* Display the current file based on the State struct. */
void display() {

}

/* Gets the keypress and processes it. */
int processKeypress(int c) {
    switch(c) {
        case CTRL_KEY('q'):
        /* Exit program. */
            printw("ligma\n");
            return 0;
            break;
        default:
            addch(c);
            break;  
    }

    return 1;
}

int main() {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    nl();

    while(1) {
        //display();
        refresh();
        int c = getch();
        if(!processKeypress(c)) break;
    }

    endwin();

    return EXIT_SUCCESS;
}