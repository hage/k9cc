#!/bin/bash
CC=k9cc

assert () {
    expected="$1"
    input="$2"

    ./$CC "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 10 'while(0){1;}return 10;'
assert 110 'r=0;for(a=0;a<=10;a=a+1){r=r+a;r=r+a;} return r;'
assert 42 'for(a=0;a<42;a=a+1)a;return a;'
assert 55 'a=0;while(a<55)a=a+1;return a;'
assert 42 'a=1;if(a==1)return 42; else return 0;'
assert 0 'a=0;if(a==1)return 42; else return 0;'
assert 42 'a=1;if(a==1)return 1+a*41;'
assert 20 'if(40*2==0)10;20;';
assert 42 'return 42;'
assert 42 'foo=33;bar=9;return foo+bar;10;'
assert 99 'foo=33;bar=3;foo*bar;'
assert 42 'a0=1+1;a1=20;a0+a1*2;'
assert 0 '0;'
assert 42 '42;'
assert 21 "5+20-4;"
assert 41 "12 + 34 - 5 ;"
assert 50 " 70-25 +5;"
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 5 '-(-2 * (2 + 1) +1);'
assert 1 '1==3-2;'
assert 0 '1==3-1;'
assert 1 '1 != 3;'
assert 0 '1 != 3-2;'
assert 1 '1 < 3;'
assert 0 '1 < 3-2;'
assert 1 '1 <= 3;'
assert 1 '1 <= 3-2;'
assert 0 '10 <= 3-2;'
assert 1 '3 > 1;'
assert 0 '0 > 1;'
assert 1 '3 >= 1;'
assert 1 '1 >= 10 / 10;'
assert 0 '1 >= 10 * 10;'

echo OK
