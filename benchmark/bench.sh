#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "Error! need two paras"
    echo "Usage: <number>  <length>"
    exit 1
fi

cargo run --release -- --address "127.0.0.1:8000" --number $1 --duration 30 --length $2