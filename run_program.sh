#!/bin/bash

echo "Compiling..."
gcc -o meuPrograma main.c discovery_service.c management_service.c monitoring_service.c user_interface.c wakeonlan.c election_service.c synchronization_service.c -lpthread -g

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo "Running program"
    ./meuPrograma
    exit 0
else
    echo "Compilation failed."
    exit 1
fi
