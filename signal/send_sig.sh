#!/bin/bash

function print_usage() 
{
    echo "Usage: $0 recv-pid repeat-times signum1 [signum2]";
}

if [[ $# -lt 3 ]]; then
    print_usage   
else
   pid=$1
   rep_cnt=$2
   sig1=$3
   for ((i=0; i<$rep_cnt; ++i)) {
        #echo "$i: $pid -$sig1";  
        kill -$sig1 $pid
   }

   if [[  $# -ge 4 ]]; then
        sig2=$4
        kill -$sig2 $pid
   fi
fi
