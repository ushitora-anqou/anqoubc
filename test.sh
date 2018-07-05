#!/bin/sh

test_anqoubc() {
    tempasm=`mktemp --suffix=.s`
    tempout=`mktemp --suffix=.o`
    tempres=`mktemp --suffix=.dat`

    ./anqoubc $1 $tempasm
    gcc $tempasm -no-pie -o $tempout
    $tempout > $tempres
    paste -d "=" $tempres $2 | sed 's/=/==/g' - | bc 2> /dev/null | grep -n 0 | cut -f 1 -d ":" | awk "{print \"ERROR $1 L.\" \$1 }"

    rm $tempasm
    rm $tempout
    rm $tempres
}

seq -f "%02.f" 1 2 | while read i; do
    test_anqoubc "test/compile_add_$i.in" "test/compile_add_$i.out"
done

