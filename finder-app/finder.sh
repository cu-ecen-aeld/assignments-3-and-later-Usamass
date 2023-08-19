#!/bin/sh
#finder script for assignment 1
#Author Osama Shafiq

set -e
set -u



if [ $# -lt 2 ]
then 
    echo "Please Specify the complete arguments ie: finder.sh /tmp/aesd/assignment1 linux"
    exit 1
else
    READDIR=$1
    SEARCHWORD=$2

   

fi

if [ -d "$READDIR" ]
then
    echo "directory ${READDIR} does exist"
else
    echo "directory does not exist"
    exit 1
fi

NUMOFFILES=$(find "${READDIR}" -type f | wc -l)
NUMOFLINE=$(grep -r "${SEARCHWORD}" "${READDIR}" | wc -l)

echo "The number of files are ${NUMOFFILES} and the number of matching lines are ${NUMOFLINE}"





