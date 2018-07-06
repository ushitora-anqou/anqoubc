#!/bin/sh

test_anqoubc() {
    tempasm=`mktemp --suffix=.s`
    tempout=`mktemp --suffix=.o`
    tempres=`mktemp --suffix=.dat`

    ./anqoubc $1 $tempasm
    gcc $tempasm -no-pie -o $tempout
    $tempout > $tempres
    #paste -d "=" $tempres $2 | sed 's/=/==/g' - | bc 2> /dev/null | grep -n 0 | cut -f 1 -d ":" | awk "{print \"ERROR $1 L.\" \$1 }"
    diff $tempres $2
    if [ $? -eq 1 ]; then
        echo "ERROR: $1"
    fi

    rm $tempasm
    rm $tempout
    rm $tempres
}

seq -f "%02.f" 1 10 | while read i; do
    test_anqoubc "test/compile_$i.in" "test/compile_$i.out"
done

