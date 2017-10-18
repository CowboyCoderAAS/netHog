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
 * 11/11/15
 * CSCI 367
 *
 * when your file get greater than 1000 lines, you start to need a header file
 */ 

#include <ctype.h>
#include <alloca.h> // like malloc but on the stack
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <getopt.h>
#include <sys/un.h>
#include <stdio_ext.h>
#include <sys/socket.h>
#include <grp.h>
#include <linux/limits.h>
#include <sys/stat.h>

#define SLEEPTEST 0
#define STARTSIZE 4 // starting size for the list of sockets
#define SSHPORT "922" // default port for ssh connections

/* 
 * enum to define the different loop options avalible for the pig
 * specifically if rightLoop is defined, that means that any output going through the right
 * will be also be sent the the left
 * this means that even though a piggy does not have a rightPig connected to it
 * it will still loop to the left because the output would have gone in that direction
 * this means I can have the end pig loop back and re-transmit the message
 */ 
enum ISLOOP
{
	NOLOOP = 0, // no looping
	RIGHTLOOP, // any output that goes through tht right will be looped into the left
	LEFTLOOP,
} loop;

enum STRIP
{
	STRIPNO = 0,
	STRIPSOME, // strips all non printible characters except newline and charage return
	STRIPALL, // strips all non printable characters
} stripFilter;

/*
 * defines my personal constants for protocal
 * will be used to get the domain, type and real protocal
 */ 
enum PROTOCOL
{
	O_IP = 0,
	O_ICMP,
	O_RAW,
	O_TCP,
	O_UDP,
	O_UNIX,
	O_IGMP,
} socketProtocol;

enum IP_TYPE
{
	T_IP4 = 0,
	T_IP6,
} socketIpType;

/*
 * wether or not I will be receving information from a user or not
 * will use but ands and ors to determin what to do, if 3 then I do both
 */
enum CONNECTION_TYPE
{
	C_RECEIVE = 1,
	C_SEND = 2, 
} socketConnectionType;

/*
 * This is the basic circularBuffer that I created in piggy4
 * just moved it to its own struct and such
 * has a special overflow feature that it will always be able to write
 * the basic idea is I will always be requesting size of of my read of BUFFSIZE
 * so I know I can only read if i have that amount in the buffer, thus the overflow feature
 * this is to garentee traversal of packets from unix domain sockets so that they stay together
 */ 
typedef struct circularBuffer
{
	char *buff; // a circular buffer for reading and writing
	size_t head; // position after write, says were information starts
	size_t tail; // position after read, total information avalable
	size_t sizeReal; // the real size of the buffer
		// this takes into acount the overflow
		// overflow will always be BUFFSIZE for now, eventually introduce ways to change this number
} cirBuff;


/*
 * holds information about the socket
 * specifically the localBindPort address and remotePort information and socket file descriptor
 * also contains information on the name of the connection, null if unspecified and will use place in the array
 *
 */
typedef struct socketInformation
{
	int socketFd; // socket file descriptor -1 if no connection is pressent

	int localPort; // the port that will be bound to
		// if -1 will be bound to any old port
	int remotePort; // the port that I will be connecting to or reseving a connect
		// if -1 will accept any port or will use defalt port number when connecting
		// XXX if using a unix domain socket remote port does not matter???
	char *remoteAddress; // name of the remote address that I am connecting to
		// could be a file path in the case of domain sockets
	
	int protocolOption; // my own enum representing the type of connection
	int ipType; // anothe enume representing ip4 or ip6
	/*
	int socketDomain; // IPv4 IPv6 or Unix domain 
	int socketType; // DGRAM or STREAM
	int socketProtocol; // the differnt layer cake of netorking levels
	*/

	char *name; // the name of the conection, defaults to NULL but can be configured by the user
		// if null the name will be the index in the array of socket Information
	struct sockaddr_in remoteTable; // address relating the remote address used for connectionless sockets
	
	cirBuff outBuffer; // the out buffer for the sockets
	cirBuff inBuffer; // used to stop messeges until it gets the correct deliminter
} sockInfo;

/*
 * struct for the left or right connections
 * contains address information localBindPort and remotePort remoteAddress and localListen port
 * Along with the listening socket information and the connection socket lists 
 */
typedef struct sideInformation
{
	sockInfo **socketList; // list of sockets
	int listSize;
	int connectionCount; // the total number of connections I currently have
	int maxConnections; // the maximum number of connections I am alloud to pick up from listening
		// -1 if allowing any number of connections
		// can be changed by command
	int listenSocket; // file descripitor to the listening socket
		// -1 if not listening 
	int listenPort; // the port that I will be listening on
		// -1 will use default port number
	char *localAddress; // usually null only used for domain sockets

	int listenProtocol;
	int listenIpType;
	int inWindow; // used to define how I print the information in the borders
} sideInfo; 

/*
 * Holds information on the lr and rl intractions
 * this is the pipe array and also the strip functionality 
 * all information will need to go through this even if there is no pipe
 * changing from older versions of piggy to now avoid passing through a pipe unless it is an external filter
 */ 
typedef struct directionInformation
{
	struct sideInformation *inSide; // side that I am receving input
	struct sideInformation *outSide; // side I am sending information
	
	enum STRIP filterIn; // if I will be striping character from this guy
	
	int inWindow; // window to write incoming
	int outWindow; // window to write outgoing
	int *writingFileList; // a pointer to the list of files to write to for logging purposes

	int windowLink[2]; // file descriptors for piping to the external filter
		// when they are both -1 we do not have an external filter
		// [0] reading [1] writing

	pid_t external; // the pid of the child process that is the external filter
		// -1 if has no external filter
	struct directionInformation *loopDir; // pointer to the loop direction
	enum ISLOOP *looping; // if I am looping or not
	enum ISLOOP loopComp; // compare to this to tell if I am looping
	
	int inRecordMode; // 0 means no 1 means record mode on
	char *delimiter;
	
	cirBuff inBuffer; // comunal buffer for the in direction
		// really only used if I have an external filter in that direction
	
	char **externalCommand;
	int externalCommandCount;
	
} directionInfo;

/*
 * struct that represetns the list of file descriptors
 * uses the fileDescriptoNode
 *
 * has a list but also incorperates important elements of writing and stuff
 * thus it is fdInfo
 *
 * file descriptors this guy is responsible for is
 * 		all socket file descriptors
 * 		all iternalPipes
 * 		all writingFileDescriptors
 * 		the readingFileDescriptor
 * 		the scripteFileDescriptor
 *
 * assumes all nodes have been malloced
 */
typedef struct fildDescriptorList
{
	fd_set masterReadSet; // the master read and write sets
	fd_set masterWriteSet;
	
	int maxFileDescriptor; // the maximum file discriptor
		// should be noted that this is NOT max+1 for the select call
} fdInfo;

// struct that is used to keep track of comand line arguments and other information
typedef struct piggyInformation
{
	directionInfo lr; // the top or bottom sections of the screen... kind of primarily used for messege passing
	directionInfo rl;

	sideInfo left; // the actual side connection information, pointed to by the directionInfo
	sideInfo right; 

	int readFd; // file descriptor for reading files
	int scriptFd; // file descriptor for running a script from a file
	
	int writingFileList[4]; // witing file list to write output to a text file
		// goes off WINDOWNAME-1 gives the index for writing the textFile
	
	fdInfo fdList; // the file descriptor list
		// contains the sets and max file descriptor
	int mode; // the mode I am in for ncurses input or command
		// 0 for input else I am in some command mode
	
	directionInfo *middle; // pointer to lr or rl that defines were stdout goes
	
	enum ISLOOP looping; // if I am looping or not can only loop in one direction

	char *serverName; // used with the tmux sessions if it is NULL then I am not running within tmux

} pigInfo;

// method headers
char *getNameFromAddress(struct sockaddr_in *table);

void closePiggy(sideInfo *sid, fdInfo *fdList, int closeType, int argc, char **argv);

int bindLocalSocket(int localPort, int protocolOption, int ipType, char *localAddress);

void startScripting(pigInfo *lore, char *fileName);

int checkPort(int portus);

// File Set and File List operations
void setMaxFile(fdInfo *fileList);

void addToFileSet(int fileDescriptor, fd_set *fileSet, fdInfo *fdList);

int clearFileSet(int fileDiscriptor, fdInfo *fileList);

int canReadInDirection(directionInfo *dir);

int canReadOutDirection(directionInfo *dir);

int connectTo(fdInfo *fdList, sockInfo *path);

int acceptTo(fdInfo *fdList, sideInfo *sid);

int sendInDirection(char *messege, size_t length, directionInfo *dir, fdInfo *fdList, fd_set *writeSet);

void killChild(directionInfo *dir, fdInfo *fdList);

void toLowerCase(char *line);

void startReading(pigInfo *lore, char *fileName);

void removeDomainSockets(sideInfo *sid);

void processDelimiter(directionInfo *dir, int argc, char *arg);

void updateBorderSideInfo(sideInfo *sid);






