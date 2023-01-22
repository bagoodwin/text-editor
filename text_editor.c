#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <ncurses.h>

#define CTRL_KEY(x) ((x) & 0x1f)

typedef struct {
    int size;
    char *buf;
} TextLine;

typedef struct {
    int x;
    int y;
} Cursor;

typedef struct {
    int rows;
    int cols;
} Screen;

struct State {
    Cursor cursor;
    Screen screen;
    int topLine;
    int numLines;
    size_t textSize;
    TextLine *lines;
    int leftPad;
    FILE *fd;
};

/* VARS */
struct State S;

/* MISC */

/* Give error message and exit. */
void err(char *str) {
    perror(str);
    endwin();
    exit(EXIT_FAILURE);
}

/* Return the number of digits in the base 10 integer.*/
int digitCount(int n) {
    if(n == 0) return 1;
    int count = 0;
    while(n != 0) {
        n /= 10;
        count++;
    }
    return count;
}

/* Converts int to base 10 string. */
void intToStr(int n, char *str) {
    if(n == 0) {
        str[0] = '0';
    }
    int i;
    int digit = 0;
    for(i = n; i > 0; i /= 10, digit++);
    for(i = n; i > 0; i /= 10) {
        digit--;
        str[digit] = i % 10 + '0';
    }
}

/* CONFIG */

/* Initializes the current state of the text editor.*/
void initState() {
    S.cursor.x = 0;
    S.cursor.y = 0;
    S.screen.rows = LINES - 1;
    S.screen.cols = COLS - 5;
    S.topLine = 0;
    S.numLines = 0;
    S.textSize = 0;
    S.leftPad = digitCount(S.numLines) + 1;
}

/* Appends a line to *lines in state. */
void appendLine(char *line, size_t len) {
    S.lines = realloc(S.lines, sizeof(TextLine) * (S.numLines + 1));

    int lineNum = S.numLines;
    S.lines[lineNum].size = len;
    S.lines[lineNum].buf = malloc(len + 1);
    memcpy(S.lines[lineNum].buf, line, len);
    S.lines[lineNum].buf[len] = '\0';
    S.numLines++;
}

/* Loads information from the file into state. */
void openFile(char *filename) {
    S.fd = fopen(filename, "r");
    if(!S.fd) err("Failure to open file.");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while((read = getline(&line, &len, S.fd)) != -1) {
        while(read > 0 && (line[read - 1] == '\n' || line[read - 1] == '\r')) read--;
        appendLine(line, read);
    }
    
    S.leftPad = digitCount(S.numLines) + 1;
}

/* Scroll the screen if the cursor goes offscreen. */
void scrollScreen() {
    if(S.cursor.y < S.topLine) S.topLine = S.cursor.y;
    if(S.cursor.y >= S.topLine + S.screen.rows) S.topLine = S.cursor.y - S.screen.rows + 1;
}

/* DISPLAY */

/* Display the appropriate lines on the screen. */
void displayLines() {
    int i;
    int n;
    int screenUsed = 0;
    int charsPrinted = 0;
    char num[S.leftPad];
    char pad[S.leftPad + 1];
    for(i = 0; i < S.screen.rows; i++) {
        if(i >= S.topLine && screenUsed <= S.screen.rows && i <= S.numLines) {
            /* Create a string leftPad chars long with the beginning filled by the line number and the remainder filled by blank space. */
            memset(pad, ' ', S.leftPad + 1);
            intToStr(i, num);
            strncpy(pad, num, digitCount(i));


            while(charsPrinted <= S.lines[i].size) {
                attron(A_REVERSE);
                mvaddnstr(screenUsed, 0, pad, sizeof(pad) - 2);
                attroff(A_REVERSE);
                n = S.screen.cols - sizeof(pad);
                if(S.lines[i].size != 0){
                    mvaddnstr(screenUsed, sizeof(pad) - 1, &(S.lines[i].buf[charsPrinted]), n);
                }
                charsPrinted += n;
                screenUsed++;
                memset(pad, ' ', S.leftPad + 1);
            }   
            charsPrinted = 0;
        }
    }
}

/* Display a bottom bar with the file name and cursor position. */
void displayBar(char *name) {
    int i;
    for(i = 0; i < COLS; i++)
        mvaddch(LINES - 2, i, ACS_HLINE);
    
    
    mvaddstr(LINES - 1, 10, name);
    
    int n = COLS - 25;
    mvaddch(LINES - 1, n, '(');
    char str[10];
    memset(str, ' ', 10);
    intToStr(S.cursor.y, str);
    addnstr(str, digitCount(S.cursor.y));
    addch(',');
    intToStr(S.cursor.x, str);
    addnstr(str, digitCount(S.cursor.x));
    addch(')');
}

/* Display the current file based on the State struct. */
void display(char *name) {
    displayLines();
    displayBar(name);
}

/* KEY PROCESSOR */

/* Gets the keypress and processes it. */
int processKeypress(int c) {
    switch(c) {
        case KEY_LEFT:
            if(S.cursor.x > 0) S.cursor.x--;
            break;
        case KEY_RIGHT:
            if(S.cursor.x < S.screen.cols) S.cursor.x++;
            break;
        case KEY_UP:
            if(S.cursor.y > 0) S.cursor.y--;
            break;
        case KEY_DOWN:
            if(S.cursor.y < S.screen.rows) S.cursor.y++;
            break;
        case CTRL_KEY('q'):
        /* Exit program. */
            return 0;
            break;
        default:
            addch(c);
            break;  
    }

    return 1;
}

int main(int argc, char **argv) {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    nl();

    initState();
    openFile(argv[argc - 1]);

    while(1) {
        scrollScreen();
        display(argv[argc - 1]);
        move(S.cursor.y, S.cursor.x + S.leftPad);
        refresh();
        int c = getch();
        if(!processKeypress(c)) break;
    }

    fclose(S.fd);
    endwin();

    return EXIT_SUCCESS;
}