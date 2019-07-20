#!/bin/bash

echo 'int hoge() { return 42; }' | gcc -xc -c -o tmp_hoge.o -
echo 'int add(int a, int b) { return a + b; }' | gcc -xc -c -o tmp_add.o -
echo 'int add6(int a, int b, int c, int d, int e, int f) { return a + b + c + d + e + f; }' | gcc -xc -c -o tmp_add6.o -

try() {
  expected="$1"
  input="$2"

  ./nacc "$input" > tmp.s
  gcc -o tmp tmp.s tmp_hoge.o tmp_add.o tmp_add6.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

try 0 'int main () { 0; }'
try 42 'int main () { 42; }'

try 21 'int main () { 5+20-4; }'
try 41 'int main () { 12 + 34 - 5 ; }'

try 4 'int main () { 2 * 2; }'
try 3 'int main () { 9 / 3; }'

try 5 'int main () { 1 + 2 * 2; }'
try 6 'int main () { 2 * 1 + 2 * 2; }'
try 1 'int main () { 2 / 1 - 2 / 2; }'

try 12 'int main () { 2 * (1 + 2) * 2; }'
try 2 'int main () { 8 / (4 - 2) / 2; }'

try 5 'int main () { -10 + 15; }'
try 5 'int main () { -(2 + 8) + 15; }'

try 1 'int main () { 1 == 1; }'
try 0 'int main () { 1 == 2; }'

try 1 'int main () { 1 != 0; }'
try 0 'int main () { 1 != 1; }'

try 1 'int main () { 1 < 2; }'
try 0 'int main () { 1 < 1; }'

try 1 'int main () { 2 > 1; }'
try 0 'int main () { 1 > 1; }'

try 1 'int main () { 1 <= 1; }'
try 1 'int main () { 0 <= 1; }'
try 0 'int main () { 2 <= 1; }'

try 1 'int main () { 2 >= 2; }'
try 1 'int main () { 2 >= 1; }'
try 0 'int main () { 0 >= 1; }'

try 3 'int main () { 1;2;3; }'
try 4 'int main () { 1 * 2; 2 / 2; 3 + 1; }'

try 42 'int main () { int a; int b; a = 20;b = a + 22;b; }'
try 1 'int main () { int a; a = 10;a - 5 == 5; }'

try 1 'int main () { return 1; }'
try 1 'int main () { return 1; 2; }'

try 1 'int main () { int a; int b; int c; int abc; a = 10;b = 5;c = 15;abc = a + b + c;abc == 30; }'

try 1 'int main () { if (1) return 1;0; }'
try 0 'int main () { if (0) return 1;0; }'
try 1 'int main () { int a; a = 1;if (a) return 1;0; }'
try 0 'int main () { int a; a = 0;if (a) return 1;0; }'

try 1 'int main () { if (1) return 1;else return 0; }'
try 0 'int main () { if (0) return 1;else return 0; }'
try 0 'int main () { int a; a = 1;if (a - 1) 1;else return 0; }'
try 2 'int main () { int a; a = 0;if (a < 2) a = a + 1;else return a;if (a < 2) a = a + 1;else return a;if (a < 2) a = a + 1;else return a; }'

try 5 'int main () { int a; a=0;while (a < 5) a = a + 1;a; }'
try 0 'int main () { int a; a=10;while (a) a = a - 2;a; }'

try 5 'int main () { int a; a=0;for (;;) if (a<5) a = a + 1; else return a; }'
try 5 'int main () { int a; for (a=0;;) if (a<5) a = a + 1; else return a; }'
try 5 'int main () { int a; for (a=0;a < 5;) a = a + 1; return a; }'
try 5 'int main () { int a; for (a=0;a < 5;a=a+1) 0; return a; }'

try 1 'int main () { { return 1; } }'
try 2 'int main () { { 1; { return 2; } } }'
try 3 'int main () { { { { return 3; } } } }'
try 4 'int main () { int a; int b; a = 0; b = 0; if (1) { a = 0; b = 4; } return b; }'
try 5 'int main () { int a; int i; a = 0; for (i = 0; i<10; i=i+1) { { { { { { i=i+1; a=a+1; } } } } } } return a; }'

try 42 'int main () { hoge(); }'

try 42 'int main () { add(20, 22); }'
try 42 'int main () { add6(4, 2, 10, 15, 3, 8); }'
try 3 'int main () { add(1, add(1, 1)); }'

try 3 'int one () { return 1; } int main () { return one() + one() + one(); }'
try 42 'int num (int a) { a; } int main () { return num(42); }'
try 55 'int fib (int n) { if (n == 0) return 0; if (n == 1) return 1; return fib(n - 2) + fib(n - 1);  } int main () { return fib(10); }'

try 3 'int main() { int x; int y; x = 3; y = &x; return *y; }'

echo OK
