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

# called to spawn a remote pig from my piggy executible file
# works in conjuncture with tmux to acomplish this

unset TMUX # nevor forget the hours lost because I forgot this

while getopts ":S:P:W:C:" opt; do # simple getopt to see the server name and such
	case $opt in
		S)
			mainServerName=$OPTARG
			;;
		P)
			portName=$OPTARG
			;;
		W)
			workingDirectory=$OPTARG
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
prefix="session/"
endOfString=".session"
caboos=:
targetName=$mainServerName$caboos
serverName=$prefix$mainServerName$endOfString

#tmux -S $serverName set-option -s set-remain-on-exit on # XXX turn on when debuging to see what went wrong

if [ -e $serverName ] # I can launch a remote pig since I have the path to the main server
then 
	tmux -S $serverName new-window -n $sshAddress -t $targetName ssh -t -p $portName $sshAddress $workingDirectory/launchRemote.sh $@
else
	exit 2 # lets piggy process know why it died this way so it can print an appropriate error messege
fi

#tmux -S $serverName set-option -s set-remain-on-exit off # turn on when debuging


