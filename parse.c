#include "nacc.h"

int pos;

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

// stmt       = expr ";"
// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? term
// term       = num | "(" expr ")"

Node *stmt();
Node *expr();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *term();

Node *parse() { return stmt(); };
Node *stmt() {
  Node *node = expr();
    if (!consume(';'))
      error_at(((Token *)tokens->data[pos])->input, "';'ではないトークンです");

  return node;
};
Node *expr() { return equality(); };

Node *equality() {
  Node *node = relational();
  for (;;) {
    if (consume(ND_EQ))
      node = new_node(ND_EQ, node, relational());
    else if (consume(ND_NE))
      node = new_node(ND_NE, node, relational());
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
    else if (consume(ND_LE))
      node = new_node(ND_LE, node, add());
    else if (consume(ND_GE))
      node = new_node(ND_GE, node, add());
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

// user_inputが指している文字列を
// トークンに分割してtokensに保存する
void tokenize(char *p) {
  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if ('a' <= *p && *p <= 'z') {
      Token *token = malloc(sizeof(Token));
      token->ty = TK_IDENT;
      token->input = p;
      p++;
      vec_push(tokens, token);
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
        strncmp(p, "(", 1) == 0 || strncmp(p, ")", 1) == 0 ||
        strncmp(p, ";", 1) == 0) {
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
