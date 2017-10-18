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

#used to detach local detach global or force quit all of the piggy programs

# takes in several arguments to tell what to do
# really only called by piggy itself
# assumes this is called in the same working directory as the executable and such

while getopts ":S:Q:" opt; do # simple getopt to see the server name and such
	case $opt in
		S)
			mainServerName=$OPTARG
			;;
		Q)
			quitOption=$OPTARG
			;;
		\?)
			;;
		:)
			echo "Option -$OPTARG requires an argument."
			exit 1
			;;
	esac
done
# getting the right names and such
prefix="session/"
middle="."
caboos=".session"
sessionName=$prefix$mainServerName$middle$(hostname)$caboos
serverName=$prefix$mainServerName$caboos

# TODO add a local kill option to only kill locally

if [ $quitOption == ":q!" ] # force quit everything is killed XXX curently does not work
then
	for f in session/$mainServerName*.session; do
		if [ $f != $sessionName ] # so I don't kill myself before everyone else
		then
			tmux -S $f kill-server
			rm $f
		fi
	done
	tmux -S $sessionName kill-server
	rm $sessionName
elif [ $quitOption == ":d" ] # local detach
then
	tmux -S $serverName detach-client -s $(hostname) # detaches the client
elif [ $quitOption == ":d!" ] # global detach
then
	tmux -S $serverName detach-client -s $mainServerName
else # did not give one of the valid 3 options
	>&2 echo did not give me a valid option $quitOption # should not come to this
fi
