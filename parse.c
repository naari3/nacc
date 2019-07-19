#include "nacc.h"

int pos;
Vector *tokens;
int if_counter = 0;
int else_counter = 0;
int while_counter = 0;
int for_counter = 0;
Node *code[100];

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

Node *new_node_ident(char *name) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->name = name;
  return node;
}

Node *new_node_call(char *name, Vector *params) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_CALL;
  node->name = name;
  node->params = params;
  return node;
}

Int *new_int(int i) {
  Int *in = malloc(sizeof(Int));
  in->num = i;
  return in;
}

int expect_token(int kind) {
  if (((Token *)tokens->data[pos])->kind != kind) return 0;
  return 1;
}

int consume(int ty) {
  if (expect_token(ty)) {
    pos++;
    return 1;
  }
  return 0;
}

// program    = func*
// func       = ident "("  ")" { stmt* }
// stmt       = expr ";"
//            | { stmt* }
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//            | "return" expr ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = "+"? term
//            | "-"? term
//            | "*" unary
//            | "&" unary
// term       = num |
//            | ident ("(" ")")?
//            | "(" expr ")"

void program();
Node *func();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *term();

void parse(char *codestr) {
  tokens = new_vector();
  tokenize(codestr);
  vars = new_map();
  pos = 0;
  return program();
};

void program() {
  int i = 0;
  while (!expect_token(TK_EOF)) code[i++] = func();
  code[i] = NULL;
}

Node *func() {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_FUNC;

  if (!expect_token(TK_IDENT))
    error_at(((Token *)tokens->data[pos])->input, "関数名がありません");
  node->name = ((Token *)tokens->data[pos++])->name;

  Vector *params = new_vector();
  if (!consume('('))
    error_at(((Token *)tokens->data[pos])->input, "引数定義が存在しません");
  while (!consume(')')) {
    vec_push(params, term());
    if (consume(')')) break;
    if (!consume(',')) {
      error_at(((Token *)tokens->data[pos])->input, "コンマではありません");
    }
  }
  node->params = params;

  Vector *stmts = new_vector();
  consume('{');
  while (!consume('}')) {
    vec_push(stmts, (void *)stmt());
  }
  node->stmts = stmts;

  return node;
}

Node *stmt() {
  Node *node;

  if (consume(TK_RETURN)) {
    node = malloc(sizeof(Node));
    node->ty = ND_RETURN;
    node->lhs = expr();
  } else if (consume(TK_IF)) {
    node = malloc(sizeof(Node));
    node->ty = ND_IF;
    node->id = ((Token *)tokens->data[pos - 1])->id;
    if (consume('(')) {
      node->lhs = expr();
      if (!consume(')'))
        error_at(((Token *)tokens->data[pos])->input,
                 "開きカッコに対応する閉じカッコがありません");
      Node *then = stmt();
      if (consume(TK_ELSE)) {
        Node *elseNode = malloc(sizeof(Node));
        elseNode->ty = ND_ELSE;
        elseNode->id = ((Token *)tokens->data[pos - 1])->id;
        elseNode->lhs = then;
        elseNode->rhs = stmt();
        node->rhs = elseNode;
      } else {
        node->rhs = then;
      }

      return node;
    } else {
      error_at(((Token *)tokens->data[pos])->input, "条件のカッコがありません");
    }
  } else if (consume(TK_WHILE)) {
    node = malloc(sizeof(Node));
    node->ty = ND_WHILE;
    node->id = ((Token *)tokens->data[pos - 1])->id;
    if (consume('(')) {
      node->lhs = expr();
      if (!consume(')'))
        error_at(((Token *)tokens->data[pos])->input,
                 "開きカッコに対応する閉じカッコがありません");
      node->rhs = stmt();

      return node;
    } else {
      error_at(((Token *)tokens->data[pos])->input, "条件のカッコがありません");
    }
  } else if (consume(TK_FOR)) {
    node = malloc(sizeof(Node));
    node->ty = ND_FOR;
    node->id = ((Token *)tokens->data[pos - 1])->id;
    Node *initNode = malloc(sizeof(Node));
    initNode->lhs = NULL;
    initNode->rhs = NULL;
    initNode->ty = ND_FOR_INIT;
    Node *condNode = malloc(sizeof(Node));
    condNode->lhs = NULL;
    condNode->rhs = NULL;
    condNode->ty = ND_FOR_COND;
    Node *iterNode = malloc(sizeof(Node));
    iterNode->lhs = NULL;
    iterNode->rhs = NULL;
    iterNode->ty = ND_FOR_ITER;
    condNode->rhs = iterNode;
    initNode->rhs = condNode;
    node->lhs = initNode;
    if (consume('(')) {
      if (!consume(';')) {
        initNode->lhs = expr();
        if (!consume(';'))
          error_at(((Token *)tokens->data[pos])->input,
                   "';'ではないトークンです");
      }
      if (!consume(';')) {
        condNode->lhs = expr();
        if (!consume(';'))
          error_at(((Token *)tokens->data[pos])->input,
                   "';'ではないトークンです");
      }
      if (!consume(')')) {
        iterNode->lhs = expr();
        if (!consume(')'))
          error_at(((Token *)tokens->data[pos])->input,
                   "開きカッコに対応する閉じカッコがありません");
      }
      node->rhs = stmt();

      return node;
    } else {
      error_at(((Token *)tokens->data[pos])->input, "条件のカッコがありません");
    }
  } else if (consume('{')) {
    node = malloc(sizeof(Node));
    node->ty = ND_BLOCK;

    Vector *stmts = new_vector();
    while (!consume('}')) {
      if (consume(TK_EOF)) {
        error_at(((Token *)tokens->data[pos])->input,
                 "ブロックの開きカッコに対応する閉じカッコがありません");
      }
      vec_push(stmts, (void *)stmt());
    }

    node->stmts = stmts;
    return node;
  } else {
    node = expr();
  }

  if (!consume(';'))
    error_at(((Token *)tokens->data[pos])->input, "';'ではないトークンです");

  return node;
};

Node *expr() { return assign(); };
Node *assign() {
  Node *node = equality();
  if (consume('=')) node = new_node('=', node, assign());
  return node;
};

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
  else if (consume('&'))
    return new_node(ND_ADDR, unary(), NULL);
  else if (consume('*'))
    return new_node(ND_DEREF, unary(), NULL);
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
  if (((Token *)tokens->data[pos])->kind == TK_NUM)
    return new_node_num(((Token *)tokens->data[pos++])->val);

  if (((Token *)tokens->data[pos])->kind == TK_IDENT) {
    Vector *params = new_vector();
    char *name = ((Token *)tokens->data[pos++])->name;
    if (consume('(')) {  // call
      for (int i = 0; i < 6; i++) {
        if (consume(')')) {
          return new_node_call(name, params);
        } else {
          vec_push(params, expr());
          if (consume(')')) {
            return new_node_call(name, params);
          }
        }
        if (!consume(',')) {
          error_at(((Token *)tokens->data[pos])->input, "コンマではありません");
        }
      }
    }
    map_put(vars, name, new_int(8 * (vars->keys->len + 1)));
    return new_node_ident(name);
  }

  error_at(((Token *)tokens->data[pos])->input,
           "数値でも開きカッコでもないトークンです");
}

int is_al(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

int is_alnum(char c) { return is_al(c) || ('0' <= c && c <= '9'); }

// user_inputが指している文字列を
// トークンに分割してtokensに保存する
void tokenize(char *p) {
  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
      Token *token = malloc(sizeof(Token));
      token->kind = TK_RETURN;
      token->input = p;
      p += 6;
      vec_push(tokens, token);
      continue;
    }

    if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
      Token *token = malloc(sizeof(Token));
      token->kind = TK_IF;
      token->input = p;
      token->id = if_counter++;
      p += 2;
      vec_push(tokens, token);
      continue;
    }

    if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
      Token *token = malloc(sizeof(Token));
      token->kind = TK_ELSE;
      token->input = p;
      token->id = else_counter++;
      p += 4;
      vec_push(tokens, token);
      continue;
    }

    if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
      Token *token = malloc(sizeof(Token));
      token->kind = TK_WHILE;
      token->input = p;
      token->id = while_counter++;
      p += 5;
      vec_push(tokens, token);
      continue;
    }

    if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
      Token *token = malloc(sizeof(Token));
      token->kind = TK_FOR;
      token->input = p;
      token->id = for_counter++;
      p += 3;
      vec_push(tokens, token);
      continue;
    }

    if (is_al(*p)) {
      int token_len = 1;
      while (is_alnum(p[token_len])) {
        token_len++;
      }
      Token *token = malloc(sizeof(Token));
      token->kind = TK_IDENT;
      token->name = strndup(p, token_len);
      token->input = p;
      vec_push(tokens, token);
      p += token_len;
      continue;
    }

    if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0 ||
        strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0) {
      Token *token = malloc(sizeof(Token));
      if (strncmp(p, "==", 2) == 0) {
        token->kind = TK_EQ;
      } else if (strncmp(p, "!=", 2) == 0) {
        token->kind = TK_NE;
      } else if (strncmp(p, "<=", 2) == 0) {
        token->kind = TK_LE;
      } else if (strncmp(p, ">=", 2) == 0) {
        token->kind = TK_GE;
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
        strncmp(p, ";", 1) == 0 || strncmp(p, "=", 1) == 0 ||
        strncmp(p, "{", 1) == 0 || strncmp(p, "}", 1) == 0 ||
        strncmp(p, ",", 1) == 0 || strncmp(p, "&", 1) == 0) {
      Token *token = malloc(sizeof(Token));
      token->kind = *p;
      token->input = p;
      p++;
      vec_push(tokens, token);
      continue;
    }

    if (isdigit(*p)) {
      Token *token = malloc(sizeof(Token));
      token->kind = TK_NUM;
      token->input = p;
      token->val = strtol(p, &p, 10);
      vec_push(tokens, token);
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  Token *token = malloc(sizeof(Token));
  token->kind = TK_EOF;
  token->input = p;
  vec_push(tokens, token);
}
