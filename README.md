# text-editor
A basic text editor written in C with Ncurses.

# Usage
``./text_editor [input file]``

# Make
``make`` - compiles
``make clean`` - removes compiled code

# About
This text editor is unfinished and should not be used yet. It may not handle all inputs properly and saving is currently very buggy. The code is currently all stored in one file due to not knowing how it would be organized before beginning, but will be separated out into several source functions to improve readability when it is more usable.
[This](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/) Ncurses guide, the Ncurses man pages, and [this](https://github.com/snaptoken/kilo-tutorial) text editor guide have all been very helpful. The text editor guide was mostly used as a list of features to implement, but a few small sections (mostly related to manipulating strings and string arrays) are near copies.
