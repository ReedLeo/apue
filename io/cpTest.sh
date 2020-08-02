#!/bin/bash
GR=1024*1024
MAX_BUF_SIZE=1024*4*$GR
for ((bufsize=1024*$GR; bufsize < $MAX_BUF_SIZE; bufsize*=2))
do
    echo "Testing Buffer size = $((bufsize/GR)) MB" | tee -a "/tmp/mycptest"
    (time ./mycopy /home/leo/Downloads/src /tmp/out $bufsize) 2>>/tmp/mycptest
    # (time cp /home/leo/Downloads/src /tmp/out) 2>>/tmp/cptest
    echo "End of testing buffer size = $((bufsize/GR)) MB\n" | tee -a "/tmp/mycptest"
done
