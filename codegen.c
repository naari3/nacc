#include "nacc.h"

void gen_lval(Node *node) {
  if (node->kind != ND_IDENT) error("代入の左辺値が変数ではありません");

  int offset = ((Int *)map_get(vars, node->name))->num;
  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", offset);
  printf("  push rax\n");
}

char *registers[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// プロローグ
// 変数分の領域を確保する
void gen_prologue(Vector *args) {
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", vars->keys->len * 8);
  for (int i = 0; i < args->len; i++) {
    gen_lval(args->data[i]);
    printf("  pop rax\n");
    printf("  mov [rax], %s\n", registers[i]);
  }
}

// エピローグ
// 最後の式の結果がRAXに残っているのでそれが返り値になる
void gen_epirogue() {
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  printf("  ret\n");
}

void gen_args(Vector *args) {
  for (int i = 0; i < args->len; i++) {
    printf("  mov rax, %s\n", registers[i]);
  }
}

void gen(Node *node) {
  if (node->kind == ND_FUNC) {
    printf("%s:\n", node->name);
    gen_prologue(node->params);
    for (int i = 0; i < ((Vector *)node->stmts)->len; i++) {
      // 抽象構文木を下りながらコード生成
      gen(((Node *)((Vector *)node->stmts)->data[i]));

      // 式の評価結果としてスタックに一つの値が残っている
      // はずなので、スタックが溢れないようにポップしておく
      printf("  pop rax\n");
    }
    gen_epirogue();

    return;
  }

  if (node->kind == ND_RETURN) {
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }

  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  if (node->kind == ND_IDENT) {
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_IF) {
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    if (node->rhs->kind == ND_ELSE) {
      printf("  je  .Lelse%d\n", node->rhs->id);
      gen(node->rhs->lhs);
      printf("  jmp  .LendIf%d\n", node->id);
      printf(".Lelse%d:\n", node->rhs->id);
      gen(node->rhs->rhs);
      printf(".LendIf%d:\n", node->id);
    } else {
      printf("  je  .LendIf%d\n", node->id);
      gen(node->rhs);
      printf(".LendIf%d:\n", node->id);
      printf("  push %d\n", 0);
    }
    return;
  }

  if (node->kind == ND_WHILE) {
    printf(".LbeginWhile%d:\n", node->id);
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .LendWhile%d\n", node->id);
    gen(node->rhs);
    printf("  jmp .LbeginWhile%d\n", node->id);
    printf(".LendWhile%d:\n", node->id);
    return;
  }

  if (node->kind == ND_FOR) {
    if (node->lhs->lhs) {
      gen(node->lhs->lhs);  // init
    }
    printf(".LbeginFor%d:\n", node->id);
    if (node->lhs->rhs->lhs) {
      gen(node->lhs->rhs->lhs);  // cond
    } else {
      printf("  push 1\n");  // must loop
    }
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .LendFor%d\n", node->id);
    gen(node->rhs);  // body
    if (node->lhs->rhs->rhs->lhs) {
      gen(node->lhs->rhs->rhs->lhs);  // iter
    }
    printf("  jmp .LbeginFor%d\n", node->id);
    printf(".LendFor%d:\n", node->id);

    return;
  }

  if (node->kind == ND_BLOCK) {
    for (int i = 0; i < ((Vector *)node->stmts)->len; i++) {
      gen(((Node *)((Vector *)node->stmts)->data[i]));
      if (i != 0) printf("  pop rax\n");
    }
    return;
  }

  if (node->kind == ND_CALL) {
    for (int i = 0; i < node->params->len; i++) {
      gen(((Node *)((Vector *)node->params)->data[i]));
      printf("  pop rax\n");
      printf("  mov %s, rax\n", registers[i]);
    }
    printf("  call %s\n", node->name);
    printf("  push rax\n");

    return;
  }

  if (node->kind == ND_ADDR) {
    gen_lval(node->lhs);
    return;
  }

  if (node->kind == ND_DEREF) {
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_ASSIGN) {
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

  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_EQ:
    case ND_NE:
      printf("  cmp rax, rdi\n");
      if (node->kind == ND_EQ) {
        printf("  sete al\n");
      } else {
        printf("  setne al\n");
      }
      printf("  movzb rax, al\n");
      break;
    case ND_LT:
    case ND_GT:
      if (node->kind == ND_LT) {
        printf("  cmp rax, rdi\n");
      } else {
        printf("  cmp rdi, rax\n");
      }
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LE:
    case ND_GE:
      if (node->kind == ND_LE) {
        printf("  cmp rax, rdi\n");
      } else {
        printf("  cmp rdi, rax\n");
      }
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
    default:
      break;
  }

  printf("  push rax\n");
}
