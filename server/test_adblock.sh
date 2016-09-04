#!/bin/bash

count=1

if [ "$1" == "a" ]; then
    for ((i=0;i<${count};i++)); do
        echo 'http://www.wowomg.com/foo.html 127.0.1.1/10.129.23.1 - GET myip=- myport=3127' | nc 10.10.10.2 9991 2>/dev/null &
    done
elif [ "$1" == "b" ]; then
   for ((i=0;i<${count};i++)); do
        echo "http://clean_url/baz/${RANDOM}-${i} 127.0.1.1/10.129.23.1 - GET myip=- myport=3127" | nc 10.10.10.2 9991 2>/dev/null &
    done
elif [ "$1" == "c" ]; then
   for ((i=0;i<${count};i++)); do
        echo "http://bar.baaz.foo/test.html 127.0.1.1/10.129.23.1 - GET myip=- myport=3127" | nc 10.10.10.2 9991 2>/dev/null &
    done
elif [ "$1" == "d" ]; then
    for ((i=0;i<${count};i++)); do
        echo "http://d1fjasmrhc6q3w.cloudfront.net/testImages/testghostie_ad.jpg 127.0.1.1/10.129.23.1 - GET myip=- myport=3127" | nc 10.10.10.2 9991 2>/dev/null &
    done
    exit 0
fi

#echo 'http://d1fjasmrhc6q3w.cloudfront.net/testImages/harmless.jpg 127.0.1.1/10.129.23.1 - GET myip=- myport=3127' | nc 127.0.0.1 9991
echo
exit 0

sleep 3
killall -USR1 iotcpd
killall -USR2 iotcpd
ps axww | grep defunct | grep -Ev '(grep)|(nacl_helper)|(chrome)'
ps axww | grep 'nc 127.0.0.1' | grep -Ev '(grep)'

