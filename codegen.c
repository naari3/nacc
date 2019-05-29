#include "nacc.h"

void gen_lval(Node *node) {
  if (node->ty != ND_IDENT) error("代入の左辺値が変数ではありません");

  int offset = ((Int *)map_get(vars, node->name))->num;
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", offset);
  printf("  push rax\n");
}

void gen(Node *node) {
  if (node->ty == ND_RETURN) {
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }

  if (node->ty == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  if (node->ty == ND_IDENT) {
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->ty == '=') {
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
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
