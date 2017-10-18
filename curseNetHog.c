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
 * CSCI 364 
 * helper for nethog
 * handdles curses.h functionality
 */ 

// includes
#include "curseNetHog.h" // contains global variables that will be used in program

#define INPUTCHARCOUNT 10 // size of input message
#define DELAYTIME 0 // the amount of time I wait to process input
#define PIGSCALE 13.0 // the scailing height for the piggs, should be <43 smaller gerter terminal size
	// defaults to 16.0
#define MAXLENGTH 100 // maximum length the queue can hold

#define DELAYTIME 0 // the amount of time I wait to process input
/* global constants */

WINDOW *mainW; // the main window
WINDOW *w[5]; // list of windows
	/*
	 * 0 is the input window from standerd in
	 * 1 is data entering from the left
	 * 2 is data leaving to the right
	 * 3 is data entering from the right
	 * 4 is data leaving to the left
	 */ 
// arrays that will be used when printing information to the differen screens
int maxRows, maxCols; // the size of the window I have to work with
int vsplit, hsplit; // were I will be spliting the different pig windows
int nLines, nCols;

// current position for cursor for stdin
int inputCharCount; // number of characters that have been inputed 
int inputCharPosition; // were I currently am on the line in relation to inputCharCount
int inputXStart; // the starting position of the cursor in x position

upList queue;

#if LOGMODE
	FILE *logFile;
#endif

// methods

/*
 * method that adds char *line to the queue
 */ 
void addToQueue(char *line)
{
	int sLength = strlen(line);
	qNode *node = malloc(sizeof(qNode));
	memset((char *) node, 0, sizeof(qNode));
	char *buff = malloc(sizeof(char)*(sLength+1)); // the new home for the line
	strncpy(buff, line, sLength);
	buff[sLength-1] = '\0'; // add null character were the \n character is
	
	if(queue.head == NULL) // nothing in the queue
	{
		queue.head = node;
		queue.tail = queue.head;
	}
	else // adds it to the queue regurally
	{
		queue.head->previous = node; // linkes the nodes
		node->next = queue.head;
		queue.head = node; // moves it to start
	}
	queue.current = queue.head; // set current to the start
	node->data = buff; // adds the data
	queue.length++;
	if(queue.length >= MAXLENGTH) // have to remove the last
	{
		qNode *deadNode = queue.tail;
		queue.tail = queue.tail->previous;
		queue.tail->next = NULL; // get rid of it
		free(deadNode->data);
		free(deadNode);
		queue.length--; // decrements the length
	}
}

/*
 * returns 1 if littleEndian
 * else will return 0
 * need compatability to do this
 */ 
int isLittleEndian()
{
	int test = 1;
	char *temp = (char *) &test;
	return *temp==1; // it is little endian if that is tha case
}

void printBorders()
{
	int i, j;
	for(i=1; i<maxCols-1; i++) // dawing horizontal lines
	{
		if(i==vsplit || i==vsplit+1) // will handle edge cases seperately
		{
			mvwaddch(mainW, maxRows-1, i, ACS_HLINE);
			continue;
		}
		mvwaddch(mainW, 0, i, ACS_HLINE);
		mvwaddch(mainW, hsplit, i, ACS_HLINE);
		mvwaddch(mainW, hsplit+nLines+1, i, ACS_HLINE);
		mvwaddch(mainW, maxRows-1, i, ACS_HLINE);
	}

	// draws virtical bars
	for(j=1; j<maxRows-1; j++)
	{
		int print = ACS_VLINE;
		if(j==hsplit || j==hsplit+nLines+1)
			continue; // cannot print a line there since '+' is already there

		mvwaddch(mainW, j, 0, print);
		mvwaddch(mainW, j, maxCols-1, print);
		if(j <hsplit+nLines+1) // so I do not print in the middle of the screen
		{
			mvwaddch(mainW, j, vsplit, print);
			mvwaddch(mainW, j, vsplit+1, print);
		}
	}

	// drawing edge cases
	mvwaddch(mainW, 0, 0, ACS_ULCORNER);
	mvwaddch(mainW, 0, maxCols-1, ACS_URCORNER);
	mvwaddch(mainW, maxRows-1, 0, ACS_LLCORNER);
	mvwaddch(mainW, maxRows-1, maxCols-1, ACS_LRCORNER);

	mvwaddch(mainW, hsplit, 0, ACS_LTEE);
	mvwaddch(mainW, hsplit+nLines+1, 0, ACS_LTEE);
	mvwaddch(mainW, hsplit, maxCols-1, ACS_RTEE);
	mvwaddch(mainW, hsplit+nLines+1, maxCols-1, ACS_RTEE);

	for(i=0; i<=1; i++)
	{
		mvwaddch(mainW, 0, vsplit+i, ACS_TTEE);
		mvwaddch(mainW, hsplit, vsplit+i, ACS_PLUS);
		mvwaddch(mainW, hsplit+nLines+1, vsplit+i, ACS_BTEE);
	}

	updateMode(1); // set in command mode to start

	wrefresh(mainW);
}

/*
 * will move the cursor back to the std in screen were it was originally
 * will generally be called after the update methods
 */ 
void moveCurse()
{
	int curY, curX;
	getyx(w[0], curY, curX);
	wmove(w[0], curY, curX);
	wrefresh(w[0]);
}

/*
 * will update whate mode I currently am in on the border
 */ 
void updateMode(int mode)
{
	const char *insertMode = " INSERT ";
	const char *commandMode = " COMMAND ";
	int i;
	int maxSize = 10; // maximum size aloud 
	
	for(i=0; i<maxSize; i++) // clears previous entry
		mvwaddch(mainW, maxRows-1, 2+i, ACS_HLINE);

	if(!mode) // going into input mode
		mvwaddstr(mainW, maxRows-1, 2, insertMode);
	else // in command mode
		mvwaddstr(mainW, maxRows-1, 2, commandMode);
	
	wrefresh(mainW);
	moveCurse(); // set cursor back
}

/*
 * displays when I am in record mode
 * inRecord mode is a yes to print if 1, 0 to not print
 * side is eather INLEFT OR INRIGHT to show wich side of the screen I am printing
 * will print the delimeter as well
 */ 
void updateRecord(int inRecordMode, int side, char *delimiter)
{
	int i=0;
	int yScale = hsplit+nLines+1;
	int xScale = 10;
	int increment = 1;
	if(side == INRIGHT) // change the scale factors
	{
		yScale = 0;
		xScale = maxCols-10;
		increment = -1;
	}
	// will fill in all that is not of the horizontal line
	while(mvwinch(mainW, yScale, xScale+i)!=ACS_HLINE)
	{
		waddch(mainW, ACS_HLINE);
		i+=increment;
	}
	if(inRecordMode) // guess I'm going to have to write this one out
	{
		wattron(mainW, COLOR_PAIR(LOGC));
		wattron(mainW, A_STANDOUT);
		size_t delimiterLength = strlen(delimiter);
		size_t len = delimiterLength*2+10;
		char *hexBuffer = alloca(sizeof(char)*len);
		memset(hexBuffer, 0, len);
		char *ted = hexBuffer; // its your favorit itterator ted again
		if(side == INLEFT) // will first print record mode
		{
			sprintf(hexBuffer, " RecordMode ");
			ted = &ted[12];
		}
		sprintf(ted, "0x");
		ted = &ted[2];

		//int little = isLittleEndian();
		// going to print the hex values into the buffer
		for(i=0; i<delimiterLength; i++)
		{
			sprintf(ted, "%X", delimiter[i] & 0xff); // so I only get the lower bound
			ted = &ted[strlen(ted)];
		}
		sprintf(ted, " ");
		if(side == INRIGHT)
		{
			sprintf(ted, " RecordMode ");
			mvwprintw(mainW, 0, maxCols-10-strlen(hexBuffer), "%s", hexBuffer);
		} else
		{
			mvwprintw(mainW, hsplit+nLines+1, 10, "%s", hexBuffer);
		}
		wattroff(mainW, COLOR_PAIR(LOGC));
		wattroff(mainW, A_STANDOUT);
	}
	wrefresh(mainW);
	moveCurse(); // set cursor back
}


/*
 * will update the loop message 
 * mode is just like ISLOOP enume
 * 0 noLoop, 1 right, 2 left, 3 both
 */ 
void updateLoop(int loopMode)
{
	int i;
	// first clear the original positions
	for(i=0; i<5; i++)
	{
		mvwaddch(mainW, hsplit+nLines+1, 2+i, ACS_HLINE);
		mvwaddch(mainW, 0, maxCols-7+i, ACS_HLINE);
	}

	// now prints the loop message if they exist
	wattron(mainW, COLOR_PAIR(LOOPC)); // sets to given color
	wattron(mainW, A_STANDOUT);

	if(loopMode ==1 || loopMode == 3) // it is looping right
		mvwaddstr(mainW, 0, maxCols-7, "loopr");
	if(loopMode == 2 || loopMode == 3) // it is looping left
		mvwaddstr(mainW, hsplit+nLines+1, 2, "loopl");

	wattroff(mainW, COLOR_PAIR(LOOPC));
	wattroff(mainW, A_STANDOUT);
	
	wrefresh(mainW); // updates screen
	moveCurse();
}

/*
 * updates the direction that middle (stdin) is pointing to
 *
 * if outputr is positive will diplay goint to the right
 * if outpurr is negative will display going to the left
 * if outputr is 0 then will not be displayed at all
 */ 
void updateMiddleDir(int output)
{
	int i;
	int maxSize = 8;
	int divide = (vsplit/2)-10;
	// first step is to get rid of the old ouput directions
	for(i=0; i<maxSize; i++) // fills in the border again
	{
		mvwaddch(mainW, maxRows-1, vsplit-maxSize-divide+i, ACS_HLINE);
		mvwaddch(mainW, maxRows-1, vsplit+divide+i-3, ACS_HLINE);
	}

	wattron(mainW, COLOR_PAIR(LOOPC)); // change color and effect
	wattron(mainW, A_STANDOUT);

	if(output>0) // will print outputr
		mvwaddstr(mainW, maxRows-1, vsplit+divide-3, "outputR");
	if(output<0) // will print outputl
		mvwaddstr(mainW, maxRows-1, vsplit-maxSize-divide, "outputL");

	wattroff(mainW, COLOR_PAIR(LOOPC));
	wattroff(mainW, A_STANDOUT);

	wrefresh(mainW); // moves the cursor back and updates screen
	moveCurse();
}

/*
 * will print to the connection screen the message in three different ways
 * if mode == INLEFT the it will update the left connection
 * if mode == INRIGHT then it will update the right connection
 * message must be null terminated
 *
 * if messege is null there is no connection
 * will do its best to put the message in the middle section of the window
 *
 * update listen should always be called before update connection
 * this is because I know where to start my connection list based of of when listen ends
 */ 
void updateConnections(char *message, int mode, int connections)
{	
	int scale = 1; // used to determin writing left or right
	if(mode==INRIGHT) 
	{
		scale += vsplit+1; // transitioning to were I am printing
	}
	int i;
	for(i=1; i<vsplit-1; i++) // redraws border
		mvwaddch(mainW, hsplit, i+scale, ACS_HLINE);
	
	int movedAmount = 0; // the amount I have writen for the listening
	if(message==NULL) // not listening
	{
		wattron(mainW, A_STANDOUT);
		wattron(mainW, COLOR_PAIR(NOCONNECTC));
		mvwprintw(mainW, hsplit, 1+scale, "%s", "not listening");
		movedAmount = 14;
		wattroff(mainW, COLOR_PAIR(NOCONNECTC));
		wattroff(mainW, A_STANDOUT);
	} else // listening to somthing 
	{
		wattron(mainW, A_STANDOUT);
		wattron(mainW, COLOR_PAIR(LISTENC));
		mvwprintw(mainW,hsplit, 1+scale, "listening on: %s", message);
		movedAmount = 15+strlen(message);
		wattroff(mainW, COLOR_PAIR(LISTENC));
		wattroff(mainW, A_STANDOUT);
	}
	// now time for the connectionCount1
	wattron(mainW, A_STANDOUT);
	wattron(mainW, COLOR_PAIR(CONNECTC));
	mvwprintw(mainW, hsplit, 1+scale+movedAmount+3, "connections: %d ", connections);
	wattroff(mainW, COLOR_PAIR(CONNECTC));
	wattroff(mainW, A_STANDOUT);
	wrefresh(mainW);
	moveCurse();
	
	/*
	if(mode==WSTDIN)
	{
		printfs("%s\n", message);
	}
	else // printing to the borders
	{
		int i;
		int scale = 0;
		if(mode==INRIGHT) scale = vsplit+1; // transitioning were I am printing
		for(i=1; i<vsplit-1; i++) // overwrites what was previously there
			mvwaddch(mainW, hsplit, i+scale, ACS_HLINE);

		int length = strlen(message); // length of string
		wattron(mainW, A_STANDOUT); // setting up colors
		wattron(mainW, COLOR_PAIR(type));
		mvwprintw(mainW, hsplit, (vsplit/2)-(length/2)+scale, "%s", message); // prints it
		wattroff(mainW, A_STANDOUT);
		wattroff(mainW, COLOR_PAIR(type));
		wrefresh(mainW); // moves the cursor back and updates screen
		moveCurse();
	}
	*/
}

/*
 * method that gets called by piggy4.c to update internal filtering 
 * this gets displayed in the top left for lr and bottom right for rl
 * display in the color of blue
 *
 * position either being INLEFT for lr or INRIGHT for rl
 */ 
void updateInternalFilter(int position, int stripFilter)
{
	int i;
	int yScale = 0, xScale = 2; // what I will be scaling by to determin positioning
	int increment = 1; // go left or go right
	if(position == INRIGHT) // bottom right goes right to left different set up
	{
		yScale = hsplit+nLines+1;
		xScale = maxCols-3;
		increment = -1;
	}
	for(i=0; i<10; i++) // clearing what was already there
		mvwaddch(mainW, yScale, xScale+(i*increment), ACS_HLINE);
	wattron(mainW, A_STANDOUT); // setting up colors
	wattron(mainW, COLOR_PAIR(FILTERC));
	
	char message[30] = "stno\0"; // what I will be printing
	if(stripFilter == 1) memcpy(message, "stnpxeol\0", 9); // STRIPSOME
	if(stripFilter == 2) memcpy(message, "stnp\0", 5); // STRIPALL
	
	int length = strlen(message);
	int printP = position==INLEFT ? 0 : length-1; // seting up inital value for printing going backward or forwards 
		//also first time using one of these statments
	for(i=0; i<length; printP+=increment, i++) // prints the message
		mvwaddch(mainW, yScale, xScale+(i*increment), message[printP]);

	wattroff(mainW, A_STANDOUT);  // resets colors and updates window
	wattroff(mainW, COLOR_PAIR(FILTERC));
	wrefresh(mainW);
	moveCurse();
}

/*
 * updates the external filtering information given by the argv and argc
 * at the given position of eaither INLEFT or INRIGHT to change were it gets displayed
 * using the same ideas as updateInternalFilter of printing everything backwards for INRIGHT
 */ 
void updateExternalFilter(int position, char **argv, int argc)
{
	int i;
	int yScale = 0, xScale = 13;
	int increment = 1;
	int maxSize = maxCols-20; // arbitrary maximum size for the string so it can fit
	char line[maxSize+2]; // +2 becuase of space shenanagans when coppying
	int length = 0; // the current length of my line
	int colorType = 0;
	int printCharacter = ACS_TTEE; // what I will be printing
	memset(line, 0, maxSize+2); // clears memory
	if(position == INRIGHT)
	{
		yScale = hsplit+nLines+1;
		xScale = maxCols-14; // arbitrary starting position
		increment = -1;
		printCharacter = ACS_BTEE;
	}
	for(i=0; i<maxSize; i++)
		mvwaddch(mainW, yScale, xScale+(i*increment), ACS_HLINE); // clear wat was already there before
	for(i=0; i<=1; i++) // prints the t characters, can be overriden but makes it look good
		mvwaddch(mainW, yScale, vsplit+i, printCharacter); 
	if(argv == NULL)
	{
		colorType = NOCONNECTC; // red
		memcpy(line, "no external filter\0", 19); // they gave me nothing so I print that information
		length = strlen(line); 
	}
	else // start creating the string
	{
		for(i=0; i<argc && length < maxSize; i++) // itterates through argv adding the strings to the line
		{
			if(strlen(argv[i])+length < maxSize) // I can put it into the string
			{
				memcpy(line+length, argv[i], strlen(argv[i]));
				length+=strlen(argv[i]);
				line[length] = ' ';
				length++;
				line[length] = '\0';
			}
			else
				break; // no use to keep looping
		}
		colorType = CONNECTC; // green
	}
	wattron(mainW, COLOR_PAIR(colorType));
	wattron(mainW, A_STANDOUT);
	int printP = position==INLEFT ? 0 : length-1; // seting up inital value for printing so I go backward or forwards
	for(i=0; i<length; printP+=increment, i++) // prints the message
		mvwaddch(mainW, yScale, xScale+(i*increment), line[printP]);
	wattroff(mainW, COLOR_PAIR(colorType));
	wattroff(mainW, A_STANDOUT);
	wrefresh(mainW);
	moveCurse();
}

/*
 * used to update if I am loging or not
 * assumes logList is an array of ints of size 4
 * if an element is -1 I am not logging, else I am logging to the file
 * this message will be printed to the bottom right corner of the screen
 */ 
void updateLogFiles(int *logList)
{
	int i;
	int yScale = maxRows -1, xScale = maxCols-32;
	for(i=0; i<31; i++) // clears what was initally there
		mvwaddch(mainW, yScale, xScale+i, ACS_HLINE);
	
	wattron(mainW, COLOR_PAIR(LOGC));
	wattron(mainW, A_STANDOUT);
	mvwprintw(mainW, yScale, xScale, "%s", "Log:");
	// goes through and toggles them on if I am writing to them
	if(logList[0] > 0) 
		mvwprintw(mainW, yScale, xScale+5, "%s", "lrpre");
	if(logList[1] > 0)
		mvwprintw(mainW, yScale, xScale+11, "%s", "lrpost");
	if(logList[2] > 0)
		mvwprintw(mainW, yScale, xScale+18, "%s", "rlpre");
	if(logList[3] > 0)
		mvwprintw(mainW, yScale, xScale+24, "%s", "rlpost");

	wattroff(mainW, COLOR_PAIR(LOGC));
	wattroff(mainW, A_STANDOUT);
	wrefresh(mainW);
	moveCurse();
}

/*
 * function that gets called when I need to resize the screen
 * will redraw the border info and such
 * gets called by the signal andle function in piggy.c
 * the idea is after calling this function it will pass the border information for the border to be updated
 */ 
void handleResize()
{
	// TODO add implemntation to grab what was in the window so I can display it in the next session of curses

	endwin(); // restarts curses and all my global variables
	maxRows = 0;
	maxCols =0;
	vsplit = 0;
	hsplit = 0;
	nLines = 0;
	nCols = 0;
	inputCharCount = 0;
	inputCharPosition = 0;
	inputXStart=0;

	startCurses(); // hopfully will reset everything
}

/*
 * prints help information to the windows
 * set up so that it will overflow information between the different windows
 */
void printHelpHelper(int length, char *message[])
{
	clearWindows(); // clears the windows
	int i, j;
	i=0;
	wclear(w[INLEFT]); // clearing window
	wclear(w[OUTRIGHT]);
	
	for(i=1; i<5; i++)
	{
		wattron(w[i], COLOR_PAIR(HELPC)); // set up color
		wattron(w[i], A_STANDOUT);
	}
	i=0;
	for(j=0; j<hsplit-1 && i<length; j++, i+=2) // prints the formated message
	{
		mvwprintw(w[INLEFT], j, 0, "(%d) %s", (i/2)+1, message[i]);
		mvwprintw(w[OUTRIGHT], j, 0, "(%d) %s", (i/2)+1, message[i+1]);
	}
	if(i<length) // still have more to print so I have to clear bottom windows to
	{
		wclear(w[OUTLEFT]);
		wclear(w[INRIGHT]);
	}
	for(j=0; j<hsplit-1 && i<length; j++, i+=2) // printing to the lower two windows
	{
		mvwprintw(w[OUTLEFT], j, 0, "(%d) %s", (i/2)+1, message[i]);
		mvwprintw(w[INRIGHT], j, 0, "(%d) %s", (i/2)+1, message[i+1]);
	}
	for(i=1; i<5; i++) // closing colors
	{
		wattroff(w[i], COLOR_PAIR(HELPC));
		wattroff(w[i], A_STANDOUT);
		wrefresh(w[i]);
	}
	moveCurse();
}

// prints the :help command
void printHelp()
{
	int length = 50; // what I go to
	// even statment, odd desciption
	char *message[60] = { // table of all the mesages in pair os statment and description
		":connect(l/r) [connect args]\0", "Connect to the address at the port\0",
		":listen(l/r) [listen args]\0", "Set up passive connection on left or right\0",
		":listenop(l/r) [listen option args]\0", "Defines what can connect to side\0",
		":dropconnect(l/r) [number]\0", "Drops connected socket, if no number drops all\0",
		":droplisten(l/r)\0", "Stopes listening in that direction\0",
		":dropaccept(l/r) [number]", "Will no longer user that listen option\0",
		":loop(l/r)\0", "Toggles looping in given direction\0",
		":output(l/r)\0", "Changes stdin output direction\0",
		":(left / right)", "Prints detailed information about the left and right connections",
		":read [file]\0", "Sends file in ouput direction\0",
		":source [file]\0", "Executes script like command line\0",
		":external(l/r) [commands]\0", "Toggles external filtering\0",
		":external(l/r)off\0", "Turns off external filtering\0",
		":log(lr/rl)(pre/post) [file]\0", "Toggles loging windows to file\0",
		":log(lr/rl)(pre/post)off\0", "Stops loging to file\0",
		":st(lr/rl)(np/npxeol)\0", "Toggles internal filtering\0",
		":record(lr/rl) [on / off]\0", "Toggles record mode\0",
		":delimiter(lr/rl) 0x[delimiter]\0", "Give Hex values to represent delimiter\0",
		":spawnlocalpig [piggy args]\0", "creates another piggy on the same machine\0",
		":spawnremotepig [user@hostname] [connection args] [piggy args]\0", "spawns a piggy on a remote machine through ssh\0",
		":clear\0", "Clears all of the message windows\0",
		":help [connect / listen / listenop / protocol]\0", "shows different help commands\0",
		":q\0", "Quits the current piggy\0",
		//":q!\0", "Shuts down all piggys\0",
		":d\0", "detaches session from current machine\0",
		":d!\0", "makes piggy run in the background\0"
	};
	printHelpHelper(length, message);
}

void printHelpConnect()
{
	int length = 10;
	char *message[11] = { // table desplaying the arguments for a connection
		"--addr [arg] or -a [arg]\0", "Address or path of the connection required\0",
		"--rp [port] or -r [port]\0", "remote port of the connection\0",
		"--lp [port] or -l [port]\0", "local port that will be bound to\0",
		"--protocol [string] or -p [string]\0", "unix or tcp protocol only\0",
		"--iptype [string] or -i [string]\0", "uses ip4 or ip6\0"
	};
	printHelpHelper(length, message);
}

void printHelpListen()
{
	int length = 8;
	char *message[9] = {
		"--lp [port] or -l [port]\0", "Defines the port to listen to, not used for UNIX DOMIN SOCKET\0",
		"--addr [path] or -a [path]\0", "The path to the Unix Domain Socket Connection\0",
		"--iptype [string] or -i [string]\0", "ip4 or ip6 connection\0",
		"--protocol [string] or -p [string]\0", "defines the protocol to use see :help protocol\0"
	};
	printHelpHelper(length, message);
}

void printHelpListenOption()
{
	int length = 8;
	char *message[9] = {
		"--rp [port] or -r [port]\0", "only accept connections with remote port of this number\0",
		"--addr [address] or -a [address]\0", "only aceept connection from that address\0",
		"If you do not define port, will accept from any port\0", 
		"If you do not define address will accept from any address\0",
		"If you do not use listenop will accept from anyone\0",
		"Unix domain sockets will accept from anyone\0",
	};
	printHelpHelper(length, message);
}

void printHelpProtocol()
{
	int length = 14;
	char *message[15] = {
		"use these strings to define the different types of connection you want\0", "\0",
		"tcp\0", "reliable connection stream\0",
		"udp\0", "connectionless unreliable messages datagrams\0",
		"unix\0", "unix domain socket on local machine acts like named fifo\0",
		"icmp\0", "send icmp layer messeges\0",
		"ip\0", "sends ip layer messeges\0",
		"raw\0", "sends raw messeges across ethernet\0"
	};
	printHelpHelper(length, message);
}



/*
 * helper method to clear all the windows and set there cursor positions
 * this does not clear the stdin window
 */ 
void clearWindows()
{
	int i;
	for(i=1; i<5; i++)
	{
		wclear(w[i]);
		wmove(w[i], 0, 0);
		wrefresh(w[i]);
	}
	moveCurse();
}

// should be called at the very start of the program to set up windows
void startCurses()
{
	int c, d, e; // constant used when splitting up the windows
	int i;
	setlocale(LC_ALL, ""); // character set to use

	initscr(); // initialize the library

	cbreak(); // obtain characters in raw mode

	noecho(); // character typed do not show up on screen

	//nonl(); // do not translate newline character to a carraige return linefeed

	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE); 
	clear();
	
	/* XXX currently ignoring this check, can get as small as they want
	if(LINES< MINROWS || COLS < MINCOLS) // the window is not set up to the appropreate size
	{
		// displaying error message and exiting
		move(0, 0);
		addstr("Piggy 3 requires a screen size greater than or equal to 132 columns and 43 rows");
		move(1, 0);
		addstr("Set screen size greater than 132 by 43 and try again");
		move(2, 0);
		addstr("Press any key to terminate program");
		refresh();
		getch(); // pause to see message
		endwin();
		exit(EXIT_FAILURE);
	}
	*/

	// windows are good time to create the grid
	maxRows = LINES; maxCols = COLS; 
	if(maxCols%2!=0)
		maxCols--; // has to be even number to be split evenly
	
	// calculates window scailed posisions
	nLines = (int) ((PIGSCALE/MINROWS)*maxRows); // horizontal split
	nCols = (maxCols/2)-2;
	c = 0; d = 0; 
	e = maxRows-(nLines*2)-4; // the height for windowList[0] stdin

	hsplit = nLines+1;
	vsplit = nCols+1;
	
	// create and position  main window
	mainW = newwin(0, 0, 0, 0);
	touchwin(mainW);
	wmove(mainW, 0, 0);
	wrefresh(mainW);
	keypad(mainW, TRUE);
	
	// creates sub windows taking into account window borders
	w[0] = subwin(mainW, e, maxCols-2, 2*nLines+3, 1); // stdin window
	w[1] = subwin(mainW, nLines, nCols, c+1, d+1);
	w[2] = subwin(mainW, nLines, nCols, c+1, nCols+3);
	w[3] = subwin(mainW, nLines, nCols, nLines+2, nCols+3);
	w[4] = subwin(mainW, nLines, nCols, nLines+2, c+1);
	
	// sets up starting scrolling and key translations
	for(i=0; i<5; i++)
	{
		scrollok(w[i], TRUE);
		intrflush(w[i], FALSE);
		keypad(w[i], TRUE); // turning keypad translation on
		//nodelay(w[i], TRUE);
	}

	//halfdelay(DELAYTIME); // set up input delay
	
	if(getenv("ESCDELAY") == NULL) // reduce delay for the escape key
		ESCDELAY = 25;
	
	refreshAll(); // updates display and draws all the boarders
	
	// initalize global constants to be used
	inputCharCount = 0;
	inputCharPosition = 0;
	inputXStart = 0;
	
	if(!can_change_color()) // I can use colors
	{
		start_color();
		init_pair(LOOPC, COLOR_MAGENTA, COLOR_BLACK); // loop color
		init_pair(LISTENC, COLOR_YELLOW, COLOR_BLACK);
		init_pair(NOCONNECTC, COLOR_RED, COLOR_BLACK);
		init_pair(CONNECTC, COLOR_GREEN, COLOR_BLACK);
		init_pair(FILTERC, COLOR_YELLOW, COLOR_BLACK);
		init_pair(LOGC, COLOR_CYAN, COLOR_BLACK);
		init_pair(HELPC, COLOR_BLUE, COLOR_WHITE);
	}
	if(queue.length == 0) // a queue does not exist yet so making a new one
	{
		queue.length = 0;
		queue.head = NULL;
		queue.tail = NULL;
		queue.current = NULL;
		queue.position = -1;
	} // else everything is being resized

	updateInternalFilter(INLEFT, 0); // set up filtering display
	updateInternalFilter(INRIGHT, 0); 
	updateExternalFilter(INLEFT, NULL, 0);
	updateExternalFilter(INRIGHT, NULL, 0);

	#if LOGMODE
		logFile = fopen(LOGNAME, "w");
	#endif
}

/*
 * exits the program with the given status
 * used to not mess up the terminal
 */ 
void exitCurses(int status)
{
	endwin();
	exit(status); // good by program
}

/*
 * function that will move the curser down seting it at the x position
 * if moving the curser down by one would cause scrolling, this function
 * will scroll the window and then moves the curser position
 */ 
void moveDown(WINDOW *win)
{
	int curX, curY, maxX, maxY;

	getyx(win, curY, curX);
	getmaxyx(win, maxY, maxX);
	curX++; maxX--; // keep the compiler happy

	if(curY == maxY-1) // scroll the window 
	{
		scroll(win);
		wmove(win, curY, 0);
	}
	else // moves down by one
		wmove(win, curY+1, 0);
	moveCurse();
	wrefresh(win);
}

/*
 * prints the given messege to a window
 */ 
int printWin(int win, char *msg)
{
	return printWinf(win, "%s", msg);
}

/*
 * prints the message to the given window
 *
 * not recemended for use when printing to the stdin window
 *
 * refreshes window after print
 */ 
int printWinf(int win, const char *format, ...)
{
	va_list arg;
	int done;
	
	va_start(arg, format);
	done = vwprintw(w[win], format, arg);
	va_end(arg);
	
	moveCurse();
	wrefresh(w[win]);
	return done;
}

/*
 * helper method that will print to the stdWin terminal
 * string can contain newLine or not
 * mode refres to command mode or insert mode
 * 0 refers to being in insert mode, postive means command mode
 * assums that the user is not working on anything right now 
 *
 * if format is null will just print insert starter for next line
 */ 
void printfsm(int mode, const char *format, ...)
{
	if(format!=NULL)
	{
		va_list arg;
		va_start(arg, format);
		vwprintw(w[0], format, arg);
		va_end(arg);
	}
	// if I am in insert mode, I need to print insert
	/*if(!mode)
	{
		wprintw(w[0], "--INSERT--");
		inputXStart = INPUTCHARCOUNT;
	} */ // depreicated code
	wrefresh(w[0]);
}

// just prints the string to stdin
// XXX depreicated and should not be used
void printsm(int mode, char *msg)
{
	printfsm(mode, "%s", msg);
}

/*
 * prints messege to stdin window
 * used mainly for debug statments
 *
 * add functinality to print to terminal and not distroy what I am currently writing
 */ 
void printfs(const char *format, ...)
{
	va_list arg; 
	va_start(arg, format);
	#if ERRORMODE
		fprintf(stderr, format, arg); // so I can see all my error messages before my code breaks
	#endif
	vwprintw(w[0], format, arg); // default mode
	#if LOGMODE
		fprintf(logFile, format, arg); // prints everything to a log file
	#endif
	va_end(arg);
	wrefresh(w[0]);
}

/*
 * prints the given messege to the given screen
 * will format the output such that it will print all non, printable characters
 * with there hex values
 * will move the cursor down after printing
 *
 * win is the window I am printing to
 * msg is the msg I am sending
 * length is the length of the string I will be printing
 */ 
void printMessage(int win, char *msg, int length)
{
	int i;
	int curY, curX;
	getyx(w[win], curY, curX);
	curY--; // not used
	if(curX != 0) // if I am not on a new line, will scroll down
		moveDown(w[win]);

	for(i=0; i<length; i++)
	{
		if(msg[i]>=32 && msg[i]<=126) // printable character
		{
			waddch(w[win], msg[i]);
		}
		else // non printable character
		{
			if(msg[i] <= 0xE)
				wprintw(w[win], "0x0%X", msg[i]); // in hex with 0 infront for two characters
			else
				wprintw(w[win], "0x%X", msg[i]);
		}
	}
	wrefresh(w[win]);
	moveCurse();
	//moveDown(w[win]); // moving down occurs at the start of the method
}

/*
 * just like printMessage except wil not create a new line for scrolling, starts were the cursor is
 */ 
void printMessageNoLine(int win, char *msg, int length)
{
	int i;
	/*
	int curY, curX;
	getyx(w[win], curY, curX);
	curY--; // not used
	*/
	for(i=0; i<length; i++)
	{
		if(msg[i]>=32 && msg[i]<=126) // printable character
		{
			waddch(w[win], msg[i]);
		}
		else // non printable character
		{
			if(msg[i] <= 0xE)
				wprintw(w[win], "0x0%X", msg[i]); // in hex with 0 infront for two characters
			else
				wprintw(w[win], "0x%X", msg[i]);
		}
	}
	wrefresh(w[win]);
	moveCurse();
}

/*
 * will process a character at a time the given input
 * will not print nonprintible strings for input
 * can only process once line of input
 * this means that I cannot type more than a single line of the terminal screen
 *
 * returns null on sucesfull process of character
 * returns pointer to a malloced string when newline has been processed
 * up to calling function to free that string when they are done
 *
 * if input mode changes returns null and changes mode integer to proper setting
 *
 * when returning the string appends the newline character \n
 *
 * mode, 0 for command mode, 1 for insert mode
 */ 
char *input(int *mode)
{
	int yMax, xMax;
	int curX, curY;
	//int sLength;
	char *buff = NULL; 

	getmaxyx(w[0], yMax, xMax); // obtaining maximum values
	yMax--;
	getyx(w[0], curY, curX); // current yx values
	wmove(w[0], curY, curX); // moves cursor to position so it shows up on screen

	int character = wgetch(w[0]);
	
	if(character == 'i' && *mode == 1 && inputCharCount == 0) // switching to input mode
	{
		*mode = 0; 
		/*wprintw(w[0], "--INSERT--");
		inputXStart = INPUTCHARCOUNT; */
		
		updateMode(0);
		
		wrefresh(w[0]);
		return NULL; 
	}

	if(character>=32 && character<=126) // printable character
	{
		if(inputCharCount < xMax-1-inputXStart) // there is room to put a character down
		{
			winsch(w[0], character);
			wmove(w[0], curY, curX+1);
			inputCharCount++;
			inputCharPosition++;
			wrefresh(w[0]);
			// will return null on success
		}
		// else there was no room returning null
		return NULL;
	}
	// else this is some non-printable character that needs to be processed
	
	switch(character)
	{
		case 27: // esc key
			if(*mode == 0) // insert mode
			{
				*mode = 1;
				updateMode(1); // switches to command mode
			}
			wmove(w[0], curY, 0); // moves the cursor back to the start of the line
			wclrtoeol(w[0]); // clears the entire line
			// clear the line

			inputXStart = 0; // reset line variables
			inputCharCount = 0;
			inputCharPosition = 0;
			wrefresh(w[0]);
			break;

		case '\n': // new line character, will grab the line and return it
			buff = malloc(inputCharCount+2); // +2 for '\n' character and '\0' character
			memset(buff, 0, inputCharCount+2); // blast that memory
			
			wmove(w[0], curY, inputXStart); // moves the cursor to obtain the string
			winnstr(w[0], buff, inputCharCount);
			
			buff[inputCharCount] = '\n'; // create newline and null termination
			buff[inputCharCount+1] = '\0';
			
			moveDown(w[0]); // sets up for the next input or print statment
			inputCharCount = 0;
			inputCharPosition = 0;
			inputXStart = 0;
			// if I am in input mode, when I get printed to next I will then display input
			wrefresh(w[0]);
			addToQueue(buff); // now adds to queue
			queue.position = -1; // setting the queue position back to the begining

			return buff; // returns the string, up to calling function to free it

		case KEY_RIGHT: // move cursor to the right
			if(inputCharPosition != inputCharCount) // I am not all the way to the right
			{
				wmove(w[0], curY, curX+1);
				inputCharPosition++;
			}
			break;
		
		case KEY_LEFT: // move cursor to the left
			if(inputCharPosition != 0) // I am not at the far left
			{
				wmove(w[0], curY, curX-1);
				inputCharPosition--;
			}
			break;

		case KEY_BACKSPACE: // delete character
			if(inputCharPosition != inputXStart) // not at the far left
			{
				mvwdelch(w[0], curY, curX-1); // moves and deletes that character
				inputCharCount--;
				inputCharPosition--;
			}
			break;

		case KEY_UP: // XXX  needs work
			if(queue.current != NULL)
			{
				wmove(w[0], curY, 0);
				wclrtoeol(w[0]);
				wprintw(w[0], "%s", queue.current->data);
				inputCharCount = strlen(queue.current->data);
				inputCharPosition = inputCharCount;
				if(queue.position+1 < queue.length)
					queue.position++;
				if(queue.current->next != NULL)
					queue.current = queue.current->next;
			}
			break;

		case KEY_DOWN: // move then print
			if(queue.current != NULL && queue.position != -1)
			{
				wmove(w[0], curY, 0);
				wclrtoeol(w[0]);
				inputCharCount = 0;
				inputCharPosition = 0;
				queue.position--;
				if(queue.current->previous != NULL)
				{
					queue.current = queue.current->previous;
					wprintw(w[0], "%s", queue.current->data);
					inputCharCount = strlen(queue.current->data);
					inputCharPosition = inputCharCount;
				}
			}
			break;

		default:
			break; // do nothing, don't want to print it
	}
	/*#if DEBUG
		wprintw(w[3], "possition %d\n", queue.position);
		wrefresh(w[3]);
	#endif */
	wrefresh(w[0]); // refreshes no matter what happens
	
	return NULL; // did commands and did not have to return a string
}

/*
 * will refresh all of the screens
 * prints the border
 * and move cursor position to the input screen
 *
 */
void refreshAll()
{
	printBorders(); // print the borders and refreshes mainWin

	int i;
	for(i=0; i<5; i++)
		wrefresh(w[i]);

	int inputCurrY, inputCurrX; // moves the cursor to the position
	getyx(w[0], inputCurrY, inputCurrX);
	wmove(w[0], inputCurrY, inputCurrX);
}

void scrollWindow(int win, int direction)
{
	wscrl(w[win], direction);
	wrefresh(w[win]);
}


