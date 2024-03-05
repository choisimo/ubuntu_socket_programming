#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

void main() {

	char key;
	char *ptr;
	ptr = (char*) malloc(sizeof(char) * 100);

	int index = 0;

	/*curses library init*/
	initscr();

	printw("input any key, if want to exit, insert 'q' ");
 	while((key = getch()) != 'q'){
		ptr[index] = key;
		index ++;
		addch(key);
		refresh();
	}
	ptr[index] = '\0';
	printw("%s ", ptr);
	refresh();

	free(ptr);

	/*end curses*/
	endwin();
}
