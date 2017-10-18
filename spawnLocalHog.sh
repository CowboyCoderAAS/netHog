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

# used to spawn a new local pig

while getopts ":S:" opt; do # simple getopt to see the server name and such
	case $opt in
		S)
			mainServerName=$OPTARG
			;;
		\?)
			;;
		:)
			echo "Option -$OPTARG requires an argument."
			exit 1
			;;
	esac
done

unset TMUX
prefix="session/"
middle="."
caboos=".session"
sessionName=$prefix$mainServerName$middle$(hostname)$caboos

tmux -S $sessionName new-window ./netHog "$@"

tmux -S $sessionName has-session -t $(hostname) 2> /dev/null
if [ $? != 0 ] # does not have any local pigs left
then
	rm $sessionName
fi

