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

/* the purpose of this is to determin if the functions even work 
 * creates a process and just waits
 * will only run for the maximun unsinged number but should be long enough for testing
 * does not read stdin or out
 */

#include <unistd.h>
#include <limits.h>
#include <stdio_ext.h>
#include <stdio.h>
#include <errno.h>
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
#include <sys/stat.h>
#include <sys/un.h>

/*
 * shows how different buffering occurs
 */ 
void differentBufferTest()
{
	// going to attemt to see what the stdout says
	FILE* f = fdopen(1, "w");
	if(f!=stdout)
	{
		fprintf(stderr, "the file descriptor f is not the same as stdout\n");
	}
	//FILE *f = stdout;
	char *buffy = (char *) malloc(BUFSIZ);
	setvbuf(f, buffy, _IOFBF, BUFSIZ);
	if(f==NULL)
		fprintf(stderr, "the file is NULL\n");
	size_t amount = __fpending(f);
	if(errno!=0)
	{	
		fprintf(stderr, "errno was set and was %s\n", strerror(errno));
	}
	fprintf(stderr, "right now in the buffer is %ld\n", amount);
	size_t bufResult = __fbufsize(f);
	fprintf(stderr, "overal buff size was %ld\n", bufResult);
	if(errno!=0) 
	{
		fprintf(stderr, "errno was set and was %s\n", strerror(errno));
	}
	char *casey = (char *) malloc(BUFSIZ);
	setvbuf(stdout, casey, _IOFBF, BUFSIZ);
	fprintf(f, "hello world1\n");
	amount = __fpending(f);
	fprintf(stderr, "new ammount pending for f is %ld\n", amount);
	fprintf(stdout, "hello world2\n");
	amount = __fpending(stdout);
	fprintf(stderr, "new amount that is pending for stdout is %ld\n", amount);
	fflush(stdout);
	fflush(f);
}

/*
 * going to test pipe calls and see if I can use fstat 
 */ 
void pipeTest()
{
	int smoker[2];
	pipe(smoker);
	fprintf(stderr, "file descriptro [0]=%d [1]=%d\n", smoker[0], smoker[1]);

	struct stat willy;
	struct stat casey;
	if(fstat(smoker[1], &willy)<0)
	{
		perror("something went down in the neborhood: ");
		return;
	}
	fprintf(stderr, "the offsetSize is %ld\n", willy.st_size);
	char buffy[100] = "hello world\n\0";
	write(smoker[1], buffy, strlen(buffy));
	fstat(smoker[1], &willy);
	fstat(smoker[0], &casey);
	fprintf(stderr, "the new offsiet size is %ld\n", willy.st_size);
	fprintf(stderr, "the offset on the read is %ld\n", casey.st_size);
	/*
	char luffy[100];
	ssize_t amount = read(smoker[1], luffy, 100);
	fprintf(stderr, "the amount read from the write file descriptor was %ld\n", amount);
	if(amount<0)
	{
		perror("well that did not work ");
		FILE *f = fdopen(smoker[1], "r"); // will it work
		if(f==NULL)
		{
			perror("getting the file pointer did not: ");
		}
		else // lets see what I can do now
		{
			size_t reading = fread(luffy, strlen(buffy), 1, f);
			fprintf(stderr, "I was able to read %ld bytes from the file and was %s", reading, luffy);
		}
	}
	*/
}

/*
 * messing around with various stuff in the /proc/[pid]/maps
 * going to see if I can obtain the location in memory were the pipe call allocated memory for the kernal to work with
 * and weather or not I am able to access it
 */ 
void mappedMemoryTest()
{
	int smoker[2];
	char temp[100];
	fprintf(stderr, "going to wait for user input before calling pipe\n");
	fgets(temp, 100, stdin);
	pipe(smoker);
	fprintf(stderr, "going to wait before attempting to write to the pipe\n");
	fgets(temp, 100, stdin);
	char luffy[100] = "hello world yalls\n\0";
	write(smoker[1], luffy, strlen(luffy));
}

/*
 * I want to see if I get partial writes when I keep sending information to a unix domain dgram socket
 */ 
void unixDatagramTest()
{
	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(sock < 0)
	{
		perror("socket failed ");
		return;
	}
	int sockListen = socket(AF_UNIX, SOCK_DGRAM, 0);
	struct sockaddr_un localTable;
	struct sockaddr_un remoteTable;
	memset(&localTable, 0, sizeof(localTable));
	memset(&remoteTable, 0, sizeof(remoteTable));
	localTable.sun_family = AF_UNIX;
	remoteTable.sun_family = AF_UNIX;
	strcpy(localTable.sun_path, "unixDgramLocal.sock");
	strcpy(remoteTable.sun_path, "unixDgramRemote.sock");
	int check = bind(sock, (struct sockaddr *) &localTable, sizeof(localTable));
	if(check < 0)
	{
		perror("bind failed for local");
		return;
	}
	check = bind(sockListen, (struct sockaddr *) &remoteTable, sizeof(remoteTable));
	if(check < 0)
	{
		perror("bind failed for the remote ");
	}
	fprintf(stderr, "made the socket and bind it\n");
	/*if(connect(sock, (struct sockaddr *) &remoteTable, sizeof(remoteTable))<0)
	{
		perror("connect failed ");
		return;
	}
	fprintf(stderr, "connected the socket now\n");*/
	size_t buffsize = 1000;
	int startPlace = 707;
	char buff[buffsize];
	memset(buff, 0, buffsize);
	memset(buff, 'a', startPlace);
	size_t writeAmount;
	int count = 0;

	fd_set masterSet;
	FD_SET(sock, &masterSet);
	struct timeval tick;
	int place = startPlace;
	char value = ' ';
	while(1) // keep writing to the socket until it gets a partial write
	{
		tick.tv_sec = 0;
		tick.tv_usec = 500;
		buff[place] = value;
		value++;
		if(value == 126)
		{
			place++;
			value = ' ';
		}
		fd_set changeSet = masterSet;
		int found = select(sock+1, NULL, &changeSet, NULL, &tick);
		if(found == -1)
		{
			perror("select failed ");
			return;
		} if(FD_ISSET(sock, &changeSet))
		{
			writeAmount = sendto(sock, buff, strlen(buff), 0, (struct sockaddr *) &remoteTable, sizeof(remoteTable));
			count++;
			if(writeAmount != strlen(buff))
			{
				fprintf(stderr, "The final write amount was %ld\n", writeAmount);
				fprintf(stderr, "Total number of writes %d\n", count);
				return;
			}
		} else // the writing is done
		{
			fprintf(stderr, "Finished writing the the socket with total number of %d\n", count);
			break;
		}
	}
	// going to start calling read from the socklisten
	char casey[buffsize];
	char compare[buffsize];
	memset(compare, 0, buffsize);
	memset(compare, 'a', startPlace);
	place = startPlace;
	value = ' ';
	FD_ZERO(&masterSet);
	FD_SET(sockListen, &masterSet);
	while(1)
	{
		tick.tv_sec = 0;
		tick.tv_usec = 500;
		compare[place] = value;
		value++;
		if(value == 126)
		{
			place++;
			value = ' ';
		}
		fd_set changeSet = masterSet;
		int found = select(sockListen+1, &changeSet, NULL, NULL, &tick);
		if(found == -1)
		{
			perror("select failed ");
			return;
		} if(FD_ISSET(sockListen, &changeSet))
		{
			socklen_t theLength = (socklen_t) sizeof(localTable);
			writeAmount = recvfrom(sockListen, casey, buffsize, 0, (struct sockaddr *) &localTable, &theLength);
			if(strcmp(casey, compare)) // they are not equal to each other
			{
				fprintf(stderr, "the two strings are not equal, should be %s but is %s\n", compare, casey);
			}
		} else
		{
			fprintf(stderr, "Finished reading from the socket\n");
			fprintf(stderr, "the final read was %s\n", casey);
			break;
		}
	}

	close(sock);
	close(sockListen);
}

int main(int argc, char *argv[]) 
{
	pid_t myPid = getpid();
	fprintf(stderr, "my pid is %d\n", myPid);

	// differentBufferTest();
	//pipeTest();
	//mappedMemoryTest();
	//unixDatagramTest();
	fprintf(stderr, "going to wait\n");
	while(1) // going to take a realllly long nap
	{
		sleep(UINT_MAX);
	}
	return 0;
}
