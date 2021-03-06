#include "nacc.h"

int label = 1;

void gen_lval(Node *node) {
  if (node->kind != ND_LVAR) error("代入の左辺値が変数ではありません");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

char *registers[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// プロローグ
// 変数分の領域を確保する
void gen_prologue(Vector *args) {
  int offset = 0;
  if (locals) {
    offset += locals->offset;
  }
  if (args->len) {
    offset += args->len * 8;
  }
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  printf("  sub rsp, %d\n", offset);
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
    }

    // 式の評価結果としてスタックに一つの値が残っている
    // はずなので、スタックが溢れないようにポップしておく
    printf("  pop rax\n");
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

  if (node->kind == ND_LVAR) {
    gen_lval(node);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_IF) {
    int if_label;
    if_label = label;
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    if (node->rhs->kind == ND_ELSE) {
      printf("  je  .Lelse%d\n", if_label);
      gen(node->rhs->lhs);
      printf("  jmp  .LendIf%d\n", if_label);
      printf(".Lelse%d:\n", if_label);
      gen(node->rhs->rhs);
      printf(".LendIf%d:\n", if_label);
    } else {
      printf("  je  .LendIf%d\n", if_label);
      gen(node->rhs);
      printf(".LendIf%d:\n", if_label);
      printf("  push %d\n", 0);
    }
    label++;
    return;
  }

  if (node->kind == ND_WHILE) {
    int while_label;
    while_label = label;
    printf(".LbeginWhile%d:\n", while_label);
    gen(node->lhs);
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .LendWhile%d\n", while_label);
    gen(node->rhs);
    printf("  jmp .LbeginWhile%d\n", while_label);
    printf(".LendWhile%d:\n", while_label);
    label++;
    return;
  }

  if (node->kind == ND_FOR) {
    int for_label;
    for_label = label;
    if (node->lhs->lhs) {
      gen(node->lhs->lhs);  // init
    }
    printf(".LbeginFor%d:\n", for_label);
    if (node->lhs->rhs->lhs) {
      gen(node->lhs->rhs->lhs);  // cond
    } else {
      printf("  push 1\n");  // must loop
    }
    printf("  pop rax\n");
    printf("  cmp rax, 0\n");
    printf("  je  .LendFor%d\n", for_label);
    gen(node->rhs);  // body
    if (node->lhs->rhs->rhs->lhs) {
      gen(node->lhs->rhs->rhs->lhs);  // iter
    }
    printf("  jmp .LbeginFor%d\n", for_label);
    printf(".LendFor%d:\n", for_label);
    label++;

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
