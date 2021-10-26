
<a href="https://scan.coverity.com/projects/rodan-iotcpd">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/23942/badge.svg"/>
</a>

## iotcpd

a stdio to tcp redirector daemon

```
 source:       https://github.com/rodan/iotcpd
 author:       Petre Rodan <2b4eda@subdimension.ro>
 author:       Cristian Sandu - pipe handling, a shoulder to cry on
 license:      BSD
```

### Description

iotcpd is a generic STDIO to TCP redirector daemon. it can start multiple identical non-networking enabled daemons that only interact via stdin/stdout and query them based on requests received via TCP. the request is sent to the stdin of one of the daemons that is available and the reply from its stdout is then sent to the requester via TCP. the connection is then torn down. basically an inetd/xinetd/tcpserver on crack.

epoll API functions are used to handle incoming connections.

***SYNOPSIS***

```
iotcpd  [-hv]  [-d,  --daemon  NAME ] [-i, --ipv4 IP ] [-I, --ipv6 IP ]
  [-p, --port NUM ] [-n, --num-daemons NUM ] [-b,  --busy-timeout  NUM  ]
  [-a, --alarm-interval NUM ]
```

```
example of how this fits in with having multiple squidguard daemons that can talk via TCP:

          ---------------------      ---- squidguard
         |                     |    |
---------+                     +----+---- squidguard
  TCP    |       iotcpd server |    |
          ---------------------      ...
                                    |
                                     ---- squidguard
```
```
 $ ./iotcpd --num-daemons 8 --daemon "squidGuard -c sg/adblock.conf" \
 --ipv4 10.20.30.40 --port 1234
```

### Build requirements

 dependencies include an epoll aware OS (Linux >=2.6, FreeBSD) and optionally >=net-proxy/squidguard-1.5 and net-analyzer/netcat6 (from http://netcat6.sourceforge.net/) for testing.

### Build and install

if you're using gentoo, a portage overlay is provided. a simple

```
emerge iotcpd
```

will compile and install the application.

for any other distribution, you can use the following commands:

```
cd ./server
make
install -m 755 ./iotcpd /usr/sbin/
install -m 644 ../doc/iotcpd.1 /usr/share/man/man1/
```

### Usage

a manual is provided
```
man ./doc/iotcpd.1
```

### Testing

```
 $ iotcpd --num-daemons 8

  # in a different terminal
 $ bash test_adblock.sh a
 $ bash test_adblock.sh b
 $ bash test_adblock.sh c
```

the code itself is static-scanned by [llvm's scan-build](https://clang-analyzer.llvm.org/), [cppcheck](http://cppcheck.net/) and [coverity](https://scan.coverity.com/projects/rodan-cwiticald?tab=overview). Dynamic memory allocation in the PC applications is checked with [valgrind](https://valgrind.org/).

sending USR1 and USR2 signals will provide with detailed statistics on the innerworkings of the redirector service:

```
 $ kill -USR1 `pidof iotcpd`

 --- statistics ---- >8 -------
queries total      855997
queries replied    855992
queries delayed    24740
queries timeout    1085
queries failed     5
queries 0-100      657411
queries 100-250    179397
queries 250-500    16034
queries 500-750    1810
queries 750-1000   492
queries 1000-      343
daemon spawns      741
daemon S_AVAILABLE 27
daemon S_BUSY      5
daemon S_SPAWNING  0
daemon S_STARTING  0
uptime   7916
 --- statistics ---- 8< -------

```

 queries x-y represent a histogram of the replies based on the time their processing took (in miliseconds).


