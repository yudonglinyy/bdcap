#!/bin/bash

if [ $# != 2  ]; then
    echo "Usage: $0 ip port "
    exit 1
fi

while [[ 1 ]]; do
    /bdcapdir/client $1 $2
    sleep 2
done

