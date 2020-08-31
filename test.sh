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
assert 42 'main(){return fun();} fun(){return 42;}'
assert 55 'main(){return fib(9);} fib(n){if(n<=1)return 1;else{return fib(n-1) + fib(n-2);}}'
assert 6 'main(){return fun(1,2,3,4,5,6);}fun(a,b,c,d,e,f){return f;}'
assert 3 'main(){return fun(1,2,3,4,5,6);}fun(a,b,c,d,e,f){return c;}'
assert 1 'main(){return fun(1,2,3,4,5,6);}fun(a,b,c,d,e,f){return a;}'
assert 40 'main(){return twice(20);} twice(a){return a * 2;}'
assert 42 'main(){return fourty_two();} fourty_two(){return 42;}'
assert_exsrc 6 test/add2.c 'main () {return return6th(1,2,3,4,5,6);}'
assert_exsrc 1 test/add2.c 'main () {return return1st(1,2,3,4,5,6);}'
assert_exsrc 231 test/add2.c 'main () {return add_each2_and_multiply(1,2,3,4,5,6);}'
assert_exsrc 42 test/add2.c 'main () {return add2(add2(10, 30), 2);}'
assert_exsrc 42 test/value40.c 'main () {return value40() + 2;}'

assert 55 'main () {for(sum=i=0;i<11;i=i+1){b=i;sum=sum+b;}return sum;}'
assert 55 'main () {sum = 0; for(i=0;i<11;i=i+1){sum=sum+i;}return sum;}'
assert 24 'main () {for(;0;)return 42;return 24;}'
assert 42 'main () {for(;1;)return 42;return 24;}'
assert 10 'main () {i=0;for(;i<10;)i=i+1;return i;}'
assert 55 'main () {sum=0;for(i=0;i<11;i=i+1)sum=sum+i;return sum;}'
assert 42 'main () {a=0;while(a<10)if(a==5)return 42;else a=a+1;}'
assert 11 'main () {a=0;while(a<=10)a=a+1;return a;}'
assert 10 'main () {if (1) return 10; else return 0;}'
assert 10 'main () {if (0) return 100; else return 10;}'
assert 20 'main () {if (0) return 100; return 20;}'
assert 100 'main () {if (1) return 100; return 20;}'
assert 100 'main () {a=100; b=100; if (a==b) return b; return a;}'
assert 101 'main () {a=101; b=100; if (a==b) return b; return a;}'
assert 3 'main () {a=3; return a;}'
assert 8 'main () {abc=3; xyzz=5; return abc+xyzz;}'
assert 6 'main () {a=b=3; return a+b;}'
assert 10 'main () {return 10;}'
assert 3 'main () {1;2; return 3;}'
assert 0 'main () {return 3>=4;}'
assert 1 'main () {return 3>=3;}'
assert 1 'main () {return 3>=2;}'
assert 1 'main () {return 3>2;}'
assert 0 'main () {return -1<=-2;}'
assert 1 'main () {return -1<=-1;}'
assert 1 'main () {return -1<=5;}'
assert 1 'main () {return -1<5;}'
assert 0 'main () {return 1!=1;}'
assert 1 'main () {return -1==(-2+3)*(-1);}'
assert 1 'main () {return 1==1;}'
assert 4 'main () {return 2+2;}'
assert 5 'main () {return 1-(-4);}'
assert 4 'main () {return -(- 4);}'
assert 3 'main () {return +3;}'
assert 42 'main () {return (18 + 3)*(2 + 2) / 2;}'
assert 26 'main () {return 2* 3+4 *5;}'
assert 42 'main () {return   12 + 20 - 10 +20;}'
assert 0 'main () {return 10+20-30;}'
assert 42 'main () {return 20+22;}'
assert 0 'main () {return 0;}'
assert 42 'main () {return 42;}'

wait
echo OK
