#include "nacc.h"

char *user_input;

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
    return 1;
  }

  // トークナイズする
  user_input = argv[1];
  if (strncmp(user_input, "-test", 5) == 0) {
    runtest();
    return 0;
  }

  parse(user_input);

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  for (int i = 0; code[i]; i++) {
    gen(code[i]);
  }

  return 0;
}
