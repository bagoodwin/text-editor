# text-editor
A basic text editor written in C with ncurses.

# Usage
``./text_editor [input file]``

# Make
``make`` - compiles
``make clean`` - removes compiled code

# About
This text editor allows for basic file editing and should look something like a less feature rich nano. Features include line numbering, a bottom bar telling cursor position, saving to different file locations, handling wide characters (tabs), snapping to the end of lines while scrolling to avoid going out of bounds, and wrapping lines. A test file is included to open and try viewing and editing.
