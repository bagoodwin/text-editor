#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <setjmp.h>
#include <ncurses.h>

#define CTRL_KEY(x) ((x) & 0x1f)
#define TAB_WIDTH 8

typedef struct {
    int size;
    char *buf;
    int dsize;
    char *dbuf;
} TextLine;

typedef struct {
    int x;
    int y;
} Cursor;

typedef struct {
    int numLines;
    TextLine *lines;
} Data;

struct State {
    Cursor cursor;
    Data data;
    int topLine;
    int leftWidth;
    FILE *fd;
    char *fileName;
};

/* VARS */

/* MISC */

/* Frees blocks allocated in data. */
void freeData(Data *data) {
    int i;
    for(i = 0; i < data->numLines; i++) {
        free(data->lines[i].buf);
        free(data->lines[i].dbuf);
    }
    free(data->lines);
}

/* Give error message and exit. */
void errorExit(Data *data, FILE *fd, char *str) {
    perror(str);
    fclose(fd);
    endwin();
    freeData(data);
    exit(EXIT_FAILURE);
}

/* Exit normally. */
void normalExit(Data *data, FILE *fd) {
    fclose(fd);
    endwin();
    freeData(data);
    exit(EXIT_SUCCESS);    
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
void initState(struct State *S) {
    S->cursor.x = 0;
    S->cursor.y = 0;
    S->data.numLines = 0;
    S->topLine = 0;
}

/* Creates a display line from its corresponding logical line. */
void createDisplayLine(TextLine *line) {
    // Get number of tabs
    int tabCount = 0;
    int i;
    for(i = 0; i < line->size; i++) {
        if(line->buf[i] == '\t') tabCount++;
    }

    // Set display line
    free(line->dbuf);
    line->dsize = line->size + (TAB_WIDTH - 1) * tabCount;
    line->dbuf = malloc(line->dsize + 1);
    int dindex = 0;
    int j;
    for(i = 0; i < line->size; i++) {
        if(line->buf[i] == '\t') {
            for(j = 0; j < TAB_WIDTH; j++) {
                line->dbuf[dindex] = ' ';
                dindex++;
            }  
        } else {
            line->dbuf[dindex] = line->buf[i];
            dindex++;

        }
    }
    
    line->dbuf[dindex] = '\0';
}

/* Insert a line in *lines at y. */
void insertLine(char *line, size_t len, Data *data, int y) {
    // Update amount of space allocated to the lines array.
    data->lines = realloc(data->lines, sizeof(TextLine) * (data->numLines + 1));
    memmove(&data->lines[y + 1], &data->lines[y], sizeof(TextLine) * (data->numLines - y));

    // Add line to position y in lines array
    data->lines[y].size = len;
    data->lines[y].buf = malloc(len + 1);
    memcpy(data->lines[y].buf, line, len);
    data->lines[y].buf[len] = '\0';

    // Make sure there is something to be freed.
    data->lines[y].dbuf = NULL;
    createDisplayLine(&data->lines[y]);

    data->numLines++;
}

/* Loads information from the file into state. */
void readFile(FILE *fd, Data *data) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while((read = getline(&line, &len, fd)) != -1) {
        while(read > 0 && (line[read - 1] == '\n' || line[read - 1] == '\r')) read--;
        insertLine(line, read, data, data->numLines);
    }
}

/* Scroll the screen if the cursor goes offscreen. */
void scrollScreen(Cursor *cursor, int *topLine) {
    if(cursor->y < (*topLine)) (*topLine)--;
    // - 2 to account for bottom bar
    if(cursor->y >= (*topLine) + LINES - 2) (*topLine)++;
}

/* DISPLAY */

/* Calculate x position in line accounting for tabs. */
int xPos(Data *data, Cursor *cursor) {
    int i;
    int dx = 0;
    for(i = 0; i < cursor->x; i++) {
        if(data->lines[cursor->y].buf[i] == '\t') dx += TAB_WIDTH;
        else dx++;
    }
    return dx;
}

/* Display the appropriate lines on the screen. */
void displayLines(Data *data, int topLine, int leftWidth) {
    int i;
    int n;
    int screenUsed = 0;
    int charsPrinted = 0;
    char num[leftWidth];
    char pad[leftWidth + 1];
    /* Iterate through each line of data starting from the topLine and print it if there's room */
    for(i = topLine; i < data->numLines; i++) {
        if(screenUsed <= LINES - 2 && i <= data->numLines) {
            /* Create a string with the beginning filled by the line number and the remainder filled by blank space. */
            memset(pad, ' ', leftWidth + 1);
            intToStr(i, num);
            strncpy(pad, num, digitCount(i));

            /* Prints line number followed by the line data, which will wrap to the next line until all has been printed. */
            while(charsPrinted <= data->lines[i].dsize) {
                attron(A_REVERSE);
                mvaddnstr(screenUsed, 0, pad, leftWidth);
                attroff(A_REVERSE);
                n = COLS - leftWidth - 1;
                if(data->lines[i].size != 0){
                    mvaddnstr(screenUsed, leftWidth + 1, &(data->lines[i].dbuf[charsPrinted]), n);
                }
                charsPrinted += n;
                screenUsed++;
                memset(pad, ' ', leftWidth + 1);
            }   
            charsPrinted = 0;
        } else {
            break;
        }
    }
}

/* Display a bottom bar with the file name and cursor position. */
void displayBar(Data *data, Cursor *cursor, char *fileName) {
    // Draw horizontal line
    int i;
    for(i = 0; i < COLS; i++)
        mvaddch(LINES - 2, i, ACS_HLINE);

    // Print file name
    mvaddstr(LINES - 1, 10, fileName);
    
    // Calculate x position including tabs
    // TODO
    int x = xPos(data, cursor);

    // Print cursor position
    int n = COLS - 25;
    mvaddch(LINES - 1, n, '(');
    char str[10];
    memset(str, ' ', 10);
    intToStr(cursor->y, str);
    addnstr(str, digitCount(cursor->y));
    addch(',');
    intToStr(x, str);
    addnstr(str, digitCount(x));
    addch(')');
}

/* Move the cursor to its correct position on the screen. */
void displayCursor(Data *data, Cursor *cursor, int leftWidth, int topLine) {
    int lineWidth = COLS - leftWidth - 1;
    /* The number of spaces taken up by line numbers + cursor coords. */
    int i;
    int dx = xPos(data, cursor);

    int x = dx % lineWidth + leftWidth + 1;
    /* The screen row the cursor should appear on. */
    int y = dx / lineWidth;
    /* Sums the lines of height each text line takes up.*/
    for(i = topLine; i < cursor->y; i++) {
        y += 1 + data->lines[i].dsize / lineWidth;
    }
    move(y, x);
}

/* Display the current file based on the State struct. */
void display(struct State *S) {
    erase();
    S->leftWidth = digitCount(S->data.numLines) + 1;
    displayLines(&S->data, S->topLine, S->leftWidth);
    displayBar(&S->data, &S->cursor, S->fileName);
    displayCursor(&S->data, &S->cursor, S->leftWidth, S->topLine);
}

/* KEY PROCESSOR */

/* Move the cursor based on input. */
void moveCursor(struct State *S, int c) {
    switch(c) {
        case KEY_LEFT:
            if(S->cursor.x > 0) S->cursor.x--;
            else if(S->cursor.y > 0) {
                // Wrap to end of previous line.
                S->cursor.y--;
                S->cursor.x = S->data.lines[S->cursor.y].size;
            }
            break;
        case KEY_RIGHT:
            if(S->cursor.x < S->data.lines[S->cursor.y].size) S->cursor.x++;
            else if(S->cursor.y < S->data.numLines) {
                // Wrap to start of next line.
                S->cursor.y++;
                S->cursor.x = 0;
            }
            break;
        case KEY_UP:
            if(S->cursor.y > 0) {
                S->cursor.y--;
                // Realign cursor if beyond bounds
                if(S->cursor.x >= S->data.lines[S->cursor.y].size) S->cursor.x = S->data.lines[S->cursor.y].size;
            }
            break;
        case KEY_DOWN:
            if(S->cursor.y < S->data.numLines) {
                S->cursor.y++;
                // Realign cursor if beyond bounds
                if(S->cursor.x >= S->data.lines[S->cursor.y].size) S->cursor.x = S->data.lines[S->cursor.y].size;
            }
            break;
    }
}

/* Process Page Up and Page Down keys. */
void movePage(struct State *S, int c) {
    int lastPage;
    int i;
    switch(c) {
        case KEY_PPAGE:
        /* Page Up */
            for(i = 0; i < LINES - 3; i++) {
                moveCursor(S, KEY_UP);
                if(S->topLine > 0) S->topLine--;
            }
            break;
        case KEY_NPAGE:
        /* Page Down */
            if(S->data.numLines - S->topLine < LINES - 2 ) lastPage = 1;
            else lastPage = 0;
            for(i = 0; i < LINES - 3; i++) {
                moveCursor(S, KEY_DOWN);
                if(S->topLine < S->data.numLines && !lastPage) S->topLine++;
            }
            break;
    }
}
/* Join line y + 1 to the end of line y. */
void joinLines(Data *data, int y) {
    // Resize y's buf to it's size + the size of the line below it.
    data->lines[y].buf = realloc(data->lines[y].buf, data->lines[y].size + data->lines[y + 1].size + 1);
    // Copy y + 1 to the end of y
    memcpy(&data->lines[y].buf[data->lines[y].size], data->lines[y + 1].buf, data->lines[y + 1].size);
    data->lines[y].size += data->lines[y + 1].size;
    data->lines[y].buf[data->lines[y].size] = '\0';
}

/* Removes the yth line from data and discards it. */
void removeLine(Data *data, int y) {
    free(data->lines[y].buf);
    free(data->lines[y].dbuf);
    // Shift lines back to overwrite the freed line
    if(y < data->numLines)
        memmove(&data->lines[y], &data->lines[y + 1], sizeof(TextLine) * (data->numLines - y - 1));
    data->numLines--;
    // Resize lines to account for one fewer line
    data->lines = realloc(data->lines, sizeof(TextLine) * (data->numLines + 1));
}

/* Delete the character at the cursor position. */
void deleteChar(Data *data, Cursor *cursor, int y, int x) {
    if(cursor->x == 0 && cursor->y == 0) {
        /* Backspace at 0,0 */
        cursor->x++;
        return;
    }
    if(y == data->numLines) {
        /* Cursor is on the final line */
        if(x == -1) {
            cursor->x = data->lines[y - 1].size + 1;
            cursor->y = y - 1;
        }
        return;
    }
    if(x == -1 && y > 0) {
        /* Backspace at beginning of line */
        cursor->x = data->lines[y - 1].size + 1;
        cursor->y = y - 1;
        joinLines(data, y - 1);
        removeLine(data, y);
        createDisplayLine(&data->lines[y - 1]);
        return;
    }
    if(x == data->lines[y].size) {
        /* Delete at EOL */
        joinLines(data, y);
        removeLine(data, y + 1);
        createDisplayLine(&data->lines[y]);
        return;
    }

    /* Delete in line */
    memmove(&data->lines[y].buf[x], &data->lines[y].buf[x + 1], data->lines[y].size - x);
    data->lines[y].size--;

    createDisplayLine(&data->lines[y]);
}

/* Enter creates a new line with the string to the right of the cursor */
void enter(Data *data, Cursor *cursor) {
    if(cursor->x == 0) {
        /* Handles case at EOF */
        insertLine("", 0, data, cursor->y);
        cursor->y++;
        return;
    }
    TextLine *line = &data->lines[cursor->y];
    insertLine(&line->buf[cursor->x], line->size - cursor->x, data, cursor->y + 1);
    line->size = cursor->x;
    line->buf[cursor->x] = '\0';
    line->buf = realloc(line->buf, line->size + 1);
    createDisplayLine(line);
    cursor->y++;
    cursor->x = 0;
}

/* Insert a character at the cursor position. */
void insertChar(TextLine *line, int x, int c) {
    // Update line
    line->buf = realloc(line->buf, line->size + 2);
    // Moves the section of the line after x forwards one space
    memmove(&line->buf[x + 1], &line->buf[x], line->size - x + 1);  
    line->buf[x] = c;
    line->size++;
    
    createDisplayLine(line);
}

/* Gets the keypress and processes it. */
int processKeypress(struct State *S, int c) {
    switch(c) {
        /* MOVEMENT */
        case KEY_LEFT:
        case KEY_RIGHT:
        case KEY_UP:
        case KEY_DOWN:
            moveCursor(S, c);
            break;
        case KEY_PPAGE:
        case KEY_NPAGE:
            movePage(S, c);
            break;
        case KEY_HOME:
            S->cursor.x = 0;
            break;
        case KEY_END:
            S->cursor.x = S->data.lines[S->cursor.y].size;
            break;
        case CTRL_KEY('q'):
        /* Exit program. */
            normalExit(&S->data, S->fd);
            break;
        /* EDITING */
        case KEY_DC:
        /* Delete character. */
            deleteChar(&S->data, &S->cursor, S->cursor.y, S->cursor.x);
            break;
        case KEY_BACKSPACE:
            deleteChar(&S->data, &S->cursor, S->cursor.y, S->cursor.x - 1);
            S->cursor.x--;
            break;
        case 10:
        case KEY_ENTER:
            enter(&S->data, &S->cursor);
            break;
        default:
            if(S->cursor.y == S->data.numLines) insertLine("", 0, &S->data, S->data.numLines);
            insertChar(&S->data.lines[S->cursor.y], S->cursor.x, c);
            S->cursor.x++;
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

    struct State S;

    initState(&S);
    S.fileName = argv[argc - 1];
    S.fd = fopen(S.fileName, "r");
    readFile(S.fd, &S.data);

    while(1) {
        scrollScreen(&S.cursor, &S.topLine); // TODO: integrate into display
        display(&S);
        refresh();
        int c = getch();
        if(!processKeypress(&S, c)) break;
    }
}