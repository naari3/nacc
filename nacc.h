// 9cc.h: ヘッダファイル
// main.c: main関数
// parse.c: パーサ
// codegen.c: コードジェネレータ
// container.c: ベクタ、マップ、およびそのテストコード

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(char *fmt, ...);

// トークンの型を表す値
enum {
  TK_NUM = 256,  // 整数トークン
  TK_EQ,         // ==トークン
  TK_NE,         // !=トークン
  TK_LE,         // <=トークン
  TK_GE,         // >=トークン
  TK_EOF,        // 入力の終わりを表すトークン
  TK_IDENT,      // 識別子トークン
};

enum {
  ND_NUM = 256,  // 整数ノード
  ND_EQ,         // ==ノード
  ND_NE,         // !=ノード
  ND_LE,         // <=ノード
  ND_GE,         // >=ノード
};

typedef struct Node {
  int ty;            // 演算子かND_NUM
  struct Node *lhs;  // 左辺
  struct Node *rhs;  // 右辺
  int val;           // tyがND_NUMの場合のみ使う
} Node;

Node *parse();

void gen(Node *node);

// トークンの型
typedef struct {
  int ty;       // トークンの型
  int val;      // tyがTK_NUMの場合、その数値
  char *input;  // トークン文字列（エラーメッセージ用）
} Token;

void tokenize(char *p);

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

// トークナイズした結果のトークン列はここに保存される
Vector *tokens;

Vector *new_vector();
void vec_push(Vector *vec, void *elem);
void runtest();

extern int pos;
extern char *user_input;
