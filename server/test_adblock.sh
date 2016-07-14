#!/bin/bash

if [ "$1" == "a" ]; then
    for ((i=0;i<40;i++)); do
        echo 'http://www.wowomg.com/foo.html 127.0.1.1/10.129.23.1 - GET myip=- myport=3127' | nc 127.0.0.1 9991 2>/dev/null &
    done
elif [ "$1" == "b" ]; then
   for ((i=0;i<40;i++)); do
        echo "http://clean_url/baz/${RANDOM}-${i} 127.0.1.1/10.129.23.1 - GET myip=- myport=3127" | nc 127.0.0.1 9991 2>/dev/null &
    done
elif [ "$1" == "c" ]; then
   for ((i=0;i<40;i++)); do
        echo "http://bar.baaz.foo/test.html 127.0.1.1/10.129.23.1 - GET myip=- myport=3127" | nc 127.0.0.1 9991 2>/dev/null &
    done
fi


#echo 'http://d1fjasmrhc6q3w.cloudfront.net/testImages/harmless.jpg 127.0.1.1/10.129.23.1 - GET myip=- myport=3127' | nc 127.0.0.1 9991

sleep 3
killall -USR1 url_checkd
killall -USR2 url_checkd
ps axww | grep defunct | grep -Ev '(grep)|(nacl_helper)|(chrome)'
ps axww | grep nc | grep -Ev '(grep)'

