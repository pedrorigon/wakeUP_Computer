#!/bin/bash

echo "Generating protocol buffers..."

python3 ./nanopb/generator/nanopb_generator.py messages.proto
if [ $? -eq 0 ]; then
    echo "Protobufs generated successfully"
else
    echo "Protobuf generation failed."
    echo "Check out if required software is installed using"
    echo "sudo apt install python3 python3-pip"
    echo "pip3 install protobuf grpcio-tools"
    exit 1
fi

echo "Compiling..."
gcc -o meuPrograma main.c discovery_service.c management_service.c monitoring_service.c messages.pb.c nanopb/*.c -lpthread -I nanopb -lpthread

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo "Running program with parameters: $@"
    ./meuPrograma $@
    exit 0
else
    echo "Compilation failed."
    exit 1
fi
