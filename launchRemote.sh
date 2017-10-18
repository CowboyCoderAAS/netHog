#!/bin/bash

# MIT License
# 
# Copyright (c) 2017 CowboyCoderAAS
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# called by spawnRemotePig to launch the ssh on the new window and such

unset TMUX # nevor forget the hours lost because I forgot this
echo start of program
echo start of program

for word in $@; do echo $word; done

while getopts ":P:S:W:C:" opt "$@"; do # simple getopt to see the server name and such
	case $opt in
		S)
			mainServerName=$OPTARG
			echo got serverName $mainServerName
			;;
		W)
			workingDirectory=$OPTARG
			echo got working directory $workingDirectory
			;;
		P)
			sshPort=$OPTARG
			;;
		C)
			sshAddress=$OPTARG
			;;
		\?)
			;;
		:)
			echo "Option -$OPTARG requires an argument."
			exit 1
			;;
	esac
done

echo mainServerName is $mainServerName

prefix="session/"
middle="."
caboos=".session"

cd $workingDirectory
theHost=$(hostname)

serverName=$prefix$mainServerName$caboos
sessionName=$prefix$mainServerName$middle$theHost$caboos

#if [ -e $serverName ] # only will rename the window if
#then
	#echo going to renaim window
	#tmux -S $serverName rename-window $theHost
	#echo finished renaming window
#fi

if [ ! -e $sessionName ]
then # I am not attaching will create a new one
	echo attempting to create Hog
	tmux -S $sessionName -f localTmux.conf new-session -s $theHost ./netHog "$@"
	#tmux -S $serverName set-option -s set-remain-on-exit on
else
	resultCount=$(tmux -S $sessionName list-sessions | grep $theHost | grep -c attached) 
	if [ resultCount == 0 ] # checking if it is already attached, if not will attach to it
	then
		tmux -S $sessionName attach -t $theHost
		#tmux -S $serverName set-option -s set-remain-on-exit on
	#else # going have to kill myself as in the window I just created
		#tmux -S $serverName kill-window # goodby world # XXX should not need to kill since it will be done when it returns
	fi
fi

tmux -S $sessionName has-session -t $(hostname) 2> /dev/null
if [ $? != 0 ] # does not have any local pigs left
then
	rm $sessionName
fi


