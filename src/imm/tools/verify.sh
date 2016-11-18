#!/bin/bash

###
### Only intended as a temporary checkin (until the the single imm.xml is removed rfom repository)
###



if [ $# -ne 1 ] ; then
   echo "usage: $0 old_immxml_file"
   exit 1
fi

EXISTING_IMM_FILE=$1
RESULT_IMM_FILE=../../../../../etc/opensaf/imm.xml

if [ ! -f $1 ] ; then
   echo "The file $1 cannot be found"
   exit 1
fi

if [ ! -f $2 ] ; then
   echo "The file $2 cannot be found"
   exit 1
fi

rm -rf ./verify
mkdir ./verify


echo -e "\n\n________________________________________________________"
echo -e "\n\n Step3 (i.e. only for testing this procedure) compare the generated imm.xml with the old imm.xml ($EXISTING_IMM_FILE) (the generated file may have differences due to that it is more consistent): \n"

# sort original file (to be able to diff with generated file)
echo "Sort original imm.xml file as reference (needed for comparision)"
./immxml-merge --sort $EXISTING_IMM_FILE > verify/imm_xml_orig_sort.xml

# diff the (sorted) generated file with (sorted) original file
echo "diff the final imm.xml file with a (sorted) original file"
echo "<press ENTER>"
read ANSW
diff $RESULT_IMM_FILE verify/imm_xml_orig_sort.xml |more

