#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの型を表す値
enum {
  TK_NUM = 256,  // 整数トークン
  TK_EQ = 255,   // ==トークン
  TK_NE = 254,   // !=トークン
  TK_LE = 251,   // <=トークン なぜか253だとループに陥る
  TK_GE = 252,   // >=トークン
  TK_EOF,        // 入力の終わりを表すトークン
};

enum {
  ND_NUM = 256,  // 整数のノードの型a
};

typedef struct Node {
  int ty;            // 演算子かND_NUM
  struct Node *lhs;  // 左辺
  struct Node *rhs;  // 右辺
  int val;           // tyがND_NUMの場合のみ使う
} Node;

// トークンの型
typedef struct {
  int ty;       // トークンの型
  int val;      // tyがTK_NUMの場合、その数値
  char *input;  // トークン文字列（エラーメッセージ用）
} Token;

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vector() {
  Vector *vec = malloc(sizeof(Vector));
  vec->data = malloc(sizeof(void *) * 16);
  vec->capacity = 16;
  vec->len = 0;
  return vec;
}

void vec_push(Vector *vec, void *elem) {
  if (vec->capacity == vec->len) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
  }
  vec->data[vec->len++] = elem;
}

void expect(int line, int expected, int actual) {
  if (expected == actual) return;
  fprintf(stderr, "%d: %d expected, but got %d\n", line, expected, actual);
  exit(1);
}

void runtest() {
  Vector *vec = new_vector();
  expect(__LINE__, 0, vec->len);

  // intだとサイズが違って警告が出るので代わりにlongを使う
  // warning: cast to pointer from integer of different size
  // [-Wint-to-pointer-cast]
  // for (int i = 0; i < 100; i++) vec_push(vec, (void *)i);
  for (long i = 0; i < 100; i++) vec_push(vec, (void *)i);

  expect(__LINE__, 100, vec->len);
  expect(__LINE__, 0, (long)vec->data[0]);
  expect(__LINE__, 50, (long)vec->data[50]);
  expect(__LINE__, 99, (long)vec->data[99]);

  printf("OK\n");
}

// 入力プログラム
char *user_input;

int pos = 0;

// トークナイズした結果のトークン列はこの配列に保存する
// 100個以上のトークンは来ないものとする
Vector *tokens;

Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

int consume(int ty) {
  if (((Token *)tokens->data[pos])->ty != ty) return 0;
  pos++;
  return 1;
}

void error_at(char *loc, char *msg);

// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? term
// term       = num | "(" expr ")"

Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *term();

Node *expr() { return equality(); };

Node *equality() {
  Node *node = relational();
  for (;;) {
    if (consume(TK_EQ))
      node = new_node(TK_EQ, node, relational());
    else if (consume(TK_NE))
      node = new_node(TK_NE, node, relational());
    else
      return node;
  }
};

Node *relational() {
  Node *node = add();
  // return node;

  for (;;) {
    // printf("%s\n", tokens[pos].input);
    if (consume('<'))
      node = new_node('<', node, add());
    else if (consume('>'))
      node = new_node('>', node, add());
    else if (consume(TK_LE))
      node = new_node(TK_LE, node, add());
    else if (consume(TK_GE))
      node = new_node(TK_GE, node, add());
    else
      return node;
  }
};

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume('+'))
      node = new_node('+', node, mul());
    else if (consume('-'))
      node = new_node('-', node, mul());
    else
      return node;
  }
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume('*'))
      node = new_node('*', node, unary());
    else if (consume('/'))
      node = new_node('/', node, unary());
    else
      return node;
  }
}

Node *unary() {
  if (consume('+'))
    return term();
  else if (consume('-'))
    return new_node('-', new_node_num(0), term());
  return term();
}

Node *term() {
  // 次のトークンが'('なら、"(" expr ")"のはず
  if (consume('(')) {
    Node *node = expr();
    if (!consume(')'))
      error_at(((Token *)tokens->data[pos])->input,
               "開きカッコに対応する閉じカッコがありません");
    return node;
  }

  // そうでなければ数値のはず
  if (((Token *)tokens->data[pos])->ty == TK_NUM)
    return new_node_num(((Token *)tokens->data[pos++])->val);

  error_at(((Token *)tokens->data[pos])->input,
           "数値でも開きカッコでもないトークンです");
}

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラー箇所を報告するための関数
void error_at(char *loc, char *msg) {
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");  // pos個の空白を出力
  fprintf(stderr, "^ %s\n", msg);
  exit(1);
}

// user_inputが指している文字列を
// トークンに分割してtokensに保存する
void *tokenize(char *p) {
  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0 ||
        strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0) {
      Token *token = malloc(sizeof(Token));
      if (strncmp(p, "==", 2) == 0) {
        token->ty = TK_EQ;
      } else if (strncmp(p, "!=", 2) == 0) {
        token->ty = TK_NE;
      } else if (strncmp(p, "<=", 2) == 0) {
        token->ty = TK_LE;
      } else if (strncmp(p, ">=", 2) == 0) {
        token->ty = TK_GE;
      }
      token->input = p;
      p++;
      p++;  // 2文字なので
      vec_push(tokens, token);
      continue;
    }

    if (strncmp(p, "<", 1) == 0 || strncmp(p, ">", 1) == 0 ||
        strncmp(p, "+", 1) == 0 || strncmp(p, "-", 1) == 0 ||
        strncmp(p, "*", 1) == 0 || strncmp(p, "/", 1) == 0 ||
        strncmp(p, "(", 1) == 0 || strncmp(p, ")", 1) == 0) {
      Token *token = malloc(sizeof(Token));
      token->ty = *p;
      token->input = p;
      p++;
      vec_push(tokens, token);
      continue;
    }

    if (isdigit(*p)) {
      Token *token = malloc(sizeof(Token));
      token->ty = TK_NUM;
      token->input = p;
      token->val = strtol(p, &p, 10);
      vec_push(tokens, token);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  Token *token = malloc(sizeof(Token));
  token->ty = TK_EOF;
  token->input = p;
  vec_push(tokens, token);
}

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
    case TK_EQ:
    case TK_NE:
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
    case TK_LE:
    case TK_GE:
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

  tokens = new_vector();
  pos = 0;

  tokenize(user_input);
  Node *node = expr();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // 抽象構文木を下りながらコード生成
  gen(node);

  // スタックトップに式全体の値が残っているはずなので
  // それをRAXにロードして関数からの返り値とする
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
