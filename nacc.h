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

typedef struct {
  int num;
} Int;

Int *new_int(int i);

// トークンの型を表す値
typedef enum {
  TK_NUM = 256,  // 整数トークン
  TK_EQ,         // ==トークン
  TK_NE,         // !=トークン
  TK_LE,         // <=トークン
  TK_GE,         // >=トークン
  TK_INT,        // type int
  TK_EOF,        // 入力の終わりを表すトークン
  TK_RETURN,     // returnトークン
  TK_IF,         // ifトークン
  TK_ELSE,       // elseトークン
  TK_WHILE,      // whileトークン
  TK_FOR,        // forトークン
  TK_IDENT,      // 識別子トークン
} TokenKind;

typedef struct Token Token;

// トークンの型
struct Token {
  TokenKind kind;  // トークンの型
  Token *next;     // 次の入力トークン
  int val;         // kindがTK_NUMの場合、その数値
  char *name;      // kindがTK_IDENTの場合、その名前
  char *input;     // トークン文字列（エラーメッセージ用）
  int len;         // トークンの長さ
};

Token *tokenize(char *p);

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

Vector *new_vector();
void vec_push(Vector *vec, void *elem);
void runtest();

typedef enum {
  ND_NUM,       // 整数ノード
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_ASSIGN,    // =
  ND_EQ,        // ==ノード
  ND_NE,        // !=ノード
  ND_LT,        // <ノード
  ND_LE,        // <=ノード
  ND_GT,        // >ノード
  ND_GE,        // >=ノード
  ND_RETURN,    // returnノード
  ND_IF,        // ifノード
  ND_ELSE,      // elseノード
  ND_WHILE,     // whileノード
  ND_FOR,       // forノード
  ND_FOR_INIT,  // forの初期化式ノード
  ND_FOR_COND,  // forの条件式ノード
  ND_FOR_ITER,  // forの増減処理ノード
  ND_IDENT,     // 識別子ノード
  ND_LVAR,      // 識別子ノード
  ND_BLOCK,     // ブロックノード
  ND_CALL,      // 関数呼び出しノード
  ND_FUNC,      // 関数ノード
  ND_ADDR,      // & アドレスノード
  ND_DEREF,     // * 参照ノード
} NodeKind;

typedef struct Node {
  NodeKind kind;     // 演算子かND_NUM
  struct Node *lhs;  // 左辺
  struct Node *rhs;  // 右辺
  int val;           // kindがND_NUMの場合のみ使う
  char *name;        // kindがND_IDENTの場合のみ使う
  int offset;        // RBPからのオフセット
  Vector *stmts;     // ブロック用のstmtのベクタ
  Vector *params;    // 関数呼び出し時の引数用のstmtのベクタ
} Node;

void parse(char *codestr);

void gen(Node *node);

extern Node *code[];
extern Node *functions[];

typedef struct LVar LVar;

// ローカル変数の型
struct LVar {
  LVar *next;  // 次の変数かNULL
  char *name;  // 変数の名前
  int len;     // 名前の長さ
  int offset;  // RBPからのオフセット
};

// ローカル変数
LVar *locals;

extern int pos;
extern char *user_input;

char *strndup(const char *s, size_t n);
