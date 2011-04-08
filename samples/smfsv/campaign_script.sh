#!/bin/sh
#
echo `date` Running the campaign script `hostname` >> /hostfs/campaignScript.log
echo Arguments: >> /hostfs/campaignScript.log

ARGS=$@

for ARG in $ARGS; do
    echo $ARG >> /hostfs/campaignScript.log
done

exit 0
