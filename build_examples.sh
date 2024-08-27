set -e
gcc -O3 -Iincludes/ -o tcp  src/nare.c src/nareTCPSvr.c examples/TCPServer.c -luring
gcc -O3 -Iincludes/ -o open src/nare.c src/nareTCPSvr.c examples/open.c      -luring
