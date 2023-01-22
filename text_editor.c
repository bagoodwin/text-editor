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
};

struct State S;

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

/* Initializes the current state of the text editor.*/
void initState() {
    S.cursor.x = 0;
    S.cursor.y = 0;
    S.screen.rows = LINES - 1;
    S.screen.cols = COLS - 1;
    S.topLine = 0;
    S.numLines = 0;
    S.textSize = 0;
    S.leftPad = digitCount(S.numLines) + 1;
}

/* Appends a line to *lines in state. */
void appendLine(char *line, size_t len) {
    S.lines = realloc(S.lines, S.textSize += len + 1);

    int lineNum = S.numLines;
    S.lines[lineNum].size = len;
    S.lines[lineNum].buf = malloc(len + 1);
    memcpy(S.lines[lineNum].buf, line, len);
    S.lines[lineNum].buf[len] = '\0';
    S.numLines++;
}

/* Loads information from the file into state. */
void openFile(char *filename) {
    FILE *fd = fopen(filename, "r");
    if(!fd) err("Failure to open file.");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while((read = getline(&line, &len, fd)) != -1) {
        while(read > 0 && (line[read - 1] == '\n' || line[read - 1] == '\r')) read--;
        appendLine(line, read);
    }
    
    S.leftPad = digitCount(S.numLines) + 1;
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
        str[digit] = i % 10 + '0';
        digit--;
    }
}

/* Display the appropriate lines on the screen. */
void displayLines() {
    FILE *ef = fopen("error.txt", "w");

    int i;
    int n;
    int screenUsed = 0;
    int charsPrinted = 0;
    char num[S.leftPad];
    char pad[S.leftPad + 1];
    for(i = 0; i < S.screen.rows; i++) {
        fprintf(ef, "iteration");
        if(i >= S.topLine && screenUsed <= S.screen.rows) {
            fprintf(ef, " if");
            /* Create a string leftPad chars long with the beginning filled by the line number and the remainder filled by blank space. */
            memset(pad, ' ', S.leftPad + 1);
            intToStr(i, num);
            strncpy(pad, num, digitCount(i));

            while(charsPrinted <= S.lines[i].size) {
                mvaddstr(screenUsed, 0, pad);
                n = S.screen.cols - sizeof(pad);
                mvaddnstr(screenUsed, sizeof(pad), &(S.lines[i].buf[charsPrinted]), n);
                charsPrinted += n;
                screenUsed++;
                memset(pad, ' ', S.leftPad + 1);
            }   
            charsPrinted = 0;
        }
        fprintf(ef, "\n");
    }
}

/* Display the current file based on the State struct. */
void display() {
    displayLines();
}

/* Gets the keypress and processes it. */
int processKeypress(int c) {
    switch(c) {
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
        display();
        refresh();
        int c = getch();
        if(!processKeypress(c)) break;
    }

    endwin();

    return EXIT_SUCCESS;
}