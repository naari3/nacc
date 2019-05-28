#include "nacc.h"

void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->ty) {
    case '+':
      printf("  add rax, rdi\n");
      break;
    case '-':
      printf("  sub rax, rdi\n");
      break;
    case '*':
      printf("  imul rdi\n");
      break;
    case '/':
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_EQ:
    case ND_NE:
      printf("  cmp rax, rdi\n");
      if (node->ty == TK_EQ) {
        printf("  sete al\n");
      } else {
        printf("  setne al\n");
      }
      printf("  movzb rax, al\n");
      break;
    case '<':
    case '>':
      if (node->ty == '<') {
        printf("  cmp rax, rdi\n");
      } else {
        printf("  cmp rdi, rax\n");
      }
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LE:
    case ND_GE:
      if (node->ty == TK_LE) {
        printf("  cmp rax, rdi\n");
      } else {
        printf("  cmp rdi, rax\n");
      }
      printf("  setle al\n");
      printf("  movzb rax, al\n");
  }

  printf("  push rax\n");
}
