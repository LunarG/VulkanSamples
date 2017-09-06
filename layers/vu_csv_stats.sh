#! /bin/bash
# Basic count stats from the VU database in CSV format for spreadsheet loading.
# Usage: ./vu_csv_stats.sh > stats.csv

set -o errexit
set -o nounset

COUNT="grep -c"

INFILE="./vk_validation_error_database.txt"

if [ ! -r "$INFILE" ]
then
    echo "ERROR: \"$INFILE\" is not readable." >&2
    exit 1
fi

echo "\"Generated\",\"`date`\""
echo "\"Directory\",\"`pwd -P`\""
echo "\"Commit\",\"$(git describe --all --long)\""
echo
echo "\"All VUs\""
echo "\"\",\"Total\",$($COUNT '^VALIDATION_ERROR_' $INFILE)"
echo "\"\",\"Done (Y)\",$($COUNT '~^~Y~^~' $INFILE)"
echo "\"\",\"Not done (N)\",$($COUNT '~^~N~^~' $INFILE)"
echo "\"\",\"Unknown (U)\",$($COUNT '~^~U~^~' $INFILE)"
echo
echo "\"Implicit VUs\""
echo "\"\",\"Total\",$($COUNT 'implicit' $INFILE)"
echo "\"\",\"Done (Y)\",$(grep 'implicit' $INFILE | $COUNT '~^~Y~^~')"
echo "\"\",\"Not done (N)\",$(grep 'implicit' $INFILE | $COUNT '~^~N~^~')"
echo
echo "\"Tests\""
echo "\"\",\"None\",$($COUNT '~^~None~^~' $INFILE)"
echo "\"\",\"Unknown\",$($COUNT '~^~Unknown~^~' $INFILE)"
echo "\"\",\"NotTestable\",$($COUNT '~^~NotTestable~^~' $INFILE)"

# vim: set sw=4 ts=8 et ic ai:
