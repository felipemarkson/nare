set -e
# gcc -O3 -Iincludes/ -o tcp  src/nare.c src/nareTCPSvr.c examples/TCPServer.c -luring
gcc -O3 -Iincludes/ -o nare_eg_simple src/nare.c examples/simple.c      -luring
