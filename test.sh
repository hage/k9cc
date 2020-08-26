#!/bin/bash

CC='k9cc'

assert () {
    local exfile="tmp"
    local asfile="tmp.s"
    local expected="$1"
    local input="$2"
    ./$CC "$input" > $asfile
    cc -o $exfile $asfile

    ./$exfile
    local actual="$?"

    # rm -f $exfile $asfile

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}
assert 3 'a=3; return a;'
assert 8 'a=3; z=5; return a+z;'
assert 6 'a=b=3; return a+b;'
assert 10 'return 10;'
assert 3 '1;2; return 3;'
assert 0 'return 3>=4;'
assert 1 'return 3>=3;'
assert 1 'return 3>=2;'
assert 1 'return 3>2;'
assert 0 'return -1<=-2;'
assert 1 'return -1<=-1;'
assert 1 'return -1<=5;'
assert 1 'return -1<5;'
assert 0 'return 1!=1;'
assert 1 'return -1==(-2+3)*(-1);'
assert 1 'return 1==1;'
assert 4 'return 2+2;'
assert 5 'return 1-(-4);'
assert 4 'return -(- 4);'
assert 3 'return +3;'
assert 42 'return (18 + 3)*(2 + 2) / 2;'
assert 26 'return 2* 3+4 *5;'
assert 42 'return   12 + 20 - 10 +20;'
assert 0 'return 10+20-30;'
assert 42 'return 20+22;'
assert 0 'return 0;'
assert 42 'return 42;'

wait
echo OK
