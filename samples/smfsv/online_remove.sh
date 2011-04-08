#!/bin/sh
#
echo `date` Running online remove command on hostname `hostname` >> /hostfs/upgradeInstallationRemoval.log
echo Arguments: >> /hostfs/upgradeInstallationRemoval.log

ARGS=$@

for ARG in $ARGS; do
    echo $ARG >> /hostfs/upgradeInstallationRemoval.log
done

exit 0
