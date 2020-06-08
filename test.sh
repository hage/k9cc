#!/bin/bash
CC='k9cc'

assert () {
    expected="$1"
    input="$2"
    ./$CC -e "$input" > tmp.s
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

assert_stdout_linked() {
    expected="$1"
    source="$2"
    input="$3"
    ./$CC -e "$input" > tmp.s
    cc -o tmp tmp.s $source
    actual=`./tmp`
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert_result_linked() {
    expected="$1"
    source="$2"
    input="$3"
    ./$CC -e "$input" > tmp.s
    cc -o tmp tmp.s $source
    ./tmp
    actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert_num_by_source() {
    expected="$1"
    source="$2"
    assembly=${source%.c}.s

    ./$CC $source
    cc -o tmp $assembly
    ./tmp
    actual="$?"
    if [ "$actual" = "$expected" ]; then
        echo "$source => $actual"
    else
        echo "$source => $expected expected, but got $actual"
        cat $source
        exit 1
    fi
}

assert '42' 'int main () {int a; int b; a=42;b=&a;return *b;}'
assert_num_by_source '144' test/fib-144.c
assert_num_by_source '43' test/num-43.c
assert_num_by_source '42' test/num-42.c

assert_stdout_linked '42'    test/test_primitive_param_func.c  "int main () {int a; int b; a=1;b=2;return foo(30, 12);}"
assert_stdout_linked '42'    test/test_primitive_param_func2.c "int main () {foo(12, 15, 2);}"
assert_result_linked '42'    test/test_primitive_param_func2.c "int main () {int a; a = foo(12, 15, 2); return a;}"
assert_stdout_linked 'hello' test/test_primitive_func_call.c   "int main () {foo();}"

assert '10' 'int main () {while(0){1;}return 10;}'
assert 110 'int main () {int r; int a; r=0;for(a=0;a<=10;a=a+1){r=r+a;r=r+a;} return r;}'
assert 42 'int main () {int a;for(a=0;a<42;a=a+1)a;return a;}'
assert 55 'int main () {int a; a=0;while(a<55)a=a+1;return a;}'
assert 42 'int main () {int a;a=1;if(a==1)return 42; else return 0;}'
assert 0 'int main () {int a;a=0;if(a==1)return 42; else return 0;}'
assert 42 'int main () {int a;a=1;if(a==1)return 1+a*41;}'
assert 20 'int main () {if(40*2==0)return 10;return 20;}';
assert 42 'int main () {return 42;}'
assert 42 'int main () {int foo;int bar;foo=33;bar=9;return foo+bar;10;}'
assert 99 'int main () {int foo;int bar;foo=33;bar=3;return foo*bar;}'
assert 42 'int main () {int a0;int a1;a0=1+1;a1=20;return a0+a1*2;}'
assert 0 'int main () {return 0;}'
assert 42 'int main () {return 42;}'
assert 21 "int main () {return 5+20-4;}"
assert 41 "int main () {return 12 + 34 - 5 ;}"
assert 50 "int main () {return 70-25 +5;}"
assert 47 'int main () {return 5+6*7;}'
assert 15 'int main () {return 5*(9-6);}'
assert 4 'int main () {return ( 3+5)/2;}'
assert 5 'int main () {return -(-2 * (2 + 1) +1);}'
assert 1 'int main () {return 1==3-2;}'
assert 0 'int main () {return 1==3-1;}'
assert 1 'int main () {return 1 != 3;}'
assert 0 'int main () {return 1 != 3-2;}'
assert 1 'int main () {return 1 < 3;}'
assert 0 'int main () {return 1 < 3-2;}'
assert 1 'int main () {return 1 <= 3;}'
assert 1 'int main () {return 1 <= 3-2;}'
assert 0 'int main () {return 10 <= 3-2;}'
assert 1 'int main () {return 3 > 1;}'
assert 0 'int main () {return 0 > 1;}'
assert 1 'int main () {return 3 >= 1;}'
assert 1 'int main () {return 1 >= 10 / 10;}'
assert 0 'int main () {return 1 >= 10 * 10;}'

echo OK
