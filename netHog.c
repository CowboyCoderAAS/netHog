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
 * 6/08/16
 * Piggy5.0 NetHog
 * Advisor: Dr. Michael Meehan
 *
 * the super pig adding new featurs starting from piggy5
 * will add multiple connections from listening and unix domain sockets
 * so many things
 */

// includes are in curesePiggy.h along with structs and enums
#include "curseNetHog.h" // curses methods
#include "netHog.h"

#if DEBUG
	int happend = 0; // used to print information after each loop
#endif

int bufferFlag = 0; // if there is currently information in the buffer this flag will be incremented up
int frameFlag = 0;  // if a change occured and I need to update the windows in ncurses, will set to 1

// methods 

/*
 * helper method that will close does the pipes if files exist in them
 * for the dir info and stuff
 * used when opeining internal pipes
 * and forking off to other external filters
 * 
 * works if the pipes 
 */ 
void closePipes(directionInfo *dir, fdInfo *fdList)
{
	if(dir->windowLink[0] > 0) // something was being read, removing it
	{
		#if DEBUG
			printfs("I am closing the read file descriptor\n");
		#endif
		close(dir->windowLink[0]); // closes the file discriptor
		clearFileSet(dir->windowLink[0], fdList); // removes and sets maximum
		dir->windowLink[0] = -1;
	}
	if(dir->windowLink[1] > 0) // something being written to 
	{
		#if DEBUG
			printfs("I am closing the write file descriptor\n");
		#endif
		close(dir->windowLink[1]);
		clearFileSet(dir->windowLink[1], fdList); // cannot actually be in fdList natural storing
		dir->windowLink[1] = -1; // sets to -1
	}
}

/*
 * a method used to call pipe and create communication between windows of piggy
 * works off of the windoLink array 
 *
 * adds the appropreate file discriptors to the fileList
 *
 * if the file discriptors in the list are present, will close them and remove them from fdList
 *
 */ 
void openPipe(directionInfo *dir, fdInfo *fdList)
{
	closePipes(dir, fdList); // removes anything that was previously ocupying the pipe
	// calls pipe and adds them to there appropreate locations
	if(pipe(dir->windowLink) < 0)
	{
		fprintf(stderr, "pipe error\n");
		exitCurses(EXIT_FAILURE);
	}
	addToFileSet(dir->windowLink[0], &fdList->masterReadSet, fdList);
	addToFileSet(dir->windowLink[1], &fdList->masterWriteSet, fdList);
}

/*
 * helper method that will put the char *adder in address
 * calls malloc
 * frees the previous address
 */ 
char *addAddress(char *address, char *adder)
{
	int length = strlen(adder);
	if(address != NULL) // first need to free the previous version
		free(address);
	address = malloc(sizeof(char)*(length+1)); // obtains memory
	memset(address, 0, (length+1));

	strncpy(address, adder, length);
	address[length] = '\0'; // puts the null character in
	return address;
}

// yo get the max fro this one
int maximum(int num, ...)
{
	va_list arron; // you know this is surpising but first time using this variable name in my program, so unlike me
	int i;
	if(num==0) // they gave me no arguments I think
		return 0; // can never go wrong with the nobal zero
	va_start(arron, num);
	int max = va_arg(arron, int);

	for(i=1; i<num; i++) // process the rest of the numbers
	{
		int temp = va_arg(arron, int);
		if(temp > max)
			max = temp;
	}
	// cleaning memory
	va_end(arron);
	return max;
}

// returns the minumum bro
int minimum(int num, ...)
{
	va_list arron;
	int i;
	if(num==0) // they gave me no arguments I think
		return 0; // can never go wrong with the nobal zero
	va_start(arron, num);
	int min = va_arg(arron, int);

	for(i=1; i<num; i++) // process the rest of the numbers
	{
		int temp = va_arg(arron, int);
		if(temp < min)
			min = temp;
	}
	// cleaning memory
	va_end(arron);
	return min;
}

/*
 * converts the string source into an integer and playses it in destination
 * returns 1 if it was succesfull zero otherwise
 * destination will not be changed it it was unable to convert the number
 */ 
int convertToInt(int *destination, char *source)
{
	int result;
	int notFound = 0; 
	if((result = atoi(source)) == 0)
	{
		if(strlen(source)!=1 || source[0]!='0')
		{
			notFound = 1; // it was not zero
		}
	}
	if(notFound==0) // result is the the number
	{
		*destination = result;
		return 1; // sucess
	}
	return 0; // did not work out
}

/*
 * helper function that will create the circular buffer
 * initate the head and the tail
 * calls malloc to get space for the buffer string space
 * sizeReal will the the full size of the buffer including the overflow
 * will always add +1 to the sizeReal to prevent null characters messing everything up
 */ 
void createCircularBuffer(cirBuff *casey, size_t sizeReal)
{
	casey->buff = (char *) malloc(sizeof(char)*(sizeReal+1));
	casey->sizeReal = sizeReal;
	casey->head = 0;
	casey->tail = 0;
}

/* 
 * helper method
 * creates a sockInfo with the given peramaters
 * calls malloc and populates it with the given information
 *
 * idea is this is called before a connection is made passing these the actual struct into the connnect
 * if socketFd is -1 don't have a connect
 * if port is -1 use default values
 * if name is null it does not have one
 *
 * the final argument sizeReal is the size that the out buffer will be since the in buffer is a communal one
 *
 * when given a char * goes under assumption that I must malloc it again to get its information safly 
 */ 
sockInfo *createSocketInformation(int socketFd, int localPort, int remotePort, char *remoteAddress, 
		int protocolOption, int ipType, char *name, size_t sizeReal)
{
	sockInfo *path = malloc(sizeof(sockInfo));
	
	path->socketFd = socketFd; // putting information into the struct
	path->localPort = localPort;
	path->remotePort = remotePort;
	path->protocolOption = protocolOption;
	path->ipType = ipType;

	path->remoteAddress=NULL;
	if(remoteAddress!=NULL) // need to malloc and coppy
		path->remoteAddress = addAddress(path->remoteAddress, remoteAddress);
	path->name=NULL;
	if(name!=NULL)
		path->name = addAddress(path->name, name);

	createCircularBuffer(&path->outBuffer, sizeReal); 
	createCircularBuffer(&path->inBuffer, sizeReal);
	
	return path;
}

/*
 * helper function that iwll free the memory on socketInformation
 * also frees the buffer
 */ 
void freeSocketInformation(sockInfo *path)
{
	// attempts to free the strings
	if(path->remoteAddress!=NULL)
		free(path->remoteAddress);
	if(path->name!=NULL)
		free(path->name);
	if(path->socketFd>0) // I had a connection and now I will need to close it
		close(path->socketFd);
	free(path->outBuffer.buff);
	free(path->inBuffer.buff);
	free(path); // free the struct now
}

/*
 * helper method to populate a side information struct
 * does not call malloc, simply passed a pointer to its place in memory
 * also hense forth generic sideInfo structs will be call sid
 */ 
void createSideInformation(sideInfo *sid)
{
	sid->listSize = STARTSIZE;
	sid->socketList = malloc(sizeof(sockInfo *)*(STARTSIZE+1)); // obtain a list
	sid->connectionCount = 0; // I just made it theres no connections right now
	sid->maxConnections = -1;
	sid->listenSocket = -1; // no actual connection yet
	sid->listenPort = -1; // gona use default unless specified
	sid->localAddress = NULL; 
	
	sid->listenProtocol = O_TCP;
	sid->listenIpType = T_IP4;
	
	
	/*sid->listenDomain = AF_INET; // defaults to a tcp connection
	sid->listenType = SOCK_STREAM;
	sid->listenProtocol = IPPROTO_TCP;
	*/
		// will be filed with eather a /tmp address or a user defined address
	
}

/*
 * helper method
 * fills in the given information to the directionInfo dir
 * will call maloc to get the char*buff for the messeges and stuff
 */ 
void createDirectionInformation(directionInfo *dir, sideInfo *inSide, sideInfo *outSide, int inWindow, int outWindow, 
		int *writingFileList, directionInfo *loopDir, size_t sizeReal, enum ISLOOP *looping, enum ISLOOP loopComp)
{
	dir->inSide = inSide; // setting the variables to given values
	dir->outSide = outSide;
	dir->inWindow = inWindow;
	dir->outWindow = outWindow;
	dir->writingFileList = writingFileList;

	dir->filterIn = STRIPNO; // default to not strip at all
	dir->windowLink[0] = -1; // no connection no pipe
	dir->windowLink[1] = -1;
	dir->external = -1; // external pid does not exist
	
	dir->loopDir = loopDir;
	dir->looping = looping; // can only loop in one direction
	dir->loopComp = loopComp;
	dir->inRecordMode = 0; // start out not in record mode
	
	dir->delimiter = (char *) malloc(sizeof(char)*5); // delimiter is set to be the newline by default
	dir->delimiter[0] = '\n'; // defaults to the new line character
	dir->delimiter[1] = '\0';

	createCircularBuffer(&dir->inBuffer, sizeReal);
}

/* 
 * creates the struct information initalizing it to the default values
 * calls malloc
 * should not have to worry about freeing struct
 *
 */ 
pigInfo *createPigInformation()
{
	int i;
	pigInfo *lore = malloc(sizeof(pigInfo)); // get me that memory
	memset(lore, 0, sizeof(pigInfo));

	createSideInformation(&lore->left);
	createSideInformation(&lore->right);
	lore->left.inWindow = INLEFT; // for printing border information
	lore->right.inWindow = INRIGHT;
	
	for(i=0; i<4; i++) // defaulting the writing file log list
		lore->writingFileList[i]=-1;
	// can acess writingFileLIst at WINDOW-1

	createDirectionInformation(&lore->lr, &lore->left, &lore->right, INLEFT, OUTRIGHT, 
			lore->writingFileList, &lore->rl, BUFFSIZE*5, &lore->looping, RIGHTLOOP);
	createDirectionInformation(&lore->rl, &lore->right, &lore->left, INRIGHT, OUTLEFT, 
			lore->writingFileList, &lore->lr, BUFFSIZE*5, &lore->looping, LEFTLOOP);

	lore->readFd = -1; // the file descriptors for reading and scripting default to -1 since I don't have them yet
	lore->scriptFd = -1;

	lore->mode = 1; // start in command mode
	
	// just creates the fdList information inside of create pig
	FD_ZERO(&lore->fdList.masterReadSet); // zeroing out the sets
	FD_ZERO(&lore->fdList.masterWriteSet); 
	lore->fdList.maxFileDescriptor = 3; // start maxFD at three to include stdin, stdout, and stderror
		// ok to not be the actuall max, but good starting out value
	
	lore->middle = &lore->lr; // start out by going left to the right by default
	return lore;
}


/*
 * attempt to add the socket information to the sideInfo list stuff
 * returns the integer that it was added
 */ 
int addToSocketList(sideInfo *sid, sockInfo *path)
{
	int found = 0; // if I found a connection among the NULL's
	int i;
	for(i=0; i<sid->listSize; i++) // loop through looking for a NULL and then add it there
	{
		if(sid->socketList[i]==NULL) // found one
		{
			sid->socketList[i] = path;
			found = 1; // set that I found it
			break; // no need to loop any more 
		}
	}
	if(!found) // I could not find any NULL'S so I will have to re-allocate memory 
	{
		sid->listSize*=2; // doubles the size of the list
		sid->socketList = realloc(sid->socketList, sizeof(sockInfo *)*(sid->listSize+1));
		sid->socketList[i] = path; // since I was the end I can safly do this
	}
	sid->connectionCount++; // I added a connection
	updateBorderSideInfo(sid);
	return i;
}

/*
 * helper method that will check to make sure the port is within range
 * if the port is not in range will return -1 instead
 * 		after first printing an error message
 * if the port is in range will return the port number
 * if the port is -1 will just return -1 since that is the tenicall default
 */ 
int checkPort(int port)
{
	if((port < 1 || port > 65536) && port != -1) // port out of range
	{
		printfs("port %d is out of range\n");
		return -1;
	}
	return port;
}

/*
 * processes the port makes sure that it is an actual integer and is in the proper range
 * prints an error if it is out of range or is not an integer
 * if error occurs returns the oldPort instead
 */ 
int processPort(int oldPort, char *newPort)
{
	int result = oldPort;
	if(convertToInt(&result, newPort)==0) // was not a proper int
	{
		printfs("%s is not a valid port number\n");
		return oldPort;
	}
	// checking if in correct range
	if(checkPort(result)<0) // checking failed returning old port
		return oldPort;
	return result; // passed all of the tests
}

/*
 * helper method that will try to convert newNum into a number and return it
 * print error messege and return oldNum if it did not work
 */
/*
int processNumber(int oldNum, char *newNum)
{
	int result;
	if((result = atoi(newNum))==0)
	{
		printfs("%s is not a number\n");
		return oldNum;
	}
	return result; // got the number
}
*/ // old code being replaced with specialized functions
/*
 * will process the string and determin if it is one of the protocols that I support or not
 * use one enume to define the type since they are all somewhat related
 */ 
int processSocketOptions(int protocolOld, char *argument)
{
	toLowerCase(argument);
	if(!strcmp(argument, "ip"))
	{
		return O_IP;
	} else if(!strcmp(argument, "unix"))
	{
		return O_UNIX;
	} else if(!strcmp(argument, "icmp"))
	{
		return O_ICMP;
	} else if(!strcmp(argument, "raw"))
	{
		return O_RAW;
	} else if(!strcmp(argument, "tcp"))
	{
		return O_TCP;
	} else if(!strcmp(argument, "udp"))
	{
		return O_UDP;
	} else if(!strcmp(argument, "igmp")) // new guy for the multicast group
	{
		return O_IGMP;
	}
	// else they did not give a valid string
	printfs("%s is not a valid domain argument\n", argument);
	return protocolOld;
}

/*
 * processes the string of argument into the ip type
 * returns oldIp if what they gave does not fit what I am looking for
 */ 
int processSocketIpType(int oldIp, char *argument)
{
	toLowerCase(argument);
	if(!strcmp(argument, "ip4"))
	{
		return T_IP4;
	}
	else if(!strcmp(argument, "ip6"))
	{
		return T_IP6;
	}
	// else what they gave was not correct
	printfs("%s is not a valid ip type use ip4 or ip6\n", argument);
	return oldIp;
}

/*
 * returns 1 if it is a connectionless socket type
 * 0 otherwise
 * a connectionless socket is one that is not unix nore tcp everything else is connectionless
 */ 
int isConnectionless(int protocolOption)
{
	return protocolOption != O_UNIX && protocolOption != O_TCP;
}

/*
 * will convert my protocal
 * option is the PROTOCAL enum and ipType is the IP_TYPE enume
 * changes the values in the pointers
 */ 
void convertSocketOptions(int option, int ipType, int *domain, int *type, int *protocol)
{
	if(option==O_UNIX) // unix option don't give a dam about ipType
	{
		*domain = AF_UNIX;
		*type = SOCK_STREAM; // XXX needs testing don't know if this argument type will work
		*protocol = 0;
		return; // all done here
	}
	*domain = AF_INET;
	if(ipType==T_IP6) // actually dealing with ip6
		*domain = AF_INET6;
	
	switch(option) 
	{
		case O_IP:
			*type = SOCK_DGRAM; // XXX needs verification that this is the correct one
			if(ipType==T_IP4)
				*protocol = IPPROTO_IP;
			else
				*protocol = IPPROTO_IPV6;
			break;
		case O_ICMP:
			*type = SOCK_DGRAM;
			*protocol = IPPROTO_ICMP;
			break;
		case O_RAW: // XXX needs work don't understand how raw is supposed to work
			*type = SOCK_RAW; // no Ip so I use this
			*protocol = IPPROTO_UDP;
			*domain = PF_INET; // does not work with ip but packets instead
			break;
		case O_TCP:
			*type = SOCK_STREAM;
			*protocol = IPPROTO_TCP;
			break;
		case O_IGMP: // do the same as udp for now
		case O_UDP:
			*type = SOCK_DGRAM;
			*protocol = IPPROTO_UDP;
			break;
	}
	// now the type protocal and domain should be set
}

/*
 * function that will process a connection request through using getopt_long
 * used both in process Arguments and command process functions
 * the argc and argv are just like they would be on the command line
 * except that the arguments actually start at zero index instead of one
 *
 * XXX right now have to put the type and domain as numbers, going to add more versitility latter on
 * XXX add check for maximum connection limit
 */ 
void processConnection(sideInfo *sid, fdInfo *fdList, int argc, char *argv[])
{
	// creates a sockInfo with default information, setting up IPV4 TCP connection by default
	sockInfo *path = createSocketInformation(-1, -1, DEFAULTPORT, NULL, O_TCP, T_IP4, NULL, BUFFSIZE*2);
		// able to store 1-2 full messeges before stop getting information

	optind = 0; // set to zero instead of one because arguments start
	
	struct option connection_options[] = 
	{
		{"addr", required_argument, 0, 'a'},
		{"address", required_argument, 0, 'a'},
		{"raddr", required_argument, 0, 'a'},
		{"ad", required_argument, 0, 'a'},
		{"rp", required_argument, 0, 'r'},
		{"remoteport", required_argument, 0, 'r'},
		{"lp", required_argument, 0, 'l'},
		{"name", required_argument, 0, 'n'},
		/*{"domain", required_argument, 0, 'd'},
		{"type", required_argument, 0, 't'},
		{"protocol", required_argument, 0, 'p'},*/
		{"protocol", required_argument, 0, 'p'},
		{"iptype", required_argument, 0, 'i'},
		{0, 0, 0, 0}
	};
	while(1) // until I run out of options to process
	{
		/*
		#if DEBUG
			printfs("optind before getopt_long %d\n", optind);
		#endif
		*/
		int option_index = 0;
		int c = getopt_long(argc, argv, "a:r:l:n:p:i:", connection_options, &option_index);
		if(c==-1) // all done
		{
			/*
			#if DEBUG
				printfs("breaking from command loop\n");
				printfs("errno is %s\n", strerror(errno)); // obtain why the connection failed
				printfs("the optind is %d\n", optind);
				printfs("the current option at option index %d is %s\n", option_index, argv[option_index]);
			#endif */
			break;
		}
		/*
		#if DEBUG
			printfs("the c option is %c\n", c);
			printfs("the argument is %s\n", optarg);
			printfs("optind after getopt_long %d\n", optind);
		#endif
		*/
		switch(c)
		{
			case 0: // option set a flag
				break;
			case 'r': // remotePort
				path->remotePort = processPort(path->remotePort, optarg); 
				break;
			case 'l': // localPort
				path->localPort = processPort(path->localPort, optarg);
				break;
			case 'a': // remoteAddress
				/*
				#if DEBUG
					printfs("the remote address is %s\n", optarg);
				#endif */
				path->remoteAddress = addAddress(path->remoteAddress, optarg);
				break;
			case 'n': // name of connection
				path->name = addAddress(path->name, optarg); // actually just calling malloc to get seperate string
				break;
			case 'p': // protocalOption
				path->protocolOption = processSocketOptions(path->protocolOption, optarg);
				break;
			case 'i':
				path->ipType = processSocketIpType(path->ipType, optarg);
				break;
			/*
			case 'd': // domain
				path->socketDomain = processNumber(path->socketDomain, optarg);
				break;
			case 't': // type
				path->socketType = processNumber(path->socketType, optarg);
				break;
			case 'p': // protocol used
				path->socketProtocol = processNumber(path->socketProtocol, optarg);
				break;
			*/
			case '?': // found error
				printfs("do not understad argument\n");
				break;
		}
	}
	// finished looping time to check if it is a connection that I can even make
	if(path->remoteAddress==NULL) // no remote address can't even make an attempt at a connection
	{
		printfs("no remoteAddress given cannot attempt connection\n");
		freeSocketInformation(path);// time to free information
		return;
	}
	// now time to attempt the connection
	int result = connectTo(fdList, path);
	if(result==0) // error did not occur and was able to connect
	{
		addToSocketList(sid, path); // adds it the the list
	}
	// else error did occur, but messege was printed and memory was already freed in connectTo
}

/*
 * process the listen option argument
 * used to tell what connections I am willing to acept
 *
 * XXX don't need to check protocal domain and type becuase it won't be able to connect...?
 * XXX needs upgrading and more versitility, right now want the program to compile so I can test
 */ 
void processListenOption(sideInfo *sid, int argc, char *argv[])
{
	optind = 0;
	sockInfo *path = createSocketInformation(-1, -1, -1, NULL, O_TCP, T_IP4, NULL, BUFFSIZE*2);

	static struct option listen_options[] = // options to be processed 
	{
		{"rp", required_argument, 0, 'r'},
		{"addr", required_argument, 0, 'a'},
		{0, 0, 0, 0}
	};
	
	while(1)
	{
		int option_index = 0;
		int c = getopt_long(argc, argv, "r:a:", listen_options, &option_index);

		if(c==-1) // all done
			break;
		switch(c)
		{
			case 0: // option set a flag
				break;
			case 'r':
				path->remotePort = processPort(path->remotePort, optarg);
				break;
			case 'a':
				path->remoteAddress = addAddress(path->remoteAddress, optarg);
				break;
			case '?':
				printfs("do not understand argument %s\n", optopt);
		}
	}
	if(path->remotePort == -1 && path->remoteAddress==NULL) // did not actually do anything to the connection, freeing node
	{
		printfs("no arguments were given will accept any connection\n");
		freeSocketInformation(path); // since I am not using it I must free it
	} 
	else // I add it to my list, since socketFd is -1 I will know to accept to it
	{
		addToSocketList(sid, path); // adds to the list
	}
}


/*
 * function that will listen
 * processes arguments just like processConnect
 * first checks if there is not a connection already on that side
 */ 
void processListen(sideInfo *sid, fdInfo *fdList, int argc, char *argv[])
{
	if(sid->listenSocket>0)
	{
		printfs("Already listening on that side\n");
		return;
	}
	optind = 0; 

	struct option listen_options[] = // options ot be processed
	{
		{"lp", required_argument, 0, 'l'},
		{"localPort", required_argument, 0, 'l'}, // using two deffenitions for user conveneance
		{"addr", required_argument, 0, 'a'},
		/*{"domain", required_argument, 0, 'd'},
		{"type", required_argument, 0, 't'},*/
		{"iptype", required_argument, 0, 'i'},
		{"protocol", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};

	while(1) // loop until I'm out of arguments
	{
		int option_index = 0;
		int c = getopt_long(argc, argv, "p:a:l:i:", listen_options, &option_index);
		/*
		#if DEBUG
			printfs("listen optind is %d\n", optind);
		#endif
		*/
		if(c==-1) // all done
			break;
		switch(c)
		{
			case 0: // option set a flag
				break;
			case 'l': // local port
				sid->listenPort = processPort(sid->listenPort, optarg);
				break;
			case 'a': // bound address, really only used for domain sockets
				/*
				#if DEBUG
					printfs("I am changing my local address to %s\n", optarg);
				#endif
				*/
				sid->localAddress = addAddress(sid->localAddress, optarg);
				break;
			/*
			case 'd': // domain
				sid->listenDomain = processNumber(sid->listenDomain, optarg);
				break;
			case 't': // socket type
				sid->listenType = processNumber(sid->listenType, optarg); // XXX eventually replaced by process domain
				break;
			case 'c': // protocal
				sid->listenProtocol = processNumber(sid->listenProtocol, optarg);
				break;
			*/
			case 'p':
				sid->listenProtocol = processSocketOptions(sid->listenProtocol, optarg);
				break;
			case 'i':
				sid->listenIpType = processSocketIpType(sid->listenIpType, optarg);
				break;
			case '?':
				printfs("Do not understand argument %s\n", optopt);
				break;
		}
	}
	// now to listen on that socket
	acceptTo(fdList, sid); // attempts to listen on that socket
		// error messege will be printed inside method if failed
}

/* 
 * returns the pointer to the first non-WiteSpace
 * null on hiting \n
 */
char *getFirstNonWhite(char *line)
{
	int i;
	for(i=0; line[i]!='\n' && line[i]!='\0'; i++)
	{
		if(line[i]!=' ' && line[i]!='\t')
			return (line+i);
	}
	return NULL; // hit \n
}

/*
 * same as getFirstNonWhite
 * but returns the instance of the first white
 * also will return when hitting the '\n' character
 * should aways hit \n before \0
 */
char *getFirstWhite(char *line)
{
	int i;
	for(i=0; line[i]!='\n' && line[i]!='\0'; i++)
	{
		if(line[i]==' ' || line[i]=='\t')
			return (line+i);
	}
	return (line+i); // I hit \n
}

/*
 * helper function that obtains the next quote character
 * goes under the assumption that the first quote character can be found at position zero of line
 */ 
char *getNextQuote(char *line)
{
	int i;
	for(i=1; line[i]!='\n' && line[i]!='\0'; i++)
	{
		if(line[i]=='"') // found the quote character and returning its index
			return line+i;
	}
	// else I hit the end of the string or somthing without finding any quotes, don't have an easy way to recover
	// gust going to return the end of the string
	return line+i;
}

/*
 * converts the string to lower case
 * just like the method names implys
 */ 
void toLowerCase(char *line) 
{
	int length = strlen(line);
	int i;
	for(i=0; i<length; i++)
		line[i] = tolower(line[i]);
}

/*
 * helper method that will free the arguments from create args
 */ 
void freeArgs(int argc, char *argv[])
{
	// free my arguments that I malloced
	int k;
	for(k=0; k<argc && argv[k] != NULL; k++)
		free(argv[k]); // freeing saves lives
	
	free(argv); // free the array itself
}


/*
 * helper method that will take the string line and strip it by white space to obtain 
 * argc is a pointer because I will need it from the calling function
 * used for processing commands and such
 *
 * if hasQuotes is 1 then will keep the quotes around teh arguments, else will remove the quotes but treat them as
 * one argument
 */ 
char **createArgs(int *argc, char *line, int hasQuotes)
{
	int argCount = 0;
	int argLength = 5; // inital value for my argument calls
	char **args = malloc(sizeof(char *)*(argLength+1));
	char *next;
	memset(args, 0, sizeof(char *) * (argLength+1)); // clears memory
	
	while((line = getFirstNonWhite(line)) != NULL)
	{
		if(line[0] == '"') // starts in quote, treet the entire quote as a single argument
		{
			next = getNextQuote(line);
			if(!hasQuotes)
				line = &line[1];
		} else // was a regular character and will process to the next white space
		{
			next = getFirstWhite(line); // obtains the string
		}

		args[argCount] = malloc((next-line+1)*sizeof(char)); // obtains the first one, fenceposet
		strncpy(args[argCount], line, next-line);
		args[argCount][next-line] = '\0'; // strncpy does not add the null termination for strings
		argCount++;
		if(argCount==argLength) // must realloc
		{
			argLength = argLength*2; // double the size double the fun
			args = realloc(args, (sizeof(char *)*(argLength+1)));
		}
		line = next; //  moves it up
		if(line[0]=='"' && !hasQuotes)
			line = getFirstWhite(line);
	}
	*argc = argCount;
	
	return args; // pointer to the list
}

/*
 * because I am using getlong opt it will always avoid the 0th element of the array
 * to prevent this I will just 
 * XXX does not handle quotes
 */ 
char **createCommandArgs(int *argc, char *line)
{
	int argCount = 0;
	int argLength = 5; // inital value for my argument calls
	char **args = malloc(sizeof(char *)*(argLength+1));
	char *next;
	memset(args, 0, sizeof(char *) * (argLength+1)); // clears memory
	
	// now to set up the first one
	args[argCount] = malloc(sizeof(char)*5); // sets the zero element to be empty
	args[argCount][0]='a';
	args[argCount][1]='\0'; // puts in a singular string
	argCount++;
	while((line = getFirstNonWhite(line)) != NULL)
	{
		next = getFirstWhite(line); // obtains the string

		args[argCount] = malloc((next-line+1)*sizeof(char)); // obtains the first one, fenceposet
		strncpy(args[argCount], line, next-line);
		args[argCount][next-line] = '\0'; // strncpy does not add the null termination for strings
		argCount++;
		if(argCount==argLength) // must realloc
		{
			argLength = argLength*2; // double the size double the fun
			args = realloc(args, (sizeof(char *)*(argLength+1)));
		}
		line = next; //  moves it up
	}
	*argc = argCount;
	return args; // pointer to the list

}

/*
 * creates the struct Information and sets the values 
 * to the given arguments passed in
 * will kill program if fatale error has occured
 * but will revert to default senarios if problem happes
 *
 */ 
pigInfo *processArguments(int argc, char *argv[])
{
	pigInfo *lore = createPigInformation(); // sets up with default values
	
	#if DEBUG // going to print out the list
		int k;
		printfs("command arguments [");
		for(k=0; argv[k]!=NULL; k++)
		{
			printfs("%s", argv[k]);
			if(argv[k+1]==NULL)
				printfs("]\n");
			else
				printfs(", ");
		}
		//sleep(10);
	#endif

	struct option long_options[] = // setting up struct to process the arguments
	{
		// setting flags in memory options
		// now switch condition
		{"connectr", required_argument, 0, 'a'},
		{"connectl", required_argument, 0, 'b'},
		{"listenr", required_argument, 0, 'c'},
		{"listenl", required_argument, 0, 'd'},
		{"listenopr", required_argument, 0, 'e'}, // the type of connection I am listening for
		{"listenopl", required_argument, 0, 'f'},
		{"loopl", no_argument, 0, 'g'},
		{"loopr", no_argument, 0, 'h'},
		{"outputl", no_argument, 0, 'i'}, // changes starting output direction
		{"outputr", no_argument, 0, 'j'},
		{"read", required_argument, 0, 'k'},
		{"source", required_argument, 0, 'l'},
		{"recordlr", optional_argument, 0, 'm'}, // will put the left to right side into record mode
			// really only defined fr reading and not sending currently XXX
		{"recordrl", optional_argument, 0, 'n'},
		// finishing with zero don't know why
		{0, 0, 0, 0}
	};
	#if DEBUG
		printfs("original optind is %d\n", optind);
	#endif

	optind = 1; // in case I used it before
	opterr = 0;
	int myOptind = optind; // doing some inseption stuff by calling getopt_long nested 
	int argCount = 0;
	char **argList = NULL;
	
	while(1) // loop until I break out by finishing all argmuents
	{
		int option_index = 0;
		int c = getopt_long(argc, argv, "a:b:c:d:e:f:ghijk:l:m::n::S:H:", 
				long_options, &option_index); // calls long option to parse
		
		if(c==-1) // all done
			break;
		switch(c)
		{
			case 0: // option set a flag
				break;
			case 'a': // connectr
				myOptind = optind; // saving my optind so I can use it after function call
				argCount = 0;
				argList = createCommandArgs(&argCount, optarg); // will mess up optarg
				processConnection(&lore->right, &lore->fdList, argCount, argList);
				optind = myOptind; // continue were I left off
				break;
			case 'b': // connectl
				myOptind = optind;
				argCount = 0;
				argList = createCommandArgs(&argCount, optarg);
				processConnection(&lore->left, &lore->fdList, argCount, argList);
				optind = myOptind;
				break;
			case 'c': // listenr
				myOptind = optind;
				argCount = 0;
				argList = createCommandArgs(&argCount, optarg);
				processListen(&lore->right, &lore->fdList, argCount, argList);
				optind = myOptind;
				break;
			case 'd': // listenl
				myOptind = optind;
				argCount = 0;
				argList = createCommandArgs(&argCount, optarg);
				processListen(&lore->left, &lore->fdList, argCount, argList);
				optind = myOptind;
				break;
			case 'e': // listenopr obtaining viable options for the connection
				myOptind = optind;
				argCount = 0;
				argList = createCommandArgs(&argCount, optarg);
				processListenOption(&lore->right, argCount, argList);
				optind = myOptind;
				break;
			case 'f':
				myOptind = optind;
				argCount = 0;
				argList = createCommandArgs(&argCount, optarg);
				processListenOption(&lore->left, argCount, argList);
				optind = myOptind;
				break;
			case 'g': // loopl
				lore->looping = LEFTLOOP;
				updateLoop(lore->looping);
				break;
			case 'h': // loopr
				lore->looping = RIGHTLOOP;
				updateLoop(lore->looping);
				break;
			case 'i': // set output direction to left
				lore->middle = &lore->rl;
				updateMiddleDir(-1);
				break;
			case 'j': // set output to right
				lore->middle = &lore->lr;
				updateMiddleDir(1);
				break;
			case 'k': // reading from file
				startReading(lore, optarg);
				break;
			case 'l': // reading source file and running that
				startScripting(lore, optarg);
				break;
			case 'm': // recordlr
				lore->lr.inRecordMode = !lore->lr.inRecordMode;
				if(optarg) // I have an option
					processDelimiter(&lore->lr, 2, optarg);
				// else do nothing since it was optional
				break;
			case 'n': // recordrl
				lore->rl.inRecordMode = !lore->rl.inRecordMode;
				if(optarg)
					processDelimiter(&lore->rl, 2, optarg);
				break;
			case 'S': // adds the server name so I can launch more scripts
				lore->serverName = addAddress(lore->serverName, optarg);
				break;
			case '?': // an error has occured time to print an error messege
				// printfs("do not understand argument %s\n", argv[option_index]);
				// XXX right now going to cheese the system by both combining my arguments
				// and the arguments for the bash script
				// won't print error messeege
				// also it was not working very well anyway
				break;
		}
		if(argList) // I made a list of arguments need to free them now
		{
			freeArgs(argCount, argList);
			argList = NULL; // so that I don't attempt to free on another pass of the loop
			argCount = 0; 
		}
	}
	return lore; 
}

/*
 * removes the socket at the given index of the socketList
 * free memory
 * if there was a connection present will decrement the connection count
 * else it was just an accept node and the count will not be touched
 */ 
void removeSocketInfo(sideInfo *sid, int index, fdInfo *fdList)
{
	if(sid->socketList[index]->socketFd > 0) // was a connection
		sid->connectionCount--;
	clearFileSet(sid->socketList[index]->socketFd, fdList);
	freeSocketInformation(sid->socketList[index]);
	sid->socketList[index] = NULL;
	updateBorderSideInfo(sid);
}

/*
 * returns the index that is assosiated with the given path
 * returns -1 on error
 */ 
int getIndexOfPath(sideInfo *sid, sockInfo *path)
{
	int i;
	for(i=0; i<sid->listSize; i++)
	{
		if(sid->socketList[i]==path)
			return i;
	}
	#if DEBUG
		printfs("could not find the path in getIndoxOfPath\n");
	#endif
	return -1;
}

/* circular buffer method functionality XXX requires testing */

/*
 * flushes the current buffer, seting tail and head to the begining
 * does not do any sending of the messages
 */ 
void flushBuffer(cirBuff *casey)
{
	casey->head = 0; 
	casey->tail = 0;
	memset(casey->buff, 0, casey->sizeReal);
}

/*
 * method that will detect if the current buffer already has a message in it
 * returns 1 if it has a message in it
 * returns 0 if there is no message currently in the buffer
 *
 * detects by if tail == head then it has no message
 */ 
int hasMessage(cirBuff *casey)
{
	return !(casey->tail == casey->head);
}

/*
 * obtains the curent avalable space in the buffer
 * this includes the overflow amount
 *
 * also always give the avaliable space -1
 * this is to prevent the tail being at the same position as the head
 *
 */ 
size_t getBufferSpace(cirBuff *casey)
{
	size_t curBuff; // the size of this buffer
	if(casey->tail >= casey->head)  // strate line calculation
		curBuff = (casey->sizeReal - (casey->tail - casey->head))-1;
	else
		curBuff = (casey->head - casey->tail)-1; // if the buffer wraps back around
	return curBuff; 
}

/*
 * function used to determin if I have space in the buffer
 * essentially that I have yet to go into my overflow space so I can still grab a full BUFFSIZE
 * 
 * returns the space avalable minus the overflow space
 * if no space is avaliable returns 0... so false
 */ 
size_t hasSpace(cirBuff *casey)
{
	size_t curSize; // total size in the buffer incuding overflow space
	if(casey->tail >= casey->head) // strate line calculation
		curSize = (casey->sizeReal - (casey->tail - casey->head))-1;
	else // buffer wraps back around itself
		curSize = (casey->head - casey->tail)-1;

	if(curSize>=BUFFSIZE) // I have just enough room to accept
		return curSize;
	return 0; // I am right now overflowed
}

/*
 * function that tests the buffers on the out direction
 * will return 1 if at least one buffer in the out direction can accept a messege
 * 		this also will inclue looping as well
 * if none of them are able then will return 0
 * XXX if there are no connections on one side I will return it is ok to send the messege
 * 		could be problamadic if the buffer is full in the other direction and I am looping
 */
int canReadOutDirection(directionInfo *dir)
{
	int found = 0;
	int i;
	for(i=0; i<dir->outSide->listSize; i++) // iterates through the entire socketList checking the buffers
	{
		if(dir->outSide->socketList[i]!=NULL && dir->outSide->socketList[i]->socketFd>0) // I have a socket
		{
			found++; // keeping track of the amount actually connected
			if(hasSpace(&dir->outSide->socketList[i]->outBuffer))
				return 1; // I have space
		}
	}
	if(found==0) // so there are no connections on this side so its alright
		return 1;

	if(*dir->looping == dir->loopComp)
		return canReadInDirection(dir->loopDir);
	return 0;
}

/*
 * function that will test the buffers to see if it is posible to read from the in file in that direction
 * if there is an external filter will only test the inBuff
 * 
 * return 0 if it cannot, else returns 1
 *
 */ 
int canReadInDirection(directionInfo *dir)
{
	if(dir->external<=0) // has no external filter for use thus we check the out direction
		return canReadOutDirection(dir);
	return getBufferSpace(&dir->inBuffer)>=BUFFSIZE; // if I have space for the file
	/*
	FILE* f = fdopen(dir->windowLink[1], "w");
	#if DEBUG
		if(f==NULL)
			printfs("the file is NULL\n");
		errno = 0;
	#endif
	size_t amount = __fpending(f);
	#if DEBUG
		if(errno!=0)
		{	
			printfs("errno was set and was %s\n", strerror(errno));
		}
	#endif
	int result;
	if(dir->inRecordMode) { // only if the buffer is empty will I send
		result = amount==0; 
	} else
	{// else only send if I have BUFFSIZE
		result = (__fbufsize(f)-amount)>=BUFFSIZE; // only if I can write that much
		#if DEBUG
			printfs("overall buffer size %d and amount %d, with result %d\n", __fbufsize(f), amount, result);
			if(errno!=0)
			{
				printfs("also the errno was set and was %s\n", strerror(errno));
			}
			sleep(1);
		#endif
	}
	return result;
	*/ // old code attempting to use file pointers, did not work
}

/*
 * adds the given message to the buffer at the position of the tail
 * assumes that length is <= the space avaliable in the buffer
 * 		also that length <= space avalable in the loop buffer as well
 * coppys the information from message 
 *
 * will perform wraping operations if needs to wrap back to the begining 
 * XXX needs testing
 *
 * if it does not have enough room in the buffer will chop off part of the messege
 * this is for the case of the circular buffer does not have enough space
 * 		primaraly used for the outBuffer in the sockets
 */ 
void addToBuffer(cirBuff *casey, char *message, size_t length)
{
	size_t remaining = getBufferSpace(casey); // the amount I have remaining
	if(length>remaining) // don't have enough space will lose part of the messege
	{
		#if DEBUG
			printfs("having to limit the amount I write to the buffer by %d\n", remaining);
		#endif
		length = remaining;
	}
	if(length > casey->sizeReal - casey->tail) // have to coppy the message in chunks
	{
		strncpy(casey->buff+casey->tail, message, (casey->sizeReal - casey->tail));
		message += (casey->sizeReal - casey->tail);  // addvances message
		length = length - (casey->sizeReal - casey->tail);
		casey->tail = 0; // set it up at the start of the buffer
	}

	// now just add the message reguarly to it, wrap around has already occured
	strncpy(casey->buff+casey->tail, message, length);
	casey->tail+=length; // increases the tail position
	if(casey->tail == casey->sizeReal && casey->head != 0) // I am at the very end of the buffer
	{
		casey->tail = 0; // I reset to the very begining
		#if DEBUG
			printfs("I have hit the end of the buffer reverting back had length of %ld\n", length);
		#endif
	}
		// do not reset if head == 0 since that would represent an empty buffer
	#if DEBUG
		// printfs("Add Buffer head: %d tail: %d\n", casey->head, casey->tail);
	#endif
	bufferFlag = 1; // I added to the buffer so I will have to process them latter on
}

/*
 * obtains the full messege from the buffer
 * does not change the head or the tail of the buffer in this process however
 * have multiple palces that do this operation so made a function out of it
 * returns the amount of things I put into the char *line
 * lineSize is the amount of space available in char *line
 * XXX need to check to make sure I am not causing any buff overflow vulnerbilitys
 */
size_t getFullBufferedMessege(cirBuff *casey, char *line)
{
	size_t tempHead = casey->head; // temporary head, will be reset because I need to move head around
	size_t length = 0;
	if(casey->tail < casey->head) // the message wraps around, chunking message
	{
		/*
		if((casey->sizeReal-casey->head)<BUFFSIZE) // yes the messege wraps around
		{*/
			strncpy(line, casey->buff+casey->head, (casey->sizeReal-casey->head));
			length += (casey->sizeReal-casey->head);
			tempHead = 0;
		//}
		// else it does wrap around but I can only grab a strate section in one go
	}
	// now just coppys the rest of the information
	strncpy(line+length, casey->buff+tempHead, (casey->tail - tempHead));
	length += (casey->tail - tempHead);
	return length;
}

/*
 * takes what is currently in the buffer and attempts to send it along its way
 * returns -2 on error in writing to the outFile
 * else returns 0 on sucess
 *
 * will advance the buffer to the appropreate amount after sending the message
 * this method does not write to the logFile or to the window or the loop file
 *
 * this method gets called eaither by sendMessage
 * or by network loop when it is sending the buffer information allong
 *
 * remote table will be used when writing, if it is NULL will do a regular write
 * if it is not NULL then I will call the sendto function
 *
 * recordMode is 0 if not in record mode, else I am in record mode
 * delimiter will be NULL if not in record mode else I am actually in record mode
 *
 * will send a single record at a time
 */ 
int sendBuffer(int fd, cirBuff *casey, fd_set *writeSet, struct sockaddr_in *remoteTable, int recordMode, char *delimiter)
{
	if(!FD_ISSET(fd, writeSet)) // cannot write to this file, it be filled
		return 0; // still tenicall sucess, just did not do anything, no errors
	char *message = (char *) alloca(casey->sizeReal);
	size_t length = getFullBufferedMessege(casey, message);
	ssize_t writeSize;
	size_t trueLength = length;

	if(recordMode) // I am in record mode
	{
		message[length+1] = '\0'; // some of the functions I use rely on null characters
		char *destiny = strstr(message, delimiter);
		if(destiny == NULL) // delimiter was not inside of the buffer can't do anything
		{
			#if DEBUG
				printfs("could not find delimiter in sendBuffer\n");
			#endif
			return 0; // could not attempt a write 
		}
		length = destiny - message + strlen(delimiter); // change the length to reflect the delimiter
		#if DEBUG
			printfs("found delimiter and has length of %ld\n", length);
		#endif
	} 
	#if DEBUG
		else
			printfs("I am not in record mode !!!!\n");
	#endif

	if(!remoteTable) // does not exist so I perform a regular write
		writeSize = write(fd, message, length);
	else
		writeSize = sendto(fd, message, length, 0, (struct sockaddr *) remoteTable, sizeof(*remoteTable)); 

	if(writeSize < 0) // errored to writing to outFile returning -1
	{
		#if DEBUG
			printfs("errored in writing to buffered outFile\n");
		#endif
		return -2;
	}
	if(writeSize != length) // was not able to write everything
	{	
		bufferFlag = 1; // will still need to process buffers
		#if DEBUG
			printfs("was unable to write everything the buffer had, wrote %d out of %d\n", writeSize, length);
		#endif
		if(casey->head+writeSize >= casey->sizeReal) // head will wrap around to the front again
		{
			casey->head = (writeSize - (casey->sizeReal - casey->head)); // XXX needs testing
		}
		else // does not wrap arround
		{
			casey->head+=writeSize; // increased by the amount I wrote to it
		}
	} else if(writeSize == trueLength) // I wrote everything I could, flushing the buffer
	{
		#if DEBUG
			printfs("wrote everything I could flushing buffer in send buffer\n");
		#endif
		flushBuffer(casey); // flushes the buffer
	} else
	{
		#if DEBUG
			printfs("I wrote a messege, still have a partial messege in the buffer\n");
		#endif
		if(casey->head+writeSize >= casey->sizeReal) // head will wrap around to the front again
		{
			casey->head = (writeSize - (casey->sizeReal - casey->head));
		} else // does not wrap arround
		{
			casey->head+=writeSize; // increased by the amount I wrote to it
		}
	}
		//printfs("Send Buffer head: %d tail: %d\n", casey->head, casey->tail);
	#if DEBUG
		printfs("returning from send buffer\n");
	#endif
	FD_CLR(fd, writeSet); // cannot write to that anymore for the time being
	return 0; // was sucessfull
}

/*
 * acts similar to the sendBuff function except instead of sending it to a fd it will return that messege
 * this is so I can call sendInDirection and such and not re-write everything
 * a partial write on the external is fine because it will go into the comunal buffer
 * returns the size of the messege being writien
 * char *messege is of size casey->sizeReal and is were the messege will be stored
 * XXX make sure I do not have buffer overflow vunerbility 
 * 
 * updates the head and the tail of casey to reflect the messege the has been removed
 * returns zero when I do not have a messege available
 */ 
size_t getRecordMessege(cirBuff *casey, char *delimiter, char *message)
{
	size_t length = getFullBufferedMessege(casey, message);
	size_t trueLength = length; // to test if there was any differences
	
	message[length+1] = '\0'; // some of the functions I use rely on null characters
	char *destiny = strstr(message, delimiter);
	
	if(destiny == NULL) // delimiter was not inside of the buffer can't do anything
	{
		return 0; // could not attempt a write, so the length is zero means I 
	}
	length = destiny - message + strlen(delimiter); // change the length to reflect the delimiter
	
	if(length==trueLength) // I grabed the entire messege
	{
		#if DEBUG
			printfs("grabed full messege from buffer\n");
		#endif
		flushBuffer(casey);
	} 
	// need to update the head and tail positions
	else if(casey->head+length >= casey->sizeReal) // head will wrap around to the front again
	{
		casey->head = (length - (casey->sizeReal - casey->head));
		bufferFlag = 1;
	} else // does not wrap arround
	{
		casey->head+=length; // increased by the amount I wrote to it
		bufferFlag = 1;
	}
	return length;
}

/*
 * helper function that does the internal filtering
 * puts the message in the result buffer but filltered by the filter enume
 * returns the new length of the string
 */ 
size_t internalFilter(char *result, char *message, size_t length, enum STRIP filter) 
{
	size_t i, place;
	for(i=0, place=0; i<length; i++)
	{
		if(message[i] >= 32 && message[i] <= 126) // printable character
		{
			result[place] = message[i];
			place++;
		}
		else if(filter==STRIPSOME && (message[i]=='\n' || message[i] == '\r'))
		{ // I am in stripsome mode and it was a new line or a charage return 
			result[place] = message[i];
			place++;
		}
	}
	result[place] = '\0'; // null characters saves lives
	return place; // this is the new length of the string now
}

/*
 * another helper function that will be called if the log file is not -1
 * will attempt to write to the log file
 * if any errors occur will take care of closing the log file and such
 */ 
void writeToLogFile(char *message, size_t length, int *fd, fdInfo *fdList, fd_set *writeSet)
{
	if(FD_ISSET(*fd, writeSet)) // have space to write
	{
		ssize_t check = write(*fd, message, length);
		if(check<=0)
		{
			#if DEBUG
				printfs("writing to log file failed\n"); // honestly should never happen
			#endif
			// TODO remove log file, incoperate remove log file function
		}
	}
	else
		printfs("cannot to the log file, not enough space\n"); // should never happen
}

/*
 * used to process the out directions buffers
 * even though there might be stuff in the buffer will never actually access it
 */
void processOutDirectionBuffers(sideInfo *sid, fdInfo *fdList, fd_set *writeSet)
{
	int i;
	for(i=0; i<sid->listSize; i++) // go through the entier list
	{
		sockInfo *path = sid->socketList[i];
		if(path!=NULL && path->socketFd>0 && hasMessage(&path->outBuffer)) // I can do stuffs with it
		{
			if(!FD_ISSET(path->socketFd, writeSet)) // cannot write to it, but still need to check another time
				bufferFlag = 1;
			else
			{
				int check = sendBuffer(path->socketFd, &path->outBuffer, writeSet, &path->remoteTable, 0, NULL);
				if(check<0)
				{
					removeSocketInfo(sid, i, fdList);
					#if DEBUG
						printfs("working of buffers and got an error who knew\n");
						printfs("also getting really tiered of writing unique print statments for this type of error\n");
					#endif
				}
			}
		}
	}
}

/*
 * will send the message in the out direction
 * will return 0 if it had to update any buffers
 * else will return 1
 * will also loop if possible
 *
 * will print to the screen before anything
 */ 
int sendOutDirection(char *message, size_t length, directionInfo *dir, fdInfo *fdList, fd_set *writeSet)
{
	int check = 0;
	int result = 1; // nothing bad happend, could be updated to zero to represent buffer changed
	int i;
	printMessage(dir->outWindow, message, length); // XXX might want to set up the same logic of if I have an out write
	if(dir->writingFileList[dir->outWindow-1]>0) // got a log
	{
		writeToLogFile(message, length, &dir->writingFileList[dir->outWindow-1], fdList, writeSet);
	}
	sideInfo *sid = dir->outSide; // temp variable for the looping, so my code does not take up entire screen
	for(i=0; i<sid->listSize; i++) // iterates through all the sockets
	{
		sockInfo *path = sid->socketList[i];
		if(path!=NULL && path->socketFd>0) // I have a connection
		{
			if(hasMessage(&path->outBuffer)) // already message in the buffer
			{
				addToBuffer(&path->outBuffer, message, length);
				check = sendBuffer(path->socketFd, &path->outBuffer, writeSet, &path->remoteTable, 0, NULL);
				if(check<0) // writing failed time to close the socket
				{
					removeSocketInfo(sid, i, fdList); // should close and free the memory
				}
				result = 0; // change of the buffer
			} else if(FD_ISSET(path->socketFd, writeSet)) // has space to write
			{
				ssize_t writeSize = 0;
				if(path->protocolOption==O_UNIX) // cannot use remoteTable since unix domain socket
					writeSize = write(path->socketFd, message, length);
				else 
					writeSize = sendto(path->socketFd, message, length, 0, (struct sockaddr *)&path->remoteTable, 
						sizeof(path->remoteTable));
				#if DEBUG
					printfs("I am sending the information\n");
				#endif
				if(writeSize < 0) // error occured when writing
				{
					removeSocketInfo(sid, i, fdList);
					#if DEBUG
						printfs("writing failed because of %s\n", strerror(errno));
						/*if(path->remoteTable==NULL)
							printfs("the remote table was null\n");
						else
							printfs("the remote table was NOT null\n");*/
					#endif
					result = 0; // O ya a buffere deffenetly changed as in go blown up
				} else if(writeSize < length) // writing did occur but now all of it
				{
					addToBuffer(&path->outBuffer, message+writeSize, length-writeSize);
					result = 0;
				}
				// has somehow was able to write everything
			} else // no room in the file so I will now buffer
			{
				addToBuffer(&path->outBuffer, message, length);
				result = 0;
			}
		}
		// else it does not exist so I'm just going to skip over it
	}
	// looped through all the sockets now time to attempt to loop
	if(*dir->looping == dir->loopComp) // I am actually looping
	{
		check = sendInDirection(message, length, dir->loopDir, fdList, writeSet);
			// will handle closing of any bad connection or stuff
		if(check!=0) 
			result = 1; // it had to update the buffer so I will have to update my buffer as well
	}
	return result; // done writing
}

/*
 * function that will send the messege for the in direction
 * eaither to the pipe for an external filter
 * or to send out direction function
 * this is were the internal filtering will take place
 * but before the filtering will print it to the screen
 *
 * returns 0 if it had to update any buffers
 * else it returns 1
 * will handle any destroying of the external filters if an error occurs
 */ 
int sendInDirection(char *message, size_t length, directionInfo *dir, fdInfo *fdList, fd_set *writeSet)
{
	int check = 0; // temp variable when I am writing and stuff
	char *line;
	
	printMessage(dir->inWindow, message, length); // prints the message to the screen

	if(dir->writingFileList[dir->inWindow-1] > 0) // I have a logging file to write to
	{
		writeToLogFile(message, length, &dir->writingFileList[dir->inWindow-1], fdList, writeSet); // will write to log file
	} if(dir->filterIn!=STRIPNO) { // needs filtering
		char *tempBuff = alloca(length+2);
		line = tempBuff;
		length = internalFilter(line, message, length, dir->filterIn);
	} else
	{
		line = message; // set the pointer of message into the pointer of line
	}
	
	// this point forward the message is now the line, don't have to worry if I have filtered or not
	if(dir->external<=0) // I have no external filter sending it to the next function
	{
		#if DEBUG
			printfs("I have no external time to send it to the next phase\n");
		#endif
		return sendOutDirection(line, length, dir, fdList, writeSet); // sends message to the out direction
	}
	/*
	// else time to send to an external filter guys
	ssize_t writeSize = write(dir->windowLink[1], line, length);
	if(writeSize < 0) // so long pall it was a fun ride but now your dead
	{
		killChild(dir, fdList);
		return 0;
	}
	else if(writeSize<length) {
		#if DEBUG
			printfs("partial write occured ****\n");
		#endif
	}
	*/
	if(hasMessage(&dir->inBuffer)) // I have a message in the buffer need to write to it and then send it
	{
		addToBuffer(&dir->inBuffer, line, length);
		check = sendBuffer(dir->windowLink[1], &dir->inBuffer, writeSet, NULL, dir->inRecordMode, dir->delimiter);
		if(check<0)
		{
			killChild(dir, fdList); // time to murder me a child process
			// since child process died will flush the buffer
			flushBuffer(&dir->inBuffer);
			updateExternalFilter(dir->inWindow, NULL, 0);
		}
		return 0; // changed the buffer
	} else if(FD_ISSET(dir->windowLink[1], writeSet)) // have space to write
	{
		#if DEBUG
			printfs("non buffered write to external\n");
		#endif
		ssize_t writeSize = write(dir->windowLink[1], line, length);
		if(writeSize < 0) // time to kill another child
		{
			killChild(dir, fdList);
			updateExternalFilter(dir->inWindow, NULL, 0);
			flushBuffer(&dir->inBuffer);
			return 0;
		} else if(writeSize < length) // did not get the opertunity to write everything
		{
			#if DEBUG
				printfs("could not write everything adding to buffer\n");
				if(dir->inRecordMode)
					printfs("********** ya got a partial write\n");
			#endif
			addToBuffer(&dir->inBuffer, line+writeSize, length-writeSize); // should not happen when in record mode
			return 0;
		}
	} else // no room in the file
	{
		#if DEBUG
			printfs("no room in external filter adding to buffer\n");
		#endif
		addToBuffer(&dir->inBuffer, line, length);
		return 0;
	}	
	return 1; // somehow did not change the buffer... this actually usualy happens
}

/*
 * used when I am in record mode to do internal buffering
 * if no delimiter was found will write it to the buffer
 * returns 0 if there was a change to the buffer
 * else will return 1
 * XXX could be more effitent an reduce number of memory writes to the buffer and stuff
 */ 
int processRecord(directionInfo *dir, sockInfo *path, fdInfo *fdList, fd_set *readSet, fd_set *writeSet)
{
	int result = 0;
	if(FD_ISSET(path->socketFd, readSet) && getBufferSpace(&path->inBuffer)>=BUFFSIZE)
	{
		#if DEBUG
			printfs("I am processing the record\n");
		#endif
		socklen_t dummy = sizeof(path->remoteTable);
		char line[BUFFSIZE];
		ssize_t amountRead = 0;
		if(path->protocolOption == O_UNIX)
		{
			amountRead = read(path->socketFd, line, BUFFSIZE);
		} else
		{
			amountRead = recvfrom(path->socketFd, line, BUFFSIZE, 0, (struct sockaddr *) &path->remoteTable, &dummy);
		}
		if(amountRead <=0) // socket error
		{
			removeSocketInfo(dir->inSide, getIndexOfPath(dir->inSide, path), fdList);
			#if DEBUG
				printfs("erroed in reading in processRecord\n");
			#endif
		} else // writes the messege to the buffer
		{
			addToBuffer(&path->inBuffer, line, amountRead);
		}
	} if(hasMessage(&path->inBuffer)) // first attempts to process the buffer before any messeges
	{
		char *record = alloca(path->inBuffer.sizeReal);
		size_t length = getRecordMessege(&path->inBuffer, dir->delimiter, record);
		if(length > 0) // I have a full record and I can send it
		{
			#if DEBUG
				printfs("currently recod mode set to %d\n", dir->inRecordMode);
			#endif
			result |= !sendInDirection(record, length, dir, fdList, writeSet);
		}
	}
	return !result;
}

/*
 * attempts to read the direction
 * does not do any of the iternal filtering, that will be handled in the latter functions
 * will remove the socket from the list if it could not read from it
 *
 * being passed in the temp read and temp write sets
 */
void readDirection(directionInfo *dir, fdInfo *fdList, fd_set *readSet, fd_set *writeSet)
{
	if(dir->inSide->connectionCount==0) // no need to check for stuff
		return;

	if(dir->external > 0 && hasMessage(&dir->inBuffer)) // attempts to move the messege first in the buffer
	{
		if(sendBuffer(dir->windowLink[1], &dir->inBuffer, writeSet, NULL, dir->inRecordMode, dir->delimiter)) // failed to write the buffer
		{
			killChild(dir, fdList);
			flushBuffer(&dir->inBuffer);
		}
	}
	char line[BUFFSIZE+1]; // temporary string were I store what I am writing
	int hasSpace = canReadInDirection(dir);
	int i;
	sideInfo *sid = dir->inSide; // just a temp variable so I don't have to writve massive chanes of structs
	for(i=0; i<sid->listSize && hasSpace; i++) // iterates through the socketList as long as I can write to a buffer
	{
		if(sid->socketList[i]!=NULL && sid->socketList[i]->socketFd>0)
		{ // I have a socket that has something that I want to read
			if(dir->inRecordMode)
			{
				if(!processRecord(dir, sid->socketList[i], fdList, readSet, writeSet)) // will call read and stuff and process buffer
					hasSpace = canReadInDirection(dir); // buffer changed seeing if I can continue
			}
			else if(FD_ISSET(sid->socketList[i]->socketFd, readSet))
			{
				socklen_t dummy = sizeof(sid->socketList[i]->remoteTable); // need to pass in pointer
				ssize_t amountRead = 0;
				if(sid->socketList[i]->protocolOption == O_UNIX) // unix sockets don't work the same as tcp or others
				{
					amountRead = read(sid->socketList[i]->socketFd, line, BUFFSIZE);
				} else
				{
					amountRead = recvfrom(sid->socketList[i]->socketFd, line, BUFFSIZE, 0, 
							(struct sockaddr *) &sid->socketList[i]->remoteTable, &dummy);
				}
				if(amountRead<=0) // there was an error in reading need to remove the file descriptor
				{
					#if DEBUG
						printfs("errored in reading socket Info removing\n");
						printfs("errno is %s\n", strerror(errno));
					#endif
					removeSocketInfo(sid, i, fdList);
					// don't have to wory about the temp fdSets since I removed the file descriptor from memory
				} else if(!sendInDirection(line, amountRead, dir, fdList, writeSet))
				{ // sends the messege along, if there was a change in the buffers I need to see if I still have space
					hasSpace = canReadInDirection(dir);
					#if DEBUG
						printfs("the buffer has been updated\n");
					#endif
				}
				#if DEBUG 
					printfs("sent the in direction\n");
				#endif
			}
		}
	}
	// all done
}

/*
 * function that will first check if I have an external filter to read from
 * then will atempt to read from it if anything was set
 * will then send to the outDirection function
 * if anything bad happens will take care of it in the function
 */ 
void readExternal(directionInfo *dir, fdInfo *fdList, fd_set *readSet, fd_set *writeSet)
{
	if(dir->external>0 && FD_ISSET(dir->windowLink[0], readSet) && canReadOutDirection(dir)) 
	{ // I have an external process and I can read from it
		char message[BUFFSIZE+1];
		ssize_t readSize = read(dir->windowLink[0], message, BUFFSIZE); // reads the message
		if(readSize<=0) // well an error occured killing child
		{
			killChild(dir, fdList);
			updateExternalFilter(dir->inWindow, NULL, 0);
		} else // send the message along its way
		{
			sendOutDirection(message, readSize, dir, fdList, writeSet);
		}
	}
}

/*
 * call the socket function and returns the socket descriptor
 * does not bind the socket to anything or attempt to connect or accept
 * uses PF_INET, SOCK_STREAM, and tcp protocal when creating the socket
 *
 * checks if socket is a valid socket before returning it
 *
 * returns socket descriptor
 * if error occurs will return -1 instead
 *
 * sets the socket options to SO_REUSEADDR as well
 */ 
int createSocket(int optionProtocol, int ipType)
{
	int domain, type, protocol;
	convertSocketOptions(optionProtocol, ipType, &domain, &type, &protocol); // gets the required options

	int sock = socket(domain, type, protocol); // creates the socket and returns
	if(sock < 0) // could not create the socket
	{
		#if DEBUG
			printfs("socket creation failed\n");
			printfs("errno: %s\n", strerror(errno));
		#endif
		return -1;
		//fprintf(stderr, "socket creation failed\n");
		//exitCurses(EXIT_FAILURE);
	}
	// setting socket options
	int flag = 1; // dummy variable
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0) // fatal error as occured
	{
		fprintf(stderr, "set socket options failed\n");
		exitCurses(EXIT_FAILURE);
	}

	// TODO add setsocketopt for igmp messeges

	/*
	#if DEBUG // going to attempt to call watermark on the socket
		printfs("attmpting to set the watermark for the socket\n");
		int bufferingAmount = BUFFSIZE; // only write if I have buffsize
		int result = setsockopt(sock, SOL_SOCKET, SO_SNDLOWAT, &bufferingAmount, sizeof(int));

		if(result < 0)
		{
			printfs("watermarked errored with errno %s\n", strerror(errno));
		} else
		{
			printfs("huston we have liftoff\n");
		}
	#endif
	*/
	return sock;
}

/*
 * will call listen of the socket with the constant QLEN
 * sticking to the phelosophy of not re-writing code in multiple places
 * since I must wait to listen somtimes I created this method
 * returns 0 on sucess -1 on error
 */
int listenSocket(int socket)
{
	if(listen(socket, QLEN) < 0) // listen failed, fatal error
	{
		#if DEBUG
			printfs("listen failed\n");
		#endif
		return -1;
	}
	return 0;
}

/*
 * creates and connects a socket to the givent targetAddress and the targetPort
 * this method will call htons to flip the bits for the target Port
 * will return -1 if connection failed, will be handdled by other methodes
 *
 * local port is a port numberthat is not previously been bound
 * if it is -1 will use any port number
 *
 * returns the fileDiscriptor of the socket, negative on error
 * returns -1 if binding failes
 * returns -2 if hostname is invalid
 * returns -3 if connection fails
 *
 */ 
int connectSocket(sockInfo *path)
{	
	int targetSocket = bindLocalSocket(path->localPort, path->protocolOption, path->ipType, NULL); 
		// obtains a generic socket
		// if localPort is negative  will get a random port number
	if(targetSocket < 0) // binding failed or creating the socket failed
	{
		#if DEBUG
			printfs("creating socket failed\n");
			printfs("errno %s\n", strerror(errno));
		#endif
		return -1; 
	}
	int domain, type, protocol;
	convertSocketOptions(path->protocolOption, path->ipType, &domain, &type, &protocol); // grabs what I need

	if(path->protocolOption != O_UNIX)
	{
		struct hostent *host;

		if(path->remotePort == -1) // will use default value instead
		{
			#if DEBUG
				printfs("using default value for target port %d\n", DEFAULTPORT);
			#endif
			path->remotePort = DEFAULTPORT;
		}

		memset((char *) &path->remoteTable, 0, sizeof(struct sockaddr_in)); // cleans memory

		// sets family and IP adress and port
		path->remoteTable.sin_family = domain; // XXX needs checking if this is legal
		path->remoteTable.sin_port = htons((u_short) path->remotePort);
		
		host = gethostbyname(path->remoteAddress);

		if(((char *) host) == NULL) // not a valid host, fatal error
		{
			#if DEBUG
				printfs("%s is an invalid host name\n", path->remoteAddress);
			#endif
			return -2;
		}
		memcpy(&path->remoteTable.sin_addr, host->h_addr, host->h_length); // coppys the host's address to targetTable
		#if DEBUG
			printfs("attempting to connect to %s on port %d\n", path->remoteAddress, path->remotePort);
		#endif
		// attempt to make a connection
		if(!isConnectionless(path->protocolOption)) // I need to call connection since I am a connection service
		{
			if(connect(targetSocket, (struct sockaddr *) &path->remoteTable, sizeof(struct sockaddr_in)) < 0) // failed to connect
			{
				#if DEBUG
					printfs("connection failed ");
					printfs("%s\n", strerror(errno)); // obtain why the connection failed
				#endif
				// won't print error messeges in this method, only return -3;
				return -3; // reporting error to the above method
			}
		}
		return targetSocket;
	} else // unix domain socket time
	{
		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, path->remoteAddress, strlen(path->remoteAddress));
		addr.sun_path[strlen(path->remoteAddress)] ='\0';
		if(connect(targetSocket, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		{
			// errored when connect
			#if DEBUG
				printfs("unix connection failed\n");
				printfs("errno: %s\n", strerror(errno));
			#endif
				return -3;
		}
		return targetSocket; //success
	}
}

/*
 * creates a socket that will be used for accepting socket connections
 * will bind to any local IP addres at the given local port
 * 
 * does not call listen on this socket
 * this is to add future functionality when I want to set the local right port number
 *
 * pass in -1 for localPort so that it will give my any port it wants
 * this port number cannot be accesed with the table used when binding
 * but calling getsockname one can determine the prot number
 *
 * return the localSocket that has been bound
 * if error occured returns negative number
 * 
 * localAddress is used for binding the listening sockets
 * if NULL will use INADDR_ANY
 * else attempts to bind the socket to the localAddress
 * if the localAddress is NULL and we have a unix domain socket it is a connecting unix domain socket
 * I cannot bind on that address since it will already be in use
 *
 * if -1 is given, just gets an unbound local socket
 * TODO need to change if I am a unix domain socket
 */
int bindLocalSocket(int localPort, int protocolOption, int ipType, char *localAddress)
{
	int localSocket = createSocket(protocolOption, ipType);
	#if DEBUG
		printfs("creating the local socket %d\n", localSocket);
	#endif
	int domain, type, protocol;
	convertSocketOptions(protocolOption, ipType, &domain, &type, &protocol);

	if(protocolOption!=O_UNIX /*|| localAddress==NULL*/) // its not a domain socket
	{
		struct sockaddr_in localTable;
		if(localPort < 0) // since -1 don't need to call bind becuase I want a random port
			return localSocket;

		// clearing memory and setting options
		memset((char *) &localTable, 0, sizeof(struct sockaddr_in)); // clearing memory
		localTable.sin_family = domain;
		localTable.sin_addr.s_addr = INADDR_ANY;

		/*
		if(localAddress==NULL) // use any old address
			localTable.sin_addr.s_addr = INADDR_ANY; // set up local ip address to any
		else
		{
			// localTable.sin_addr.s_addr = localAddress; // TODO need to fix this, not right now TODO
		}
		*/
		if(localPort > 0) // if negative will let os give me a number
		{
			localTable.sin_port = htons((u_short) localPort); // flips and sets the local port
		}
		// binds the socket
		if(bind(localSocket, (struct sockaddr *) &localTable, sizeof(struct sockaddr_in)) < 0) // bind failed, fatal error
		{
			return -1; // bind failed
		}
		// does not call listen
		return localSocket;
	} else // it is a domain socket that will attempt to be listening, if address is NULL connecting
	{
		int size;
		struct sockaddr_un unDomain;
		unDomain.sun_family = AF_UNIX;
		if(localAddress != NULL) // going to bind it to this address else it will get a random one
		{
			strncpy(unDomain.sun_path, localAddress, strlen(localAddress));
			unDomain.sun_path[strlen(localAddress)] = '\0';
		}
		size = offsetof(struct sockaddr_un, sun_path)+strlen(unDomain.sun_path);
		if(bind(localSocket, (struct sockaddr *) &unDomain, size) < 0)
		{
			#if DEBUG
				printfs("bind failed for unix domain socket\n");
				printfs("errno: %s\n", strerror(errno));
			#endif
			return -1; // failed
		}
		if(localAddress != NULL)
		{
			chmod(localAddress, S_IRWXU); // rwx for user only
		}
		return localSocket; // sucess
	}
}

/*
 * will accept connection to this socket that is being sent to the localsocket
 * if accept fails will be handled by the above method, depends on error
 *
 * will not check for correct senderaddress and senderport, will be handled in calling methods
 *
 * returns the connecting socket file discriptor, -1 on error
 */ 
int acceptSocket(int localSocket)
{
	struct sockaddr_in senderTable;
	int senderSocket = -1; // starting out with dummy variables
	
	memset((char *) &senderTable, 0, sizeof(struct sockaddr_in)); // scrubbing memory

	// calling accept
	unsigned senderAddressLength = sizeof(struct sockaddr_in); // used for accept method call
	if((senderSocket = accept(localSocket, (struct sockaddr *) &senderTable, &senderAddressLength)) < 0) // accept failed
	{
		#if DEBUG
			printfs("accept failed: %s\n", strerror(errno));
		#endif
		return -1;
	}
	return senderSocket;
}

/*
 * returns the string representation of the name of that specific socketaddr
 * done by calling gethostbyaddr
 * used multiple times thorughout program so made a method to do it
 *
 * assums that the sockaddr_in table is a valid table and thus
 * gethostbyaddr will never error 
 *
 * returns the string of the addresses name
 */ 
char *getNameFromAddress(struct sockaddr_in *table)
{
	// obtains the host by the sin_addr so I can get its name as a string
	struct hostent *host = gethostbyaddr((void *) &table->sin_addr, 
			sizeof(table->sin_addr), AF_INET);

	return host->h_name; // belive that string is stored some place safe
}

/*
 * will return the name of the address connection from the socket file discriptor
 * simply obtains the address table from the socket and then gets the name
 *
 * returns the string representation of the target address from this socket
 */
char *getNameFromSocket(int targetSocket)
{
	struct sockaddr_in targetTable;
	socklen_t size = sizeof(struct sockaddr_in); // passed into getsockname method
	memset((char *) &targetTable, 0, sizeof(targetTable));
	
	getpeername(targetSocket, (struct sockaddr *) &targetTable, &size); // fills the table
	
	return getNameFromAddress(&targetTable);
}

/*
 * simple method that you give a socket file discriptor
 * and it returns to you a string that is the IP address
 * in dot decimal notation
 * returns null if getPeerName fails
 * returns empty string if inet_ntoa fails
 */
char *getIPFromSocket(int targetSocket)
{
	socklen_t size = sizeof(struct sockaddr_in);
	struct sockaddr_in targetTable;
	memset((char *) &targetTable, 0, size); // clears memory
	int check = getpeername(targetSocket, (struct sockaddr *) &targetTable, &size); // remote address
	if(check < 0)
	{
		perror("gett peer (remote) name failed: ");
		return NULL;
	}
	return inet_ntoa(targetTable.sin_addr);
}

/*
 * used when accepting connections from the left
 * will test to see if the desiered connection is the one that I am expecting
 * 
 * pigInfo->recLeft must be set before this method is ever called
 *
 * will handle closing socket and freeing memory if there was a connection error
 *
 * return 0 if connection is corret, else returns -1
 * TODO does not actually close the connetion if it is invalid, leaves it open wich is bad
 */
void hasCorrectConnection(sideInfo *sid, int remoteSocket, fdInfo *fdList)
{		
	int found = 0; // temp varable telling me if I found anything that was not null and did not have a connection
	int i;
	struct sockaddr_in remoteTable; // setting up structs to find the address and port of the connecting socket
	memset((char *) &remoteTable, 0, sizeof(remoteTable));
	socklen_t size = sizeof(remoteTable);
	getpeername(remoteSocket, (struct sockaddr *) &remoteTable, &size); // obtains the table for this socket
	
	char *remoteAddress = inet_ntoa(remoteTable.sin_addr); // IP address XXX does not work with IPv6
	char *remoteName = getNameFromAddress(&remoteTable); // network address like linux-11.cs.wwu.edu
	int remotePort = htons((u_short) remoteTable.sin_port); // the remote port of the connecting socket

	for(i=0; i<sid->listSize; i++)
	{
		if(sid->socketList[i]!=NULL && sid->socketList[i]->socketFd<0) // could accept the connection
		{
			found = 1; // know that I am filtering for different connections
			int result = 1; // boolean telling me if the connection is valid
			
			if(sid->socketList[i]->remoteAddress!=NULL)
			{
				char *nameCheck = gethostbyname(sid->socketList[i]->remoteAddress)->h_name;
				if(strcmp(remoteAddress, nameCheck) && strcmp(remoteName, nameCheck))
				{
					result = 0; // did not match
					#if DEBUG
						printfs("failed connection ot right adress name\n");
					#endif
				}
			} if(sid->socketList[i]->remotePort > 0) // check if port matches
			{
				#if DEBUG
					printfs("connecting port number %d\n", remotePort);
				#endif
				if(remotePort != sid->socketList[i]->remotePort) // wrong port number
				{
					result = 0;
					#if DEBUG
						printfs("failed connection, not right port number\n");
					#endif
				}
			} if(result) // gentilmen we have ourselves a connection
			{
				sid->socketList[i]->socketFd = remoteSocket;
				addToFileSet(remoteSocket, NULL, fdList); // adds it to the set to be read and stuff
				// now to update the information on that socket
				sid->socketList[i]->remoteAddress = addAddress(sid->socketList[i]->remoteAddress, 
						remoteAddress);
				sid->socketList[i]->remotePort = remotePort; // stores the values to latter be printed
				
				return; // all done no need to stay in this function
			}
			// else it did not make the cut so we keep looking
		}
	} // end of for loop
	if(!found) // turns out I was never filtering for any connections so I guess I will just accept it
	{
		addToFileSet(remoteSocket, NULL, fdList);
		// TODO update the socke information like local port and stuff
		sockInfo *path = createSocketInformation(remoteSocket, -1, remotePort, remoteAddress, sid->listenProtocol,
				sid->listenIpType, NULL, BUFFSIZE*2);
		addToSocketList(sid, path); 
	} else // turns out I had options set but none of them worked so ya
	{
		close(remoteSocket); // this should hang up on the other side
	}

	/*
	if(remoteSocket < 0) // accept failed returning -1 will make me try again
	{
		return -1;
	}

	// need to first obtain the socket information table
	struct sockaddr_in remoteTable;
	memset((char *) &remoteTable, 0, sizeof(remoteTable));
	
	socklen_t size = sizeof(remoteTable);
	getpeername(remoteSocket, (struct sockaddr *) &remoteTable, &size); // obtains the table for this socket

	int result = 0; // assum starting out correct

	if(raddr != NULL) // connection must match lraddr
	{
		char *remoteAddress = inet_ntoa(remoteTable.sin_addr);
		char *remoteName = getNameFromAddress(&remoteTable);

		char *nameCheck = gethostbyname(raddr)->h_name;

		#if DEBUG
			printfs("remoteName %s : remoteAddress %s, lraddr %s\n", remoteName, remoteAddress, raddr);
		#endif
		if(strcmp(remoteAddress, nameCheck) && strcmp(remoteName, nameCheck)) // does not match
		{
			#if DEBUG
				printfs("failed connection not right address name\n");
			#endif
			result = -1;
		}
	}
	#if DEBUG
		printfs("connecting port number %d\n", htons((u_short) remoteTable.sin_port));
	#endif
	if(remotePort > 0) // check if ports match
	{
		if(remotePort != htons((u_short) remoteTable.sin_port)) // wrong port number
		{
			#if DEBUG
				printfs("failed connection, not right port number\n");
			#endif
			result = -1;
		}
	}
	if(result) // connection was a faild, closing socket and freeing memory
	{
		#if DEBUG
			printfs("closeing file discriptor\n");
		#endif
		close(remoteSocket); // closing socket, not the correct one
	}
	return result;
	*/
}

/*
 * connects the given path the the localPort, remotePort and to the address
 *
 * returns 0 on sucess
 * -1 if localPort Bind Failed
 * -2 if address is not a valid hostName
 * -3 if connection failed
 * -4 if already an inFile connection on that path / exceed max connection limit
 *
 *  does not change the noDirection information in pigInfo
 *
 *  DOES NOT add the sockInfo to the list of connections on that side
 *
 *  if error does occur will free sockInfo
 */
int connectTo(fdInfo *fdList, sockInfo *path)
{
	int tempFile = connectSocket(path);
	if(tempFile < 0) // error occured in connection, return value represents the error
	{
		if(tempFile==-1)
			printfs("Binding failed for %d\n", path->localPort);
		else if(tempFile==-2)
			printfs("%s is not a valid host name\n", path->remoteAddress);
		else if(tempFile==-3)
			printfs("Connection failed\n");
		freeSocketInformation(path);
		return tempFile;
	}
	path->socketFd = tempFile;
	addToFileSet(tempFile, NULL, fdList); // adds it both the the read and the write set
	// XXX implementation to update screen information, don't think makeConnection will live to piggy5.0 

	return 0; // success
}



/*
 * used to initalize accepting connections also known as listening
 * will print error messages if error occured
 * returns 0 on sucess negative on faileure
 *
 * DOES NOT check if I am already listening in that direction
 *
 * XXX need to do something else for binding listening sockets
 * due to if it was a domain then it matters
 */
int acceptTo(fdInfo *fdList, sideInfo *sid)
{
	if(sid->listenPort < 0) // I should be using the defualt port
		sid->listenPort = DEFAULTPORT;

	if(sid->listenProtocol == O_UNIX && sid->localAddress==NULL) // domain but I don't have address time to make one
	{
		//sid->localAddress = malloc(PATH_MAX+1); // gets a string that can fit the path
		//sid->localAddress[0] = '\0'; // so that strlen will return 0
		// will coppy that path into this string
		
		char placeHolder[] = "/tmp/netHog-XXXXXX\0";
		sid->localAddress = malloc(sizeof(char)*strlen(placeHolder+1));
		strncpy(sid->localAddress, placeHolder, strlen(placeHolder));
		sid->localAddress[strlen(placeHolder)] = '\0';
		sid->localAddress = mktemp(sid->localAddress); // XXX potential security hole
		
		/*
		sid->localAddress = malloc(PATH_MAX+1);
		char *tempPath = tmpnam(NULL);
		sid->localAddress = strncpy(sid->localAddress, tempPath, strlen(tempPath));
		*/
	}

	sid->listenSocket = bindLocalSocket(sid->listenPort, sid->listenProtocol, sid->listenIpType, sid->localAddress);
	
	if(sid->listenSocket < 0) // bind Failed
	{
		printfs("Bind failed on local port %d\n", sid->listenPort);
		return -1;
	}
	// else
	listenSocket(sid->listenSocket);
	addToFileSet(sid->listenSocket, &fdList->masterReadSet, fdList);
	updateBorderSideInfo(sid);
	#if DEBUG
		printfs("should have updated border\n");
	#endif

	return 0; // all done
}

/*
 * method that will set a new maximum in the FileList
 */
void setMaxFile(fdInfo *fdList)
{
	int i;
	for(i=fdList->maxFileDescriptor-1; i>=3; i++) // runs thorugh the file lists checking if it is set
	{
		if(FD_ISSET(i, &fdList->masterReadSet) || FD_ISSET(i, &fdList->masterWriteSet)) // it is set time to save it
		{
			fdList->maxFileDescriptor = i;
			break;
		}
	}
	if(i==2) // nothing was set, time to set it back to the original value
	{
		fdList->maxFileDescriptor = 3; // sets it back to the default of 3
	}
}

/*
 * will simply add the file discriptor to the given set
 * if fileDescriptor is -1, will not add it to the set
 *
 * if the fileSet passed in is NULL will add it to both the read and the write fileSets
 *
 * if fileDiscriptor you are adding is the maximum, will set the new maximum 
 */ 
void addToFileSet(int fileDescriptor, fd_set *fileSet, fdInfo *fdList)
{
	if(fileDescriptor<0) // if -1 cannot add to the fileSet
		return;
	
	if(fileSet != NULL)
		FD_SET(fileDescriptor, fileSet);
	else
	{
		FD_SET(fileDescriptor, &fdList->masterReadSet);
		FD_SET(fileDescriptor, &fdList->masterWriteSet);
	}
	if(fileDescriptor > fdList->maxFileDescriptor)
		fdList->maxFileDescriptor = fileDescriptor;
}

/* 
 * will remove the file descriptor from eather the readSetMaster, or the writeSetMaster, or both
 *
 * will check if file descripter was maximum, if it was will set new maximum file descriptor
 *
 * this function will not however remove the node in the linked list
 * or reset the writingFileList
 *
 * returns 0 on sucess, 1 if it was not in the list
 */ 
int clearFileSet(int fileDiscriptor, fdInfo *fileList)
{
	int result = 0; // what I will be returning 
	if(FD_ISSET(fileDiscriptor, &fileList->masterReadSet)) // clear from readSet
	{
		FD_CLR(fileDiscriptor, &fileList->masterReadSet);
		result++;
	}
	if(FD_ISSET(fileDiscriptor, &fileList->masterWriteSet)) // clear from writeSet
	{
		FD_CLR(fileDiscriptor, &fileList->masterWriteSet);
		result++;
	}
	
	if(fileDiscriptor==fileList->maxFileDescriptor && result) // if I removes something, then I can change
		setMaxFile(fileList);
	
	return !result; // if I found anything would be > 1, thus 0 for sucess
}

/*
 * reads the string and obtains the port number
 * will print error messages and return negative if not valid port number
 * or it is not in range
 * returns port on sucess
 */ 
int getPort(char *portus)
{
	int port = atoi(portus);

	if(port == 0)
	{
		printfs("%s is not a valid port number\n");
		return -1;
	}
	if(port < 1 || port > 65536)
	{
		printfs("port %d is out of range\n");
		return -1;
	}
	return port;
}

/*
 * called to update the side informations in the border
 */
void updateBorderSideInfo(sideInfo *sid)
{
	if(sid->listenSocket > 0) // I am listening
	{
		if(sid->listenProtocol==O_UNIX) // unix connection going to just pass it my path
		{
			updateConnections(sid->localAddress, sid->inWindow, sid->connectionCount);
		}else // need to pass a string representation of the port
		{
			char numBuff[20]; // should be big enough
			sprintf(numBuff, "%d", sid->listenPort);
			updateConnections(numBuff, sid->inWindow, sid->connectionCount);
		}
	} else // am not listening just pass null
	{
		updateConnections(NULL, sid->inWindow, sid->connectionCount);
	}
}

/*
 * will print the side info
 * TODO wnat to make this more fancy and formated table output
 */ 
void printSideInfo(sideInfo *sid)
{
	// first print listening socket if any
	if(sid->listenSocket>0) // I'm listening
	{
		if(sid->listenProtocol == O_UNIX) // I have a domain socket printing that out
		{
			printfs("Unix domain socket listening on path %s\n", sid->localAddress);
		} else // print out info like a regular socket, TODO can get fancy on the socket type and stuff
		{
			printfs("Listen socket listening on port %d\n", sid->listenPort);
		}
	} else
	{
		printfs("not listening\n");
	}
	int i;
	for(i=0; i<sid->listSize; i++)
	{
		if(sid->socketList[i]==NULL) // can't comment on this connection
			continue;
		printfs("[%d] ", i); // prints the index
		
		

		if(sid->socketList[i]->socketFd > 0) // actuall connection
		{
			if(sid->socketList[i]->protocolOption==O_UNIX)
			{
				printfs("unix domain socket connection address %s", sid->socketList[i]->remoteAddress);	
			}
			else
			{
				if(sid->socketList[i]->protocolOption==O_TCP)
					printfs("tcp connection at ");
				else if(sid->socketList[i]->protocolOption==O_UDP)
					printfs("udp datagram at ");
				else if(sid->socketList[i]->protocolOption==O_ICMP)
					printfs("icmp at ");
				else if(sid->socketList[i]->protocolOption==O_IP)
					printfs("ip at ");
				else 
					printfs("raw at ");
				printfs("address %s remotePort %d localPort %d", 
					sid->socketList[i]->remoteAddress, sid->socketList[i]->remotePort, sid->socketList[i]->localPort);
			}
			if(sid->socketList[i]->name!=NULL)
			{
				printfs(" name %s", sid->socketList[i]->name);
			}
			printfs("\n"); // for next input
		}
		else // accept node
		{
			printfs("accepting address ");
			if(sid->socketList[i]->remoteAddress!=NULL)
				printfs("%s", sid->socketList[i]->remoteAddress);
			else
				printfs("any");
			printfs(" on port ");
			if(sid->socketList[i]->remotePort>0)
				printfs("5d\n", sid->socketList[i]->remotePort);
			else
				printfs("any\n");
		}
	}
}

/*
 * prints information about connection
 * local IP: local port: remote IP: remote port
 * 
 * return 0 on sucess, -1 on error
 * could error from getpeername
 *
 * getpeer is the remote end
 * getsock is the local end 
 *
 * 	local socets in pigInfo -1 if not lisening
 * 	but when I am lissening I want to print information 
 * 	about the connection I am attempting to make
 *
 * 	will send message to cursePiggy by the given mode with the color of type
 */ 
int printAddressPort(int socket, int localSocket, int mode)
{
	int lineSize = 400; // arbitaray line size maximum
	int lineLength = 0; // how many characters are in there
	char line[lineSize+1]; // what I will be sending to cursePiggy
	memset(line, 0, lineSize+1); // scrubbing memory
	char *localName, *remoteName;
	int localPort, remotePort;
	
	struct sockaddr_in localTable;
	struct sockaddr_in remoteTable;
	socklen_t size;
	
	size = sizeof(struct sockaddr_in); // size variable to pass into getpeername
	
	// clearing memory
	memset((char *) &localTable, 0, size);
	memset((char *) &remoteTable, 0, size);


	if(socket>0) // right now connected to that socket
	{		
		// obtains peerAddress and localAddress tables
		getpeername(socket, (struct sockaddr *)  &remoteTable, &size); // remote

		getsockname(socket, (struct sockaddr *) &localTable, &size); // local
		
		remotePort = htons((u_short) remoteTable.sin_port); // obtain port information
		localPort = htons((u_short) localTable.sin_port);

		localName = inet_ntoa(localTable.sin_addr);
		lineLength += sprintf(line, "%s : %d : ", localName, localPort);

		remoteName = inet_ntoa(remoteTable.sin_addr);
		lineLength += sprintf(line+lineLength, "%s : %d", remoteName, remotePort);

		//updateConnection(line, mode, CONNECTC);
	}
	else if(localSocket > 0) // I am currently listening in that direction
	{
		#if DEBUG
			printfs("printing local socet information\n");
		#endif
		getsockname(localSocket, (struct sockaddr *) &localTable, &size); // local
	
		localPort = htons((u_short) localTable.sin_port);
		
		char name[lineSize+1]; // should hopefully fit
		memset(name, 0, lineSize+1);

		gethostname(name, lineSize); // obtains the host name
		struct hostent *host;
		host = gethostbyname(name); // obtaining name in IP form

		struct in_addr **addrList;

		addrList = (struct in_addr **) host->h_addr_list;

		lineLength += sprintf(line, "%s : %d : - : - ", inet_ntoa(*addrList[0]), localPort);
		//updateConnection(line, mode, LISTENC);
	}
	else // no sockets connect and no sockets are lissening
	{
		if(mode==WSTDIN) // printing to stdin
			//updateConnection("-:-:-:-", mode, 0);
			return 0;
		else // print to border
			//updateConnection("No Connection", mode, NOCONNECTC);
			return 0;
	}
	return 0; 
}

/*
 * helper method used in process commands
 * will attempt to lisen for a connection in that direction and add it to the set
 * must have 2 aruments in order to do this
 *
 * does not update no listen
 *
 * path is direciton I want, local port is what I will lisen on if not -1
 */
/*
int commandListen(pigInfo *lore, int argc, char *argv[], sockInfo *path, 
		int *noDirection, int *localSocket, int lport)
{
	if(*localSocket > 0)
	{
		printfs("I am already listening in that direction\n");
		return -1;
	}
	if(*noDirection == 0) // already connected in that direction
	{
		printfs("I am already connected to a piggy in that direction\n");
		return -1;
	}
	int localPort = lport < 0 ? DEFAULTPORT : lport; // is default port unless specified

	if(argc == 2)
	{
		localPort = atoi(argv[1]);
		if(localPort == 0)
		{
			printfs("%s is not a valid port number\n");
			return -1;
		}
		if(localPort < 1 || localPort > 65536)
		{
			printfs("port %d is out of range\n");
			return -1;
		}
	}
	if(!acceptTo(&lore->fdList, path, localPort, localSocket)) // was sucsefull
	{
		updateConnectionInformation(lore->llsocket, lore->rlsocket, lore->lrsocket, lore->rrsocket);
		return 0;
	}
	return -1; // else failed
}
*/

/*
 * method used to close a listening socket when the drop command has been made
 * localSocket is the guy I am going to close
 * XXX if I am working with a unix domain socket what happens to the fifo path thingy
 */ 
void closeListenSocket(sideInfo *sid, fdInfo *fdList)
{
	if(sid->listenSocket>0)
	{
		close(sid->listenSocket);
		clearFileSet(sid->listenSocket, fdList);
		sid->listenSocket = -1; // no longer listening
		if(sid->localAddress!=NULL)
		{
			remove(sid->localAddress);
			free(sid->localAddress);
			sid->localAddress = NULL;
		}
		updateBorderSideInfo(sid);
		removeDomainSockets(sid); // if it was a unix domain will attempt to remove it
	} else
		printfs("no listen socket on that side to close\n");
}

/*
 * this will set the given path the given internal strip options
 * this is set into a togal mode if choice is NULL
 * else if choice is on will turn it on, else will turn off if off specified
 * if on or off will print error messege and go about its day
 */ 
void setInternalStrip(int option, directionInfo *dir, char *choice)
{
	if(choice==NULL)
	{
		if(dir->filterIn == STRIPNO || dir->filterIn != option) // none set, or overiding previous option
		{
			dir->filterIn = option;
		}
		else // toggling off the option
		{
			dir->filterIn = STRIPNO;
		}
	}
	else if(!strcmp(choice, "on")) // want to turn it on
	{
		dir->filterIn = option;
	}
	else if(!strcmp(choice, "off")) // wants to turn it off
	{
		dir->filterIn = STRIPNO;
	}
	updateInternalFilter(dir->inWindow, dir->filterIn);
}

/*
 * method used to start up and then send to process log the given arguments
 * set up so it can be toggled on and off if they did not give a file argument
 *
 * position is the which of the log files I am working on so 0 to 4 excusive
 *
 * used to close down the log file in case of error during the command loop
 * if argv == NULL then will only close down the file and nothing else
 */ 
void commandLog(pigInfo *lore, int position, int argc, char **argv)
{
	int closed = 0; // if I closed somthing
	if(lore->writingFileList[position] > 0) // I am already loging a file there, time to close it
	{
		clearFileSet(lore->writingFileList[position], &lore->fdList); // clears from the set
		close(lore->writingFileList[position]); // closes the file
		lore->writingFileList[position] = -1; // -1 the file discriptor to tell other methods not to write to it
		#if DEBUG
			printfs("stoped writing to log file\n");
		#endif
		closed = 1;
	}
	if(argc == 1 && !closed) // no file to actually close and no log file given
	{
		printfs("no logFile name given\n");
	}
	else if(argv != NULL && !closed)  // will attempt to open the file for writing using "w" mode
	{
		FILE *logFile = fopen(argv[1], "w"); // opens it in write mode

		if(logFile == NULL)
		{
			printfs("could not write to file %s\n", argv[1]);
			return; // cannot continue
		}
		lore->writingFileList[position] = fileno(logFile); // gets the file descriptor
		addToFileSet(lore->writingFileList[position], &lore->fdList.masterWriteSet, &lore->fdList);
		printfs("starting loging to %s\n", argv[1]);
	}
	updateLogFiles(lore->writingFileList);
}

/*
 * will attempt to start the record mode
 * if arg is NULL no argument was given and will simply toggle the mode
 */ 
void startRecord(directionInfo *dir, char *arg)
{
	if(arg==NULL)
	{
		dir->inRecordMode = !dir->inRecordMode;
		printfs("toggled record mode\n");
	} else if(!strcmp(arg, "on")) // in record mode
	{
		dir->inRecordMode = 1;
	} else if(!strcmp(arg, "off")) // not in record mode
	{
		dir->inRecordMode = 0;
	} else
	{
		printfs("do not understand %s argument\n", arg);
		return; // prevent seting the frame flag
	}
	updateRecord(dir->inRecordMode, dir->inWindow, dir->delimiter);
}

/*
 * helper function that will put in the place of short the value represented by the char
 * so that is the 0-F hex standard
 * returns 1 on success
 * 0 if the character was not 0-F
 * works on lower case letters only
 */
int setHexValue(short *destination, char value)
{
	int i;
	for(i=0; i<10; i++)
	{
		if(value == '0'+i) // thats the character we are looking for, it lines up with our
		{
			*destination = (short) i;
			return 1;
		}
	}
	for(i=0; i<6; i++) // now for a-f
	{
		if(value == 'a'+i) // found it
		{
			*destination = (short) i+10;
			return 1;
		}
	}
	// else I did not find it, it is not in my range so I am returning 0
	return 0;
}

/*
 * process the delimiter
 * if no argument is given will print the delimiter instead
 * the idea is they give me the delimiter in hexedecimal form
 * easy way to describe non-printiable characters and such
 * always assumes it starts with 0x
 */ 
void processDelimiter(directionInfo *dir, int argc, char *arg)
{
	if(argc==1) // no argument was given, going to print out the delimiter
	{
		printfs("delimiter:");
		printMessageNoLine(0, dir->delimiter, strlen(dir->delimiter));
		printfs("\n"); // add a newline
	} else 
	{
		toLowerCase(arg); // converts it to lower case so I can read it acuratly
		size_t length = strlen(arg);
		if(strstr(arg, "0x")==arg && length>2) // starts with 0x is hexedecimal
		{	
			size_t amount = (size_t) ((length-2)/2)+length%2;
			char *delimiter = (char *) malloc(sizeof(char)+amount+2); // obtains the memory needed
			memset(delimiter, 0, amount+2); // whipes memory
			int isCorrect = 1; // starts out on the assumption that they gave me somthing I could read
			int i;
			int isLittle = isLittleEndian();
			if(length%2) // going to have to do the first one to start out with 
			{
				short lowerBound = 0;
				if(!setHexValue(&lowerBound, arg[2])) // they did not give me propper hex so I error
				{
					printfs("The argument %s does not conform with hexidecimal notation\n", arg);
					isCorrect = 0;
					free(delimiter);
				} else if(isLittle) // dealing with little endan so I can just put it in the char position
				{
					delimiter[0]=lowerBound;
				} else // I'm actaully big so I'm going to have to shift it up by 4 before storing
				{
					delimiter[0]=lowerBound<<4;
				}
			}
			for(i=2+length%2; i<length && isCorrect; i+=2) // reads 2 bytes at a time offset by 1 if it was odd number
			{
				short lowerBound=0, upperBound=0; // lower and upper of
				if(!setHexValue(&upperBound, arg[i]) || !setHexValue(&lowerBound, arg[i+1])) // failed was not correct
				{
					printfs("The argument %s does not conform with hexidecimal notation\n", arg);
					isCorrect = 0;
					free(delimiter); // freeing memory that I am using
					continue; // will exit the loop
				}
				if(!isLittle) // going to have to swap the lower and upper bounds so that it works with the little and big endan
				{
					short temp = lowerBound;
					lowerBound = upperBound;
					upperBound = temp;
				}
				char result = lowerBound + (upperBound<<4);
				delimiter[(int) (((i-2)/2)+(length%2))] = result; // places the character value were it is supposed to be
				#if DEBUG
					printfs("The character of the result is %c\n", result);
				#endif
			}
			if(isCorrect) // made it through everything with it being correct
			{
				free(dir->delimiter);
				dir->delimiter = delimiter;
				#if DEBUG
					printfs("set the new delimiter and it is %s\n", dir->delimiter);
				#endif
				updateRecord(dir->inRecordMode, dir->inWindow, dir->delimiter);
			}
		} else
		{ // print error message
			printfs("the argument %s does not start with 0x and is not hexidecimal representation\n", arg);
		}
	}
}


/*
 * helper method to start reading from a script file and doing stuff
 * will be called at run time with the -s command line argument
 * or when piggy is active by a command
 */ 
void startScripting(pigInfo *lore, char *fileName)
{
	FILE *scriptFile = fopen(fileName, "r"); // open for reading
	if(scriptFile == NULL) // failed to open the file
		printfs("could not open %s for reading\n", fileName);
	else // put it into the set and stuff
	{
		lore->scriptFd = fileno(scriptFile);
		addToFileSet(lore->scriptFd, &lore->fdList.masterReadSet, &lore->fdList);
		// will process reading the scriptFile in the network Loop select statments
	}
}

/*
 * function to start reading from a text file sending it in the direction of stdin
 */ 
void startReading(pigInfo *lore, char *fileName)
{
	if(lore->readFd < 0) // am not currently reading a file
	{
		FILE *readingFile = fopen(fileName, "r"); // opens file in read mode
		if(readingFile == NULL) // error occured when opening file
		{
			printfs("could not open file %s\n", fileName);
		}
		else // adds the file to the fileSet
		{
			lore->readFd = fileno(readingFile); // obtains fileDescriptor of file
			addToFileSet(lore->readFd, &lore->fdList.masterReadSet, &lore->fdList);
			#if DEBUG
				printfs("starting to read file\n");
			#endif
		}
	}
	else // curretnly reading a file
	{
		printfs("I am already reading a file\n");
		#if DEBUG
			printfs("reading file %d\n", lore->readFd);
		#endif
	}
}

/*
 * used by the child process to close all refrences to other file descriptors
 * was runing into issues if I killed my piggy
 * iterates through the master sets and if the number is set I will close it
 * don't need to update anything else in the structs cause I will be calling exect
 */ 
void closeAll(fdInfo *fdList)
{
	int i;
	for(i=fdList->maxFileDescriptor; i>2; i--) // start from the top go to the bottom
	{ // do not include stdin, stderr or stdout
		if(FD_ISSET(i, &fdList->masterReadSet) || FD_ISSET(i, &fdList->masterWriteSet))
			close(i); // I had a connection time to close it
	}
}

/*
 * checks the listening side if it was listining to a unix domain socket on a given path
 * if that was the case will remove that file from the path
 * goes under the assumption that the CRW has not changed and it can access that file
 */ 
void removeDomainSockets(sideInfo *sid)
{
	if(sid->localAddress!=NULL) // well we have one that could be open, going to get rid of it
	{
		remove(sid->localAddress); // hopfully will work
		// if it was already removed then it will return an error but I don't care
	}
	// don't have to worry about freeing memory because if I need to use a new one I will just overrite it
}

/*
 * will send the command character to the tmux session
 * used to change sessions to spawn new pigs and stuff
 * then  Iwill wait for the child to die off before returning as the parent
 * this is to keep the order of execing the same because of race conditions between child and parent
 * also goes under the assumption that the tmux config goes under C-b and not any other control commands
 */
/*
void sendCommandCharacter(char data, fdInfo *fdList)
{
	pid_t pid = fork();
	if(pid < 0)
	{
		perror("fork errored ");
		exitCurses(EXIT_FAILURE);
	}
	if(pid == 0) // Waa waa child has been born
	{
		closeAll(fdList);
		char **argv = malloc(sizeof(char *)*6);
		char temp[2];
		temp[0] = data;
		temp[1] = '\0';
		argv[0] = "tmux";
		argv[1] = "send-keys";
		argv[2] = "C-b";
		argv[3] = temp;
		argv[4] = NULL;
		execvp("tmux", argv);
		// if I get to here something bad happend in the code
		printfs("something bad happend to the child\n");
		exit(EXIT_FAILURE);
	} else // I am the parrent aparently... get it
	{
		#if DEBUG
			printfs("my child pid is %d\n", pid);
		#endif
		int check;
		waitpid(pid, &check, 0); // TODO make sure that this will work, check with textbook
	}
}
*/

/*
 * helper function that coppies the arg list 
 * getopt changes the order of the arguments so I am creating a coppy to prevent this
 * does call malloc but this function is called by children so should be fine
 * will also coppy the actual string inside
 */ 
char **coppyArgList(int argc, char **argv)
{
	char **result = malloc(sizeof(char *)*(argc+1));
	int k;
	for(k=0; argv[k]!=NULL; k++)
	{
		int length = strlen(argv[k]);
		char *temp = malloc(sizeof(char)*(length+1));
		strncpy(temp, argv[k], length);
		temp[length] = '\0'; // add the null termination
		result[k]=temp;
	}
	result[k]=NULL;
	return result;
}

/*
 * sends the global quit detach and global detatch to tmux
 * runns the script quitPiggy.sh
 * don't need to check the validity of command since it is checkd in the command loop
 */
void sendQuitCommand(char *command, fdInfo *fdList, char *serverName)
{
	pid_t pid = fork();
	if(pid < 0)
	{
		perror("fork errored ");
		exitCurses(EXIT_FAILURE);
	} if(pid == 0) // they grow up so fast
	{
		closeAll(fdList);
		char **argv = malloc(sizeof(char *)*6);
		argv[0] = "dummy string";
		argv[1] = "-S";
		argv[2] = serverName;
		argv[3] = "-Q";
		argv[4] = command;
		argv[5] = NULL;
		
		execvp("./quitNetHog.sh", argv);
		// if I made it here something whent wrong
		perror("something bad happend to child ");
		exitCurses(EXIT_FAILURE);
	} else // Im home kids
	{
		int check;
		waitpid(pid, &check, 0); 
	}
}

/*
 * attempts to spawn a remote pig by calling ssh and using tmux shenanagans
 * the idea is we are using nested tmux sessions to acomplish this
 *
 * TODO right now going under assumption that it is port 922 the default port
 * the first argument always given should be the ssh login key thing username@host
 * then the rest will be actual commands some of which will be used to process the ssh
 *
 * XXX Because of how the options flags are set I can no longer use A or B as a small option prefix for 
 * the main command line argument flags
 *
 * TODO add window name option and stuff
 */ 
void spawnRemotePig(int argc, char *argv[], fdInfo *fdList, char *serverName, char *line)
{
	if(serverName == NULL)
	{
		printfs("cannot do operation since was not started with script\n");
		return;
	} if(argc<2)
	{
		printfs("no remote address given\n");
		return;
	}
	struct option long_options[] =
	{
		{"sshport", required_argument, 0, 'P'}, // defaults to 922
		{"workingdirectory", required_argument, 0, 'W'},
		{"wd", required_argument, 0, 'B'},
		{0, 0, 0, 0}
	};
	
	pid_t pid = fork();
	if(pid < 0)
	{
		perror("fork errored ");
		exitCurses(EXIT_FAILURE);
	} if(pid == 0) // its a child
	{
		closeAll(fdList); // no need to keep them file descriptors around
		int scale = 10; // how much I need extra for my malloc
		
		int theCount = 0;
		char **newArgs = createArgs(&theCount, line, 1); // really ugly doing this but its because how ssh works
		
		#if DEBUG // going to print out the list
			int z;
			printfs("new arguments [");
			for(z=0; newArgs[z]!=NULL; z++)
			{
				printfs("%s", newArgs[z]);
				if(newArgs[z+1]==NULL)
					printfs("]\n");
				else
					printfs(", ");
			}
		#endif

		char **realArgs = malloc(sizeof(char *)*(theCount+scale)); // what I will be puting the arguments in
		memset(realArgs, 0, theCount+scale); // sets the memory to zero so they all start out as null
		char **coppyArgs = coppyArgList(theCount, newArgs);
		char *workingDirectory = NULL;
		char *portNumber = SSHPORT; // default port number usually 922
		optind = 1;
		opterr = 0;
		while(1)
		{
			int option_index = 0;
			int c = getopt_long(theCount, newArgs, "P:W:", long_options, &option_index);
			if(c==-1) // alldone
				break;
			switch(c)
			{
				case 0: // option flag set
					break;
				case 'A':
					portNumber = optarg;
					break;
				case 'B':
					workingDirectory = malloc(PATH_MAX+1);
					memcpy(workingDirectory, optarg, strlen(optarg));
					workingDirectory[strlen(optarg)]='\0';
					break;
				case '?': // going to process a lot of arguments it does not understand so no error statment
					break;
			}
		}
		// end of processing all the commands
		if(workingDirectory==NULL) // they did not supply a directory to find the piggy executable so use the one I have
		{
			workingDirectory = malloc(PATH_MAX+1);
			if(getcwd(workingDirectory, PATH_MAX)==NULL)
			{
				perror("get working directory failed ");
				exitCurses(EXIT_FAILURE); // don't know what happend
			}
			// else fine now and have the current path to were piggy executible is
		}
		// start creating ht real arguments
		realArgs[0] = "remote pig";
		realArgs[1] = "-C";
		realArgs[2] = coppyArgs[1];
		realArgs[3] = "-P";
		realArgs[4] = portNumber;
		realArgs[5] = "-W";
		realArgs[6] = workingDirectory;
		realArgs[7] = "-S";
		realArgs[8] = serverName;
		int i;
		for(i=2; i<theCount; i++) // coppys over all the rest of the arguments
		{
			realArgs[7+i] = coppyArgs[i];
			#if DEBUG
				printfs("coppyArgs is %s realArgs is %s\n", coppyArgs[i], realArgs[7+i]);
			#endif
		}
		realArgs[7+i] = NULL;
		#if DEBUG // going to print out the list
			int k;
			printfs("remote pig arguments [");
			for(k=0; realArgs[k]!=NULL; k++)
			{
				printfs("%s", realArgs[k]);
				if(realArgs[k+1]==NULL)
					printfs("]\n");
				else
					printfs(", ");
			}
		#endif

		execvp("./spawnRemoteHog.sh", realArgs);
		// if I made it here something whent wrong
		perror("something bad happend to child ");
		/*printfs("realArgs=[");
		for(i=0; realArgs[i]!=NULL; i++)
		{
			printfs("%s, ", realArgs[i]);
		}
		printfs("]\n");
		sleep(15);
		*/
		exitCurses(EXIT_FAILURE);
	} else // big daddy
	{
		#if DEBUG
			// printfs("child has been created hopfully with pid of %ld\n", pid);
		#endif
		int check;
		waitpid(pid, &check, 0); // TODO make sure that this will work, check with textbook
		if(check == 2) // I am not on a machine that has access to the tmux server
		{
			printfs("cannot create remote pig since I do not have access to main tmux server\n");
		}
	}
}

/*
 * going off the current tmux session and cass exect on a tmux command that will 
 * spawn a new pig
 * given the command argc and argv from creating the local piggy
 * I do belive that the actual pig will not be a child of me
 */ 
void spawnLocalPig(int argc, char *argv[], fdInfo *fdList, char *serverName)
{
	if(serverName == NULL)
	{
		printfs("was not launched with script cannot perform command\n");
		return;
	}
	pid_t pid = fork();
	if(pid < 0)
	{
		perror("fork errored ");
		exitCurses(EXIT_FAILURE);
	}
	if (pid == 0) // our very own child
	{
		closeAll(fdList);
		char **args = malloc(sizeof(char *)*(argc+4));
		memset(args, 0, argc+4);
		args[0] = "new pig"; // dummy string will be ignored
		args[1] = "-S";
		args[2] = serverName;
		int i;
		for(i=1; i<argc; i++)
		{
			args[2+i] = argv[i];
		}
		args[2+i] = NULL; // should do it
		
		#if DEBUG // going to print out the list
			int k;
			printfs("local pig arguments [");
			for(k=0; args[k]!=NULL; k++)
			{
				printfs("%s", args[k]);
				if(args[k+1]==NULL)
					printfs("]\n");
				else
					printfs(", ");
			}
		#endif
			
		execvp("./spawnLocalHog.sh", args);
		// now now something did now work if I got to this point
		perror("something bad happend in the child ");
		exit(EXIT_FAILURE);
	} else // daddys home
	{
		#if DEBUG
			printfs("the child process pid is %d\n", pid);
		#endif
	}
}

/*
 * method to kill the child process remove its information from the list and set up
 * a pipe and put it into the fdList
 *
 * does not actualy call openPipes, that must seperatly be called
 * this is due to I might be closing an external filter down just to start a new one up
 */ 
void killChild(directionInfo *dir, fdInfo *fdList)
{
	int check;
	if(dir->external<=0) // no child, does nothing
		return;
	kill(dir->external, SIGKILL); // good night sweet prince
	closePipes(dir, fdList); // also will close anything that was previously in there
	waitpid(dir->external, &check, WNOHANG); // so that it does not turn into a zombie
		// XXX need to determin if no hang is proper option
	dir->external = -1; // seting it so it does not have a child any more
	freeArgs(dir->externalCommandCount, dir->externalCommand);
}

/*
 * method that will start an external filter or will stop an external filter
 * 
 * calls fork and all that fun piping stuff from Unix to set up an external filter
 * if it gets called with no arguments and there is an external filter already in existance
 * will clober that guy and continue on
 *
 * position is INLEFT or INRIGHT for printinf information to the borders
 */ 
void commandExternal(int argc, char **argv, directionInfo *dir, fdInfo *fdList)
{
	if(dir->external) // I have an external filter already running, time to kill it
	{
		killChild(dir, fdList);
		if(argc==1) // just need to kill the child, updating information
		{
			updateExternalFilter(dir->inWindow, NULL, 0);
			return;
		}
	}
	if(argc>1) // I have arguments to fork 
	{
		closePipes(dir, fdList); // close the pipes before I fork
		int fds1[2]; // file sets for piping
		int fds2[2];
		if(!dir->inRecordMode) // not record will use a regular pipe
		{
			// set up file descriptors
			//signal(SIGPIPE, SIG_IGN);
			if(pipe(fds1)<0) 
			{
				perror("pipe error ");
				exitCurses(EXIT_FAILURE);
			} if(pipe(fds2)<0) 
			{
				perror("pipe error ");
				exitCurses(EXIT_FAILURE);
			}
		} else
		{
			if(socketpair(AF_UNIX, SOCK_DGRAM, 0, fds1))
			{
				perror("sockpair errored ");
				exitCurses(EXIT_FAILURE);
			} if(socketpair(AF_UNIX, SOCK_DGRAM, 0, fds2))
			{
				perror("sockpair errored ");
				exitCurses(EXIT_FAILURE);
			}
		}
		pid_t pid = fork(); // buckle your pants we be forking now
		if(pid < 0)
		{
			perror("fork errored ");
			exitCurses(EXIT_FAILURE); // don't really know how to recover from this so have to terminate
		}
		if(pid == 0) // we got ourselves a child
		{
			closeAll(fdList); // close all file descriptors I have
			close(fds1[1]); 
			setbuf(fdopen(fds1[0], "r"), NULL);
			dup2(fds1[0], STDIN_FILENO); // my new stdin
			close(fds2[0]); // don't need this, its the parents
			setbuf(fdopen(fds2[1], "w"), NULL);
			dup2(fds2[1], STDOUT_FILENO); // my new stdout
			argv[argc] = NULL; // makes sure it is null terminated, might not be due to realloc
			execvp(argv[1], &argv[1]); // exects the function
			// well this is awkward, the execvp did not work
			// perror("could not call execvp ");
			close(fds1[0]); // hopfully this will get the parents attention that I died
			close(fds2[1]); 
			exit(EXIT_FAILURE);
		}
		else // we have a parrent
		{
			dir->external = pid;
			close(fds1[0]); // belong to the child
			close(fds2[1]);
			dir->windowLink[0] = fds2[0]; // tranfering over the information
			dir->windowLink[1] = fds1[1];
			
			// XXX testing debuging code

			addToFileSet(dir->windowLink[0], &fdList->masterReadSet, fdList); // adds information to the fileList
			addToFileSet(dir->windowLink[1], &fdList->masterWriteSet, fdList);
			#if DEBUG
				printfs("finished creating child\n");
			#endif
			updateExternalFilter(dir->inWindow, &argv[1], argc-1);
			dir->externalCommand = coppyArgList(argc, argv);
			dir->externalCommandCount = 1;
		}
	}
	else
		printfs("no arguments given\n");
}

/*
 * metthod to process the commands made in command mode from stdin
 *
 * given pigInfo about the current pig
 * the fileSet and the maxFile in case I have to close a pig
 *
 * the string that I recived from stdin that I will be processing
 * removes the last newLine character on the buffer
 * 
 * returns 0 on sucess
 * -1 on error
 */ 
int processCommands(pigInfo *lore, char *buff)
{
	if(buff[0] == '\n') // will break code, they just hit enter
		return 0; 

	size_t newLinePosition = strlen(buff) -1; // going to remove newling
	if(buff[newLinePosition] == '\n')
		buff[newLinePosition] = '\0';

	// first must tokenize the string
	char *line = buff; // variables used to tokenize the string
	
	int argCount = 0;
	char **args = createArgs(&argCount, line, 0);
	
	#if DEBUG // going to print out the list
		int k;
		printfs("command arguments [");
		for(k=0; args[k]!=NULL; k++)
		{
			printfs("%s", args[k]);
			if(args[k+1]==NULL)
				printfs("]\n");
			else
				printfs(", ");
		}
	#endif
	
	if(!strcmp(args[0], ":q")) // quiting program
	{
		if(lore->lr.external > 0) // if I have external functions runing must kill them
			killChild(&lore->lr, &lore->fdList);
		if(lore->rl.external > 0)
			killChild(&lore->rl, &lore->fdList);
		printfs("Exiting Program. Have A Nice Day\n"); // probably will never see this
		removeDomainSockets(&lore->left);
		removeDomainSockets(&lore->right);
		exitCurses(EXIT_SUCCESS); // funny enough did not really set up a gracefull way of exiting the program
		// the operating system should handle closing those sockets for me
	} else if(/*!strcmp(args[0], ":q!") ||*/ !strcmp(args[0], ":d") || !strcmp(args[0], ":d!")) // send quit command
	{
		sendQuitCommand(args[0], &lore->fdList, lore->serverName);
	} else if(!strcmp(args[0], ":outputl")) // set output direction to left
	{
		if(lore->middle == &lore->rl) // already in that direction
		{
			printfs("output already going to the left\n");
		} else // change direction
		{
			lore->middle = &lore->rl;
			updateMiddleDir(-1); // negative goes to left
		}
	} else if(!strcmp(args[0], ":outputr")) // change output direction to the right
	{
		if(lore->middle == &lore->lr) // already in that direction
		{
			printfs("output already going to the right\n");
		} else // changing direction
		{
			lore->middle = &lore->lr;
			updateMiddleDir(1); // positive goes to right
		}
	} else if(!strcmp(args[0], ":output")) // show direction output is sent to
	{
		if(lore->middle == &lore->lr)
			printfs("output going left to right\n");
		else
			printfs("output going right to left\n");
	} 
	else if(!strcmp(args[0], ":droplistenl"))
	{
		closeListenSocket(&lore->left, &lore->fdList);
	} 
	else if(!strcmp(args[0], ":droplistenr"))
	{
		closeListenSocket(&lore->right, &lore->fdList);
	} 
	else if(!strcmp(args[0], ":dropconnectl"))
	{
		closePiggy(&lore->left, &lore->fdList, 0, argCount, args);
	} 
	else if(!strcmp(args[0], ":dropconnectr"))
	{
		closePiggy(&lore->right, &lore->fdList, 0, argCount, args);
	}
	else if(!strcmp(args[0], ":dropacceptl")) // used to drop expected connections
	{
		closePiggy(&lore->left, &lore->fdList, 1, argCount, args);
	}
	else if(!strcmp(args[0], ":dropacceptr"))
	{
		closePiggy(&lore->right, &lore->fdList, 1, argCount, args);
	}
	else if(!strcmp(args[0], ":right") || !strcmp(args[0], ":rpair")) // print right connections information
	{	
		//printAddressPort(lore->rrsocket, lore->rlsocket, WSTDIN);
		printSideInfo(&lore->right);
	}
	else if(!strcmp(args[0], ":left") || !strcmp(args[0], ":lpair")) // print left connections information
	{
		//printAddressPort(lore->lrsocket, lore->llsocket, WSTDIN);
		printSideInfo(&lore->left);
	}
	else if(!strcmp(args[0], ":loopr")) // changes loop mode to loop right
	{
		if(lore->looping==RIGHTLOOP) // will no longer loop
			lore->looping = 0;
		else
			lore->looping = RIGHTLOOP;
		updateLoop(lore->looping);
	}
	else if(!strcmp(args[0], ":loopl")) // changes loop mode for left
	{
		if(lore->looping==LEFTLOOP)
			lore->looping = 0;
		else
			lore->looping = LEFTLOOP;
		updateLoop(lore->looping);
	}
	else if(!strcmp(args[0], ":connectr"))
	{
		processConnection(&lore->right, &lore->fdList, argCount, args); // just like the command line
	}
	else if(!strcmp(args[0], ":connectl"))
	{
		processConnection(&lore->left, &lore->fdList, argCount, args);
	}
	else if(!strcmp(args[0], ":listenl"))
	{
		processListen(&lore->left, &lore->fdList, argCount, args);
	}
	else if(!strcmp(args[0], ":listenr"))
	{
		processListen(&lore->right, &lore->fdList, argCount, args);
	}
	else if(!strcmp(args[0], "listenopl")) // setting up accept options
	{
		processListenOption(&lore->left, argCount, args);
	}
	else if(!strcmp(args[0], "listenopr"))
	{
		processListenOption(&lore->right, argCount, args);
	}
	else if(!strcmp(args[0], ":read")) // will read from the file
	{
		if(argCount > 1) // have a file
		{
			startReading(lore, args[1]);
		} else
			printfs("no file given\n");
	}
	else if(!strcmp(args[0], ":stlrnp")) // left to right strip non printable characters
	{
		setInternalStrip(STRIPALL, &lore->lr, args[1]);
	}
	else if(!strcmp(args[0], ":strlnp")) // right left
	{
		setInternalStrip(STRIPALL, &lore->rl, args[1]);
	}
	else if(!strcmp(args[0], ":stlrnpxeol")) // strips non printables left to right except newline and carage return
	{
		setInternalStrip(STRIPSOME, &lore->lr, args[1]);
	}
	else if(!strcmp(args[0], ":strlnpxeol"))
	{
		setInternalStrip(STRIPSOME, &lore->rl, args[1]);
	}
	else if(!strcmp(args[0], ":source")) // start scripting
	{
		if(argCount < 2)
			printfs("no script file given\n");
		else // attempts to open the sorce file
			startScripting(lore, args[1]);
	}
	else if(!strcmp(args[0], ":loglrpre")) // log files
	{
		commandLog(lore, INLEFT-1, argCount, args);
	}
	else if(!strcmp(args[0], ":loglrpreoff"))
	{
		commandLog(lore, INLEFT-1, 0, NULL); // garentees that I will close it
	}
	else if(!strcmp(args[0], ":logrlpre"))
	{
		commandLog(lore, INRIGHT-1, argCount, args);
	}
	else if(!strcmp(args[0], ":logrlpreoff"))
	{
		commandLog(lore, INRIGHT-1, 0, NULL);
	}
	else if(!strcmp(args[0], ":loglrpost"))
	{
		commandLog(lore, OUTRIGHT-1, argCount, args);
	}
	else if(!strcmp(args[0], ":loglrpostoff"))
	{
		commandLog(lore, OUTRIGHT-1, 0, NULL);
	}
	else if(!strcmp(args[0], ":logrlpost"))
	{
		commandLog(lore, OUTLEFT-1, argCount, args);
	}
	else if(!strcmp(args[0], ":logrlpostoff"))
	{
		commandLog(lore, OUTLEFT-1, 0, NULL);
	}
	else if(!strcmp(args[0], ":externallr")) // start up or shut down external filtering
	{
		commandExternal(argCount, args, &lore->lr, &lore->fdList);
	}
	else if(!strcmp(args[0], ":externallroff")) // toggling works but o well
	{
		killChild(&lore->lr, &lore->fdList);
		updateExternalFilter(INLEFT, NULL, 0);
	}
	else if(!strcmp(args[0], ":externalrl"))
	{
		commandExternal(argCount, args, &lore->rl, &lore->fdList);
	}
	else if(!strcmp(args[0], ":externalrloff"))
	{
		killChild(&lore->rl, &lore->fdList);
		updateExternalFilter(INRIGHT, NULL, 0);
	}
	else if(!strcmp(args[0], ":recordlr")) // togles being in record mode
	{
		startRecord(&lore->lr, args[1]);	
	}
	else if(!strcmp(args[0], ":recordrl"))
	{
		startRecord(&lore->rl, args[1]);
	}
	else if(!strcmp(args[0], ":delimiterlr"))
	{
		processDelimiter(&lore->lr, argCount, args[1]);
	}
	else if(!strcmp(args[0], ":delimiterrl"))
	{
		processDelimiter(&lore->rl, argCount, args[1]);
	}
	else if(!strcmp(args[0], ":spawnlocalpig")) // spawns a local pig
	{
		spawnLocalPig(argCount, args, &lore->fdList, lore->serverName);
	}
	else if(!strcmp(args[0], ":spawnremotepig"))
	{
		spawnRemotePig(argCount, args, &lore->fdList, lore->serverName, line);
	}
	else if(!strcmp(args[0], ":help")) // prints help message
	{
		if(argCount == 1) // did not specify what type of help
			printHelp();
		else if(!strcmp(args[1], "connect"))
			printHelpConnect();
		else if(!strcmp(args[1], "listen"))
			printHelpListen();
		else if(!strcmp(args[1], "listenop"))
			printHelpListenOption();
		else if(!strcmp(args[1], "protocol"))
			printHelpProtocol();
		else
			printfs("%s is not valid help argument\n", args[1]);
	}
	else if(!strcmp(args[0], ":clear")) // clears all the windows
	{
		clearWindows();
	}
	else if(!strcmp(args[0], ":author")) // prints author's name
	{
		printfs("Albert Andrew Spencer\n");
	}
	else if(!strcmp(args[0], ":piggy")) // easter egg oink thing
	{
		printfs("piggy goes oink oink!\n"); // I told my freinds this program went oink, and so it does
	}
	else if(!strcmp(args[0], ":webserverl")) // TODO so its not done yet
	{
		
	}
	else if(!strcmp(args[0], ":webserverr"))
	{

	}
	else if(strlen(buff) != 0)
	{
		printfs("%s is not a valid command\n", buff);
	}
	freeArgs(argCount, args); // free my arguments that I malloced
	return 0; // no errors somehow
}

/*
 * will attempt to close the connection defined by the commands given
 * if no argmunts given will attempt to close all conections on that side
 * else will only attempt to close those connections given by the number or name of that connection
 * the close type argument represent what nodes it should close
 * 0 for closing only connected nodes
 * 1 for closing only accepting nodes
 *
 * TODO add regular expression support
 */
void closePiggy(sideInfo *sid, fdInfo *fdList, int closeType, int argc, char **argv)
{
	int i, j;
	for(i=0; i<sid->listSize; i++)
	{
		if(sid->socketList[i]==NULL || ((closeType==0 && sid->socketList[i]->socketFd<0) 
				|| (closeType==1 && sid->socketList[i]->socketFd>0)))
		{
			continue; // does not exist or not an actual connection
		}
		if(argc>1) // was asked for a specific connection to drop
		{
			#if DEBUG
				printfs("testing if there is a connection to close\n");
			#endif
			for(j=1; j<argc; j++) // determin if it is somthing that I need to remove
			{
				int connectionNum; // the number in the string
				if((connectionNum = atoi(argv[j]))==0) // no number
				{
					// XXX is there null characters in name and in the argv
					if(sid->socketList[i]->name != NULL && !strcmp(sid->socketList[i]->name, argv[j]))
					{ // they specified the name to kill
						#if DEBUG
							printfs("closing connectino because it matches name\n");
						#endif
						removeSocketInfo(sid, i, fdList);
						break;
					}
				} else if(connectionNum == i) // specified the correct index
				{
					#if DEBUG
						printfs("closing connection because it matches index\n");
					#endif
					removeSocketInfo(sid, i, fdList);
					break;
				}
			}
		} else // going to drop it
		{
			#if DEBUG
				printfs("closing connection because it exists\n");
			#endif
			removeSocketInfo(sid, i, fdList);
		}
	}
}

/*
 * used for processing the script file
 * will read a line at a time and will process the commands just like stdin
 * when it its the end of the file it will stop reading commands and close the file
 * prints the commands that it is going to perform to the stdin window
 *
 * reads given input into a char *buffer of BUFFSIZE
 * if they overflow it I will lose characters before the new line
 * since the default of BUFFSIZE is 4K bytes they would probably be doing some bad stuff to my program
 *
 * interesting thing, apparently making sucesseve calls to fdopen will set the file to the end of file position or something
 * or at least that is the apparent result to the fgets call
 *
 * also first time useing static variable in a c function hype
 */ 
void processScript(int *fd, pigInfo *lore, fd_set *readSet)
{
	if(*fd > 0 && FD_ISSET(*fd, readSet)) // can read
	{
		char buff[BUFFSIZE+1]; // creates a buffer to read from
		memset(buff, 0, BUFFSIZE+1); // have to scrub it becuase of array processing reasons
		static FILE *scriptFile = NULL; // used to describe the file pointer for scripting
		if(scriptFile == NULL)
			scriptFile = fdopen(*fd, "r"); // opens the file for reading
		
		if(fgets(buff, BUFFSIZE, scriptFile) == NULL) // reached the end of the file when reading
		{
			clearFileSet(*fd, &lore->fdList); 
			close(*fd); // closes the file that I was reading
			*fd = -1; // prevent further reads
			#if DEBUG
				printfs("finished reading the script file\n");
			#endif
			scriptFile = NULL; // so I get a new one next call of the function
		} else // have a command to process
		{
			if(strchr(buff, '\n')==NULL) // prints it with a new line character
				printfs("%s\n", buff);
			else // alread has a new line in there and will not print it
				printfs("%s", buff);
			processCommands(lore, buff); // will attempt to process that command
			#if DEBUG
				printfs("finished processing script command\n");
			#endif
		}
	}
}

/*
 * just like process Socket except for stdin
 *
 * the one exeption is that it first checks if the first character is the escape key command
 * this is not the case however if I am already in command mode
 *
 * insert mode is 1 if I am in insert mode, 0 means in command mode
 *
 * *readSet and *maxFile is used by the process command function when I am in command mode
 *
 * sendFile and loopFile is used when I am in insert mode when I am sending information
 * pigInfo * is also required for process command method
 *
 * will turn into command mode if the esc key is the first character on the line
 * if there is additional information on that line it will also be discarded
 * this will no longer be a problem in piggy3 when we will start using ncurses
 * 
 * does not really return anything all errors handled in program XXX is it....
 */ 
void processStdin(pigInfo *lore, fd_set *writeSet)
{
	char *buff = input(&lore->mode); // obtains the potential full message
	
	if(buff!=NULL) // got a command
	{
		#if DEBUG
			happend++;
		#endif
		if(!lore->mode && lore->middle != NULL) // in insert mode
		{
			/*
			#if DEBUG
				printfs("I got a message from stdin\n");
			#endif
			*/
			sendInDirection(buff, strlen(buff), lore->middle, &lore->fdList, writeSet); 
			// process message reguarly
		}
		else
			processCommands(lore, buff); // process the command given
		
		free(buff); // freeing memory saves lives
	}
}

/*
 * method that will acept the connection in the network loop
 * print error messages on errors
 * used for both accepting left and right connections
 * 
 * will check if there is a socket that I am listening on
 * and if I do have such a socket whether or not there is a guy on the other end attempting to connect
 * TODO add funcionality and flags to close the listen socket after so many conections and such
 *
 * XXX if I am listening but have exceeded my maximum number of connections on this side what will happen
 * 		if I choose to never pick up the phone
 */ 
void processAccept(sideInfo *sid, fdInfo *fdList, fd_set *readSet)
{
	if(sid->listenSocket > 0 && (sid->maxConnections<0 || sid->connectionCount<sid->maxConnections) 
			&& FD_ISSET(sid->listenSocket, readSet))
	{ // I am under my maximum and someone wants to connect
		#if DEBUG
			printfs("I got a connection on my listen\n");
		#endif
		int remoteSocket = acceptSocket(sid->listenSocket);
		#if DEBUG
			if(remoteSocket < 0)
				printfs("I failed to accept the socket\n");
			else
				printfs("I have accepted the socket\n");
		#endif
		// XXX seg faults after this point when unix domain socket occurs
		if(remoteSocket < 0) // accept failed time to go home
			return;

		if(sid->listenProtocol!=O_UNIX) // its not a unix need to check if it has the correct connect
			hasCorrectConnection(sid, remoteSocket, fdList);
		else // else I wll need to add it to the list
		{
			addToFileSet(remoteSocket, NULL, fdList);
			sockInfo *path = createSocketInformation(remoteSocket, -1, -1, "need to find out how to get the path ", 
					sid->listenProtocol, sid->listenIpType, NULL, BUFFSIZE*2);
			addToSocketList(sid, path);
		}
	}
}

/*
 * helper function that will read in the text file if pressent and has data
 * reads by the BUFFSIZE and stuff
 * sends the read information to the same direction as stdin
 * when finished reading the file will close the file and remove it from the set
 */ 
void readInFile(int *fd, directionInfo *dir, fdInfo *fdList, fd_set *readSet, fd_set *writeSet)
{
	if(*fd > 0 && FD_ISSET(*fd, readSet) && canReadInDirection(dir))
	{ // if I have a file, it can be read, and I can write the subsequent data
		#if DEBUG
			printfs("reading from the file\n");
		#endif
		char buff[BUFFSIZE+1]; // were I hold the message
		ssize_t check = read(*fd, buff, BUFFSIZE);
		if(check<=0) // reading failed
		{
			#if DEBUG
				printfs("stopped reading\n");
			#endif
			clearFileSet(*fd, fdList);
			close(*fd); 
			*fd = -1; // so I do not attempt to read from it again
		} else // read was sucess
			sendInDirection(buff, check, dir, fdList, writeSet); // don't need to wory about return
		// if have read everything from the file next networkLoop I will get an error and close the file
	}
}

/*
 * helper function to update all border information I have
 */ 
void updateAllBorders(pigInfo *lore)
{
	updateBorderSideInfo(&lore->left);
	updateBorderSideInfo(&lore->right);
	if(lore->middle == &lore->lr)
		updateMiddleDir(1);
	else
		updateMiddleDir(-1); // going right
	updateLoop(lore->looping);
	updateMode(lore->mode);
	updateInternalFilter(INLEFT, lore->lr.filterIn);
	updateInternalFilter(INRIGHT, lore->rl.filterIn);
	if(lore->lr.external > 0) // have external filter
		updateExternalFilter(INLEFT, &lore->lr.externalCommand[1], lore->lr.externalCommandCount-1);
	else
		updateExternalFilter(INLEFT, NULL, 0);
	if(lore->rl.external > 0)
		updateExternalFilter(INRIGHT, &lore->rl.externalCommand[1], lore->rl.externalCommandCount-1);
	else
		updateExternalFilter(INRIGHT, NULL, 0);
	
	updateLogFiles(lore->writingFileList);
	updateRecord(lore->lr.inRecordMode, INLEFT, lore->lr.delimiter);
	updateRecord(lore->rl.inRecordMode, INRIGHT, lore->rl.delimiter);
}

/*
 * method used to iterat through all of my buffers and see if they require any flushing
 * called before any of the reads occur
 */ 
/*
void flushBuffers(pigInfo *lore, fd_set *writeSet)
{
	fdNode *ted = lore->fdList.first;
	int i;
	int check;
	for(i=0; i<lore->fdList.length; i++, ted = ted->next)
	{
		if(FD_ISSET(*ted->path->outFile, writeSet) && hasMessage(ted->path)) // it has a message
		{
			#if DEBUG
				happend++;
				printfs("sending buffered message\n");
			#endif
			check = sendBuffer(ted->path, writeSet); // sending buffer
			if(check < 0)
			{
				printfs("errored when sending the buffer\n"); // XXX not handled here but latter when reading
			}
		}
	}
}
*/
/*
 * this method is the big loop, that will receive and send messages
 * also handle going into different input modes and stuff
 *
 * will always start program in command mode
 */ 
void networkLoop(pigInfo *lore)
{
	struct timeval ticktock;
	int found;
	
	addToFileSet(STDIN_FILENO, &lore->fdList.masterReadSet, &lore->fdList); // adding stdin to the file set
	// updateLogFiles(lore->writingFileList); // updates information to borders

	while(1) // its the never ending loop
	{
		#if DEBUG
			happend = 0; // debug to print when select is done
		#endif
		// changeSet will be changed with values that have information available
		fd_set changeReadSet = lore->fdList.masterReadSet;
		fd_set changeWriteSet = lore->fdList.masterWriteSet;
		
		if(bufferFlag) // if there is stuff in the buffers use a timeout value
		{
			ticktock.tv_sec = 0; // wait 1 seconds to attempt to send the message
			ticktock.tv_usec = 500;
			found = select(lore->fdList.maxFileDescriptor+1, &changeReadSet, NULL, NULL, &ticktock); 
			if(frameFlag) // what you know broke out of select for all the wrong reasons
			{
				frameFlag = 0;
				updateAllBorders(lore);
				continue; // select returns a false positive so need to call it again
			}
			ticktock.tv_sec = 0; // reseting for the write set
			ticktock.tv_usec = 0;
			found = select(lore->fdList.maxFileDescriptor+1, NULL, &changeWriteSet, NULL, &ticktock); 
			// will call select a third time but will need to since I an writing before going to the main loop
			bufferFlag = 0;
			processOutDirectionBuffers(&lore->left, &lore->fdList, &changeWriteSet);
			processOutDirectionBuffers(&lore->right, &lore->fdList, &changeWriteSet);
			changeWriteSet = lore->fdList.masterWriteSet; // so the next call will also work
		}
		else // select blocks until I have something to do
			found = select(lore->fdList.maxFileDescriptor+1, &changeReadSet, NULL, NULL, NULL);
		ticktock.tv_sec = 0; // set time to zero so I instanly obtain the write set
		ticktock.tv_usec = 0;
		found = select(lore->fdList.maxFileDescriptor+1, NULL, &changeWriteSet, NULL, &ticktock); 
			// second call is made to select to obtain the write set
			// does not wait, intly calls and returns
		// waits for input from any of the files
	
		if(found == -1) // error occured in select
		{
			fprintf(stderr, "selectf failed\n");
			exitCurses(EXIT_FAILURE); // can't really recover from this
		}
		if(frameFlag) // what you know broke out of select for all the wrong reasons
		{
			frameFlag = 0;
			updateAllBorders(lore);
			#if DEBUG
				printfs("the borders have been updated\n");
			#endif
			continue; // select returns a false positive so need to call it again
		}
		if(FD_ISSET(STDIN_FILENO, &changeReadSet)) // have somthing from stdin
			processStdin(lore, &changeWriteSet);
		// now to work on the sides
		readDirection(&lore->lr, &lore->fdList, &changeReadSet, &changeWriteSet);
		readDirection(&lore->rl, &lore->fdList, &changeReadSet, &changeWriteSet);
		// now the external filters
		readExternal(&lore->lr, &lore->fdList, &changeReadSet, &changeWriteSet);
		readExternal(&lore->rl, &lore->fdList, &changeReadSet, &changeWriteSet);
		// accept anyone who is trying to connect to my listening
		processAccept(&lore->left, &lore->fdList, &changeReadSet);
		processAccept(&lore->right, &lore->fdList, &changeReadSet);
		// read from file
		readInFile(&lore->readFd, lore->middle, &lore->fdList, &changeReadSet, &changeWriteSet);
		// read from script file
		processScript(&lore->scriptFd, lore, &changeReadSet);
	}
}

/*
 * function that gets called when the signal comes in that resizes the terminal
 * this is the SIGWINCH signal 
 * will call cursePiggy function to attempt to gather what text is on the screen and to restart the terminal session
 */ 
static void signalResize(int signo)
{
	if(signo != SIGWINCH) // these are not the erros you are looking for
		return;
	 handleResize(); // restart curses
	 frameFlag = 1; // need to update all the border information
}

int main(int argc, char *argv[])
{
	putenv("TERM=xterm"); // so that my colors show up nice a pritty
	
	startCurses(); // start using ncurses
	
	updateMiddleDir(1); // default start going to the right will change if direction gets changed
	

	// set up to catch the terminal resize event
	if(signal(SIGWINCH, signalResize) == SIG_ERR)
		printfs("cannot catch SIGWINCH signal\n");


	opterr = 0; // when calling getoptlong will not print error messeges
	#if DEBUG
		pid_t myPid = getpid();
		printfs("my  pid is %d\n", myPid);
	#endif

	pigInfo *lore = processArguments(argc, argv); // process command line arguments and puts them in struct
	
	updateBorderSideInfo(&lore->left);
	updateBorderSideInfo(&lore->right);

	updateLogFiles(lore->writingFileList); // before we begin need to update various border information
	
	networkLoop(lore); // keeps looping reading and writing messages

	return 0; // sad but will never reach this section
}

// Wow you made it to the end of the source code, I'm so proud
