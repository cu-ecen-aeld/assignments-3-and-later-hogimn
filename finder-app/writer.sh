#!/bin/sh

# $1(writefile): full path to a file (including filename) on the filesystem
# $2(writestr): text string which will be written within this file
# Exits with value 1 error and print statements if any of the arguments above
# were not specified
if [ $# -ne 2 ]; then
    echo "Error: Two arguments are required: writefile and writestr"
    exit 1
fi

# Creates a new file with name and path writefile with content writestr,
# overwriting any existing file and creating the path if it doesn’t exist.
# Exits with value 1 and error print statement if the file could not be created.
mkdir -p $(dirname $1)
touch $1
if [ ! -e $1 ]; then
    echo "Error: Could not create file $1"
    exit 1
fi

printf $2 > $1
