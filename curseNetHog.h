/* MIT License
 * 
 * Copyright (c) 2017 CowboyCoderAAS
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Albert Andrew Spencer
 * 11/6/15
 * header file for the ncurses for piggy
 */ 

// Include files
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h> // used for debuging purposes
#include <unistd.h>
#include <wchar.h> // used for finding the escape key
#include <ncurses.h> // maccros for the types of keys
#include <stddef.h> // has stuff on wchar_t
#include <locale.h>
#include <signal.h>


// constants used throughout the different programs
#define DEBUG 0 // will print debug statments when true
#define DEFAULTPORT 36758 // default port number if none others specified
#define QLEN 6 // the default queue length for lissening
#define BUFFSIZE 4000 // the buffer size for the string 4k for optimal file reading
#define MINROWS 43 // minimum values I will work with when creating windows
#define MINCOLS 132
#define ERRORMODE 0 // used to funnal all statments of printfs to fprintf(stderr)
#define LOGMODE 0 // will log all printfs to a text file
#define LOGNAME "errorLog.txt"
// structs and enumes

// the specific window names for the different windows
enum WINDOWNAME
{
	WSTDIN = 0, 
	INLEFT,
	OUTRIGHT,
	INRIGHT,
	OUTLEFT,
} wNames;

enum CHARCOLORS
{
	LOOPC = 2, // start at 2 since 0 and 1 reserved for black and white
	LISTENC = 3,
	NOCONNECTC = 4,
	CONNECTC = 5,
	FILTERC = 6,
	LOGC = 7,
	HELPC = 8,
} charColors;

// taking code from Unix prog 2 for up and down arrow support
typedef struct MemoryNode // QNodes used for the memory List
{
	struct MemoryNode *next;
	struct MemoryNode *previous;
	char *data;
} qNode; 

typedef struct MemoryList // linked list of CharList, used for up and down arrowing
{
	qNode *head; // the first, this is what gets added to
	qNode *tail; // the end of the queue, will be freed if reached
	qNode *current;
	int length; // the length of the memory list, should never exceed MAXQUEUE
	int position; // represents where I am starting when I am itterating
} upList;

// methods
void startCurses();

void exitCurses(int status);

void refreshAll();

int printWin(int win, char *msg); // prints string to Win

int printWinf(int win, const char *format, ...); // prints to given window

void printfsm(int mode, const char *format, ...); // prints to stdin with newline containing mode start

void printsm(int mode, char *msg); // prints string to stdWindow

void printfs(const char *format, ...); // print to stdin window, for debug messeges

char *input(int *mode); // processes stdin

void moveDown(WINDOW *win);

void printMessage(int win, char *msg, int length); // print messege with hex for nonprintable characters

// update border methods
void printBorders();

void updateMode(int mode);

void updateLoop(int loopMode);

void updateRecord(int inRecordMode, int side, char *delimiter);

void scrollWindow(int win, int direction);

void updateMiddleDir(int outputr);

void updateConnections(char *message, int mode, int connections);

void updateInternalFilter(int position, int stripFilter);

void updateExternalFilter(int position, char **argv, int argc);

void updateLogFiles(int *logList);

void printHelp();

void printHelpConnect();

void printHelpListen();

void printHelpListenOption();

void printHelpProtocol();

void clearWindows();

void printMessageNoLine(int win, char *msg, int length);

void handleResize();

int isLittleEndian();
