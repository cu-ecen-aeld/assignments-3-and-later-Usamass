#!/bin/bash

if [ $# -lt 2 ]
then 
    echo "Error: Incomplete arguments!"
    exit 1
else
    NAMEOFFILE=$1
    STRTOWRITE=$2
fi

mkdir -p "$(dirname "$NAMEOFFILE")"

echo "$STRTOWRITE" > "$NAMEOFFILE" 

if [ $? -ne 0 ]
then
    echo "Error: Could not create or write to the file."
    exit 1
else 
    echo "file is created"

fi

exit 0
