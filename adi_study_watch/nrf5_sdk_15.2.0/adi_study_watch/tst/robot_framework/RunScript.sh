#! /bin/sh.

a=0

while [ $a -lt 10 ]
do
   echo $a
   robot -v DUT_COMPORT:COM5 --timestampoutputs --outputdir ./log m2m2_tests.txt
   a=`expr $a + 1`
done
