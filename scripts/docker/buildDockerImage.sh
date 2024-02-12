#!/bin/bash

docker build -t ttk --build-arg paraview=5.11.0 --build-arg ttk=dev .
