build:
	gcc -Wall -Wextra -pedantic-errors text_editor.c -o text_editor -lncurses
clean:
	rm text_editor