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

assert_exsrc() {
    local exfile="tmp"
    local asfile="tmp.s"
    local expected="$1"
    local exsrc="$2"
    local input="$3"

    ./$CC "$input" > $asfile
    cc -o $exfile $asfile $exsrc

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
assert_exsrc 6 test/add2.c 'return return6th(1,2,3,4,5,6);'
assert_exsrc 1 test/add2.c 'return return1st(1,2,3,4,5,6);'
assert_exsrc 231 test/add2.c 'return add_each2_and_multiply(1,2,3,4,5,6);'
assert_exsrc 42 test/add2.c 'return add2(add2(10, 30), 2);'
assert_exsrc 42 test/value40.c 'return value40() + 2;'

assert 55 'for(sum=i=0;i<11;i=i+1){b=i;sum=sum+b;}return sum;'
assert 55 'sum = 0; for(i=0;i<11;i=i+1){sum=sum+i;}return sum;'
assert 24 'for(;0;)return 42;return 24;'
assert 42 'for(;1;)return 42;return 24;'
assert 10 'i=0;for(;i<10;)i=i+1;return i;'
assert 55 'sum=0;for(i=0;i<11;i=i+1)sum=sum+i;return sum;'
assert 42 'a=0;while(a<10)if(a==5)return 42;else a=a+1;'
assert 11 'a=0;while(a<=10)a=a+1;return a;'
assert 10 'if (1) return 10; else return 0;'
assert 10 'if (0) return 100; else return 10;'
assert 20 'if (0) return 100; return 20;'
assert 100 'if (1) return 100; return 20;'
assert 100 'a=100; b=100; if (a==b) return b; return a;'
assert 101 'a=101; b=100; if (a==b) return b; return a;'
assert 3 'a=3; return a;'
assert 8 'abc=3; xyzz=5; return abc+xyzz;'
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
