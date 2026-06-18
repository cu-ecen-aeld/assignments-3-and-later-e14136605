#!/bin/sh

if [ $# -ne 2 ]; then
 echo "Error: Invalid number of arguments."
 echo "Usage:$0 <writefile> <writestr>"
 exit 1
fi

WRITEFILE=$1
WRITESTR=$2

DIRPATH=$(dirname "$WRITEFILE")
mkdir -p "$DIRPATH"

echo "$WRITESTR" > "$WRITEFILE"

if [ $? -ne 0 ]; then
 echo "Error: Could not create file $WRITEFILE"
 exit 1
fi

exit 0
