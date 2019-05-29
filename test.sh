#!/bin/bash
try() {
  expected="$1"
  input="$2"

  ./nacc "$input" > tmp.s
  gcc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

try 0 '0;'
try 42 '42;'

try 21 '5+20-4;'
try 41 " 12 + 34 - 5 ;"

try 4 '2 * 2;'
try 3 '9 / 3;'

try 5 '1 + 2 * 2;'
try 6 '2 * 1 + 2 * 2;'
try 1 '2 / 1 - 2 / 2;'

try 12 '2 * (1 + 2) * 2;'
try 2 '8 / (4 - 2) / 2;'

try 5 '-10 + 15;'
try 5 '-(2 + 8) + 15;'

try 1 '1 == 1;'
try 0 '1 == 2;'

try 1 '1 != 0;'
try 0 '1 != 1;'

try 1 '1 < 2;'
try 0 '1 < 1;'

try 1 '2 > 1;'
try 0 '1 > 1;'

try 1 '1 <= 1;'
try 1 '0 <= 1;'
try 0 '2 <= 1;'

try 1 '2 >= 2;'
try 1 '2 >= 1;'
try 0 '0 >= 1;'

try 3 '1;2;3;'
try 4 '1 * 2; 2 / 2; 3 + 1;'

try 42 'a = 20;b = a + 22;b;'
try 1 'a = 10;a - 5 == 5;'

try 1 'return 1;'
try 1 'return 1; 2;'

try 1 'a = 10;b = 5;c = 15;abc = a + b + c;abc == 30;'

try 1 'if (1) return 1;0;'
try 0 'if (0) return 1;0;'
try 1 'a = 1;if (a) return 1;0;'
try 0 'a = 0;if (a) return 1;0;'

echo OK
