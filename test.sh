#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./k9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected, but got $actual"
        exit 1
    fi
}
assert 4 2+2
assert 5 '1-(-4)'
assert 4 '-(- 4)'
assert 3 '+3'
assert 42 '(18 + 3)*(2 + 2) / 2'
assert 26 '2* 3+4 *5'
assert 42 '  12 + 20 - 10 +20'
assert 0 10+20-30
assert 42 20+22
assert 0 0
assert 42 42

echo OK
