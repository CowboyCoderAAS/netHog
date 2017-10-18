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

# should be called to spin off the start of the pig, must be called in the same working directory of the piggy executable and stuff

mainServerName=mainHog # default name
unset TMUX # so I can have nested sessions

while getopts ":S:" opt; do # simple getopt to see the server name and such
	case $opt in
		S)
			mainServerName=$OPTARG
			foundName=1
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
caboos=".session"
serverName=$prefix$mainServerName$caboos
middle="."
sessionName=$prefix$mainServerName$middle$(hostname)$caboos # creating the path

if [ ! -e $serverName ]  # I have not created this server yet so I need to create it
then
	if [ -z $foundName ] # using default will have to supply it
	then
		tmux -S $serverName -f mainTmux.conf new-session -d -s $mainServerName ./launchHog.sh -S $mainServerName "$@"
	else # -S option was already supplied
		tmux -S $serverName -f mainTmux.conf new-session -d -s $mainServerName ./launchHog.sh "$@"
	fi
	tmux -S $serverName set-option -s set-remain-on-exit off
	tmux -S $serverName rename-window $(hostname)
fi

tmux -S $serverName attach -t $mainServerName > /dev/null # pip to null to not get the output

# now done with the program time to see if I need to remove the server sockets

tmux -S $sessionName has-session -t $(hostname) 2> /dev/null
if [ $? != 0 ] # does not have any local pigs left
then
	rm $sessionName 2> /dev/null > /dev/null
fi

tmux -S $serverName has-session -t $mainServerName 2> /dev/null
if [ $? != 0 ]  # it does not exist so there is no more piggys alive
then
	rm $serverName
fi # else I detached and will attach again

exit 0
