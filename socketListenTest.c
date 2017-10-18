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
 * going to write a simple test that will create a socket make it listen on port 36758
 * and then accept a connection
 * there it will perform various tests on that socket buffer capabilityes
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

void waitUntilEndOfWorld()
{
	fprintf(stderr, "going to wait now\n");
	while(true)
	{
		sleep(UINT_MAX);
	}
}

// going to listen on 36758 and accept connection returning the fd of that connection
int listenAndConnect()
{
	// time to remember how the hecks to call listen and stuff
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int flag = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
	
	struct sockaddr_in localTable;
	memset((char *) &localTable, 0, sizeof(struct sockaddr_in)); // clearing memory
	localTable.sin_family = AF_INET;
	localTable.sin_addr.s_addr = INADDR_ANY;
	localTable.sin_port = htons((u_short) 36758);
	bind(sock, (struct sockaddr *) &localTable, sizeof(struct sockaddr_in));
	listen(sock, 6);
	struct sockaddr_in senderTable;
	memset((char *) &senderTable, 0, sizeof(struct sockaddr_in)); // scrubbing memory
	// calling accept
	unsigned senderAddressLength = sizeof(struct sockaddr_in); // used for accept method call
	fprintf(stderr, "going to wait and accept now\n");
	int connectSock = accept(sock, (struct sockaddr *) &senderTable, &senderAddressLength);
	if(connectSock < 0)
	{
		perror("connection faild: ");
		exit(1);
	}
	fprintf(stderr, "connection has been made\n");
	return connectSock;
}

/*
 * will test the socket information by using fstat on it
 */
void testSocketInfo(int sock)
{
	char temp[100];
	struct stat willy;
	if(fstat(sock, &willy)<0)
	{
		perror("something bad in the neborhood ");
		return;
	}
	fprintf(stderr, "original size of offset %ld\n", willy.st_size);

	fprintf(stderr, "going to wait for message to be sent\n");
	fgets(temp, 100, stdin);
	fstat(sock, &willy);
	fprintf(stderr, "next size of offset %ld\n", willy.st_size);
}
int main(int argc, char *argv[]) 
{
	pid_t myPid = getpid();
	fprintf(stderr, "my pid is %d\n", myPid);

	// time to remember how the hecks to call listen and stuff
	int sock = listenAndConnect();
	testSocketInfo(sock);

	waitUntilEndOfWorld();
	return 0;
}
