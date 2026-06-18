#!/bin/sh

if [ $# -ne 2 ]; then
 echo "Error: Invalid number of arguments."
 echo "Usage: $0 <filesdir> <searchstr>"
 exit 1
fi

FILESDIR=$1
SEARCHSTR=$2

if [ ! -d "$FILESDIR" ]; then
 echo "Error: Directory $FILESDIR does not exist."
 exit 1
fi

X=$(find "$FILESDIR" -type f | wc -l)

Y=$(grep -r "$SEARCHSTR" "$FILESDIR"| wc -l)

echo "The number of files are $X and the number of matching lines are $Y"

exit 0
