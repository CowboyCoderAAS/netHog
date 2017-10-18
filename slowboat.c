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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#define BUFFSIZE 300
#define LOOP 1000
int main ()
{
	//char inBuffer[101];
	//char outBuffer[101];
	setbuf(stdout, NULL); // set the buffering to unbuffered
	setbuf(stdin, NULL);
	char buffer[BUFFSIZE+1];
	buffer[BUFFSIZE+1] = '\0';
	int i=0;
	int length;
	
  while (1)
    {
	  length = read(STDIN_FILENO, buffer, BUFFSIZE);
	  buffer[length] = '\0';
      printf("%s", buffer);
	  if(i<LOOP) // so that it will unflush the buffer as fast as posible afterwards
	  {
    	  sleep(1);
		  i++;
	  }
    }
}
