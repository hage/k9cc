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
assert 4 'int main(){int a; a=4;return *&a;}'
assert 123 'int main(){int aa; set(&aa,120);return aa;} int set(int adr, int val){*adr=val+3;}'
assert 42 'int main(){int aa; set(&aa,42);return aa;} int set(int adr, int val){*adr=val;}'
assert 42 'int main(){return fun();} int fun(){return 42;}'
assert 55 'int main(){return fib(9);} int fib(int n){if(n<=1)return 1;else{return fib(n-1) + fib(n-2);}}'
assert 6 'int main(){return fun(1,2,3,4,5,6);} int fun(int a,int b,int c,int d,int e,int f){return f;}'
assert 3 'int main(){return fun(1,2,3,4,5,6);} int fun(int a,int b,int c,int d,int e,int f){return c;}'
assert 1 'int main(){return fun(1,2,3,4,5,6);} int fun(int a,int b,int c,int d,int e,int f){return a;}'
assert 40 'int main(){return twice(20);} int twice(int a){return a * 2;}'
assert 42 'int main(){return fourty_two();} int fourty_two(){return 42;}'
assert_exsrc 6 test/add2.c 'int main() {return return6th(1,2,3,4,5,6);}'
assert_exsrc 1 test/add2.c 'int main() {return return1st(1,2,3,4,5,6);}'
assert_exsrc 231 test/add2.c 'int main() {return add_each2_and_multiply(1,2,3,4,5,6);}'
assert_exsrc 42 test/add2.c 'int main() {return add2(add2(10, 30), 2);}'
assert_exsrc 42 test/value40.c 'int main() {return value40() + 2;}'

assert 55 'int main() {int sum;int i;for(sum=i=0;i<11;i=i+1){int b;b=i;sum=sum+b;}return sum;}'
assert 55 'int main() {int sum; int i; sum = 0; for(i=0;i<11;i=i+1){sum=sum+i;}return sum;}'
assert 24 'int main() {for(;0;)return 42;return 24;}'
assert 42 'int main() {for(;1;)return 42;return 24;}'
assert 10 'int main() {int i; i=0;for(;i<10;)i=i+1;return i;}'
assert 55 'int main() {int sum; int i; sum=0;for(i=0;i<11;i=i+1)sum=sum+i;return sum;}'
assert 42 'int main() {int a; a=0;while(a<10)if(a==5)return 42;else a=a+1;}'
assert 11 'int main() {int a; a=0;while(a<=10)a=a+1;return a;}'
assert 10 'int main() {if (1) return 10; else return 0;}'
assert 10 'int main() {if (0) return 100; else return 10;}'
assert 20 'int main() {if (0) return 100; return 20;}'
assert 100 'int main() {if (1) return 100; return 20;}'
assert 100 'int main() {int a; int b;a=100; b=100; if (a==b) return b; return a;}'
assert 101 'int main() {int a; int b;a=101; b=100; if (a==b) return b; return a;}'
assert 3 'int main() {int a;a=3; return a;}'
assert 8 'int main() {int abc; int xyzz; abc=3; xyzz=5; return abc+xyzz;}'
assert 6 'int main() {int a; int b; a=b=3; return a+b;}'
assert 10 'int main() {return 10;}'
assert 3 'int main() {1;2; return 3;}'
assert 0 'int main() {return 3>=4;}'
assert 1 'int main() {return 3>=3;}'
assert 1 'int main() {return 3>=2;}'
assert 1 'int main() {return 3>2;}'
assert 0 'int main() {return -1<=-2;}'
assert 1 'int main() {return -1<=-1;}'
assert 1 'int main() {return -1<=5;}'
assert 1 'int main() {return -1<5;}'
assert 0 'int main() {return 1!=1;}'
assert 1 'int main() {return -1==(-2+3)*(-1);}'
assert 1 'int main() {return 1==1;}'
assert 4 'int main() {return 2+2;}'
assert 5 'int main() {return 1-(-4);}'
assert 4 'int main() {return -(- 4);}'
assert 3 'int main() {return +3;}'
assert 42 'int main() {return (18 + 3)*(2 + 2) / 2;}'
assert 26 'int main() {return 2* 3+4 *5;}'
assert 42 'int main() {return   12 + 20 - 10 +20;}'
assert 0 'int main() {return 10+20-30;}'
assert 42 'int main() {return 20+22;}'
assert 0 'int main() {return 0;}'
assert 42 'int main() {return 42;}'

wait
echo OK
