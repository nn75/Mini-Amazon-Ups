To make the file：

make -f Makefile.nn
make -f Makefile.nn debug


To run：

LD_PRELOAD=/home/nn75/protobuf-3.7.1/lib/libprotobuf.so.18 ./test
LD_PRELOAD=/home/nn75/protobuf-3.7.1/lib/libprotobuf.so.18 ./debug

To compile proto:
/home/nn75/protobuf-3.7.1/bin/protoc -I=. --cpp_out=. ./amazon_ups.proto


To reset the world:
docker system prune -a

World simulater on the github:

https://github.com/yunjingliu96/world_simulator_exec


Google protocal buffer tutorial:

https://developers.google.com/protocol-buffers/docs/cpptutorial


To test server running:
nc -v 127.0.0.1 45678