#!/bin/bash

docker buildx prune
docker builder prune -af
docker build -t ttk --build-arg paraview=5.11.0 --build-arg ttk=dev --progress=plain . 2> /tmp/docker-log.txt
#docker build -t ttk --progress=plain . 2> /tmp/docker-log.txt
