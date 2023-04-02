#!/bin/sh

# $1(=filesdir): path to a directory on the filesystem
# $2(=searchstr): text string which will be searched within these files
# Exits with return value 1 error and print statements if any of
# the parameters above were not specified
if [ $# -ne 2 ]; then
    echo "Usage: $0 filesdir searchstr"
    exit 1
fi

# Exits with return value 1 error and print statements if filesdir
# does not represent a directory on the filesystem
if [ ! -d "$1" ]; then
    echo "$1 is not a directory"
    exit 1
fi

# Prints a message
# "The number of files are X and the number of matching lines are Y"
# where X is the number of files in the directory and all subdirectories
# and Y is the number of matching lines found in respective files, where
# a matching line refers to a line which contains searchstr (and may also
# contain additional content).
# Search for matching lines and count the results
numOfMatchingFiles=$(find "$1" -type f | wc -l)
numOfMatchingLines=$(grep -r "$2" "$1" | wc -l)

# Print the results
echo "The number of files are $numOfMatchingFiles and the number of matching lines are $numOfMatchingLines"
