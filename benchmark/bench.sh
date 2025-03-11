#!/bin/bash
if [ "$#" -ne 2 ]; then
    echo "Error! need two paras"
    echo "Usage: <number>  <length>"
    exit 1
fi

cargo run --release -- --address "127.0.0.1:12345" --number $1 --duration 60 --length $2