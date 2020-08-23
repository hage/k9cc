#!/bin/bash

CC='k9cc'

assert () {
    local exfile=`tempfile`
    local asfile=`tempfile --suffix .s --prefix k9cc`
    local expected="$1"
    local input="$2"
    ./$CC "$input" > $asfile
    cc -o $exfile $asfile

    $exfile
    local actual="$?"

    rm -f $exfile $asfile

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}
# assert 13 'a = 2;
# b = 5 * 6 - 8; a+b / 2'
assert 0 '3>=4' &
assert 1 '3>=3' &
assert 1 '3>=2' &
assert 1 '3>2' &
assert 0 '-1<=-2' &
assert 1 '-1<=-1' &
assert 1 '-1<=5' &
assert 1 '-1<5' &
assert 0 '1!=1' &
assert 1 '-1==(-2+3)*(-1)' &
assert 1 '1==1' &
assert 4 2+2 &
assert 5 '1-(-4)' &
assert 4 '-(- 4)' &
assert 3 '+3' &
assert 42 '(18 + 3)*(2 + 2) / 2' &
assert 26 '2* 3+4 *5' &
assert 42 '  12 + 20 - 10 +20' &
assert 0 10+20-30 &
assert 42 20+22 &
assert 0 0 &
assert 42 42 &

wait
echo OK
