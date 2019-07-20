#include "nacc.h"

int pos;
int if_counter = 0;
int else_counter = 0;
int while_counter = 0;
int for_counter = 0;
Node *code[100];
Token *token;

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

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->len == tok->len && !memcmp(tok->input, var->name, var->len))
      return var;
  }
  return NULL;
}

void set_lvar(Node *node, char *name, int length) {
  node->kind = ND_LVAR;
  LVar *lvar = malloc(sizeof(LVar));
  if (locals) lvar->next = locals;
  lvar->name = name;
  lvar->len = length;
  if (locals)
    lvar->offset = locals->offset + 8;
  else
    lvar->offset = 8;
  node->offset = lvar->offset;
  locals = lvar;
}

Node *new_node(int kind, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *new_node_ident(char *name) {
  Node *node = malloc(sizeof(Node));
  node->kind = ND_IDENT;
  node->name = name;
  return node;
}

Node *new_node_call(char *name, Vector *params) {
  Node *node = malloc(sizeof(Node));
  node->kind = ND_CALL;
  node->name = name;
  node->params = params;
  return node;
}

Int *new_int(int i) {
  Int *in = malloc(sizeof(Int));
  in->num = i;
  return in;
}

int expect_token(int kind) { return token->kind == kind; }

int consume(int kind) {
  if (expect_token(kind)) {
    token = token->next;
    return 1;
  }
}

Token *consume_ident(void) {
  if (token->kind != TK_IDENT) return NULL;
  Token *tok = token;
  token = token->next;
  return tok;
}

int consume_number(void) {
  if (!expect_token(TK_NUM)) return NULL;
  Token *tok = token;
  token = token->next;
  return tok->val;
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
  token = tokenize(codestr);
  vars = new_map();
  return program();
};

void program() {
  int i = 0;
  while (!expect_token(TK_EOF)) code[i++] = func();
  code[i] = NULL;
}

Node *func() {
  Node *node = malloc(sizeof(Node));
  Token *tok;
  node->kind = ND_FUNC;
  if (!expect_token(TK_IDENT) || !(tok = consume_ident()))
    error_at(token->input, "関数名がありません");

  node->name = (char *)malloc(tok->len * sizeof(char));
  strncpy(node->name, tok->input, tok->len);

  node->name[tok->len] = '\0';

  Vector *params = new_vector();
  if (!consume('(')) error_at(token->input, "引数定義が存在しません");
  while (!consume(')')) {
    vec_push(params, term());
    if (consume(')')) break;
    if (!consume(',')) {
      error_at(token->input, "コンマではありません");
    }
  }
  node->params = params;

  Vector *stmts = new_vector();
  if (!consume('{')) error_at(token->input, "'{'ではありません");
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
    node->kind = ND_RETURN;
    node->lhs = expr();
  } else if (consume(TK_IF)) {
    node = malloc(sizeof(Node));
    node->kind = ND_IF;
    if (consume('(')) {
      node->lhs = expr();
      if (!consume(')'))
        error_at(token->input, "開きカッコに対応する閉じカッコがありません");
      Node *then = stmt();
      if (consume(TK_ELSE)) {
        Node *elseNode = malloc(sizeof(Node));
        elseNode->kind = ND_ELSE;
        elseNode->lhs = then;
        elseNode->rhs = stmt();
        node->rhs = elseNode;
      } else {
        node->rhs = then;
      }

      return node;
    } else {
      error_at(token->input, "条件のカッコがありません");
    }
  } else if (consume(TK_WHILE)) {
    node = malloc(sizeof(Node));
    node->kind = ND_WHILE;
    if (consume('(')) {
      node->lhs = expr();
      if (!consume(')'))
        error_at(token->input, "開きカッコに対応する閉じカッコがありません");
      node->rhs = stmt();

      return node;
    } else {
      error_at(token->input, "条件のカッコがありません");
    }
  } else if (consume(TK_FOR)) {
    node = malloc(sizeof(Node));
    node->kind = ND_FOR;
    Node *initNode = malloc(sizeof(Node));
    initNode->lhs = NULL;
    initNode->rhs = NULL;
    initNode->kind = ND_FOR_INIT;
    Node *condNode = malloc(sizeof(Node));
    condNode->lhs = NULL;
    condNode->rhs = NULL;
    condNode->kind = ND_FOR_COND;
    Node *iterNode = malloc(sizeof(Node));
    iterNode->lhs = NULL;
    iterNode->rhs = NULL;
    iterNode->kind = ND_FOR_ITER;
    condNode->rhs = iterNode;
    initNode->rhs = condNode;
    node->lhs = initNode;
    if (consume('(')) {
      if (!consume(';')) {
        initNode->lhs = expr();
        if (!consume(';')) error_at(token->input, "';'ではないトークンです");
      }
      if (!consume(';')) {
        condNode->lhs = expr();
        if (!consume(';')) error_at(token->input, "';'ではないトークンです");
      }
      if (!consume(')')) {
        iterNode->lhs = expr();
        if (!consume(')'))
          error_at(token->input, "開きカッコに対応する閉じカッコがありません");
      }
      node->rhs = stmt();

      return node;
    } else {
      error_at(token->input, "条件のカッコがありません");
    }
  } else if (consume('{')) {
    node = malloc(sizeof(Node));
    node->kind = ND_BLOCK;

    Vector *stmts = new_vector();
    while (!consume('}')) {
      if (consume(TK_EOF)) {
        error_at(token->input,
                 "ブロックの開きカッコに対応する閉じカッコがありません");
      }
      vec_push(stmts, (void *)stmt());
    }

    node->stmts = stmts;
    return node;
  } else {
    node = expr();
  }

  if (!consume(';')) error_at(token->input, "';'ではないトークンです");

  return node;
};

Node *expr() { return assign(); };
Node *assign() {
  Node *node = equality();
  if (consume('=')) node = new_node(ND_ASSIGN, node, assign());
  return node;
};

Node *equality() {
  Node *node = relational();
  for (;;) {
    if (consume(TK_EQ))
      node = new_node(ND_EQ, node, relational());
    else if (consume(TK_NE))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
};

Node *relational() {
  Node *node = add();
  // return node;

  for (;;) {
    if (consume('<'))
      node = new_node(ND_LT, node, add());
    else if (consume('>'))
      node = new_node(ND_GT, node, add());
    else if (consume(TK_LE))
      node = new_node(ND_LE, node, add());
    else if (consume(TK_GE))
      node = new_node(ND_GE, node, add());
    else
      return node;
  }
};

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume('+'))
      node = new_node(ND_ADD, node, mul());
    else if (consume('-'))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume('*'))
      node = new_node(ND_MUL, node, unary());
    else if (consume('/'))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

Node *unary() {
  if (consume('+'))
    return term();
  else if (consume('-'))
    return new_node(ND_SUB, new_node_num(0), term());
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
      error_at(token->input, "開きカッコに対応する閉じカッコがありません");
    return node;
  }

  // そうでなければ数値
  if (token->kind == TK_NUM) return new_node_num(consume_number());

  // でなければ変数
  if (token->kind == TK_IDENT) {
    Vector *params = new_vector();

    Token *tok;
    char *name;
    if (!expect_token(TK_IDENT) || !(tok = consume_ident()))
      error_at(token->input, "関数名がありません");

    name = (char *)malloc(tok->len * sizeof(char));
    strncpy(name, tok->input, tok->len);
    name[tok->len] = '\0';

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
          error_at(token->input, "コンマではありません");
        }
      }
    } else {
      Node *node = new_node(ND_LVAR, NULL, NULL);
      LVar *lvar = find_lvar(tok);

      if (lvar) {
        node->offset = lvar->offset;
      } else {
        set_lvar(node, tok->input, tok->len);
      }

      node->name = name;
      return node;
    }
    map_put(vars, name, new_int(8 * (vars->keys->len + 1)));
  }

  error_at(token->input, "数値でも開きカッコでもないトークンです");
}

int is_al(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c == '_');
}

int is_alnum(char c) { return is_al(c) || ('0' <= c && c <= '9'); }

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int length) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->input = str;
  tok->len = length;

  cur->next = tok;
  return tok;
}

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
      cur = new_token(TK_RETURN, cur, p, 6);
      p += 6;
      continue;
    }

    if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
      cur = new_token(TK_IF, cur, p, 2);
      p += 2;
      continue;
    }

    if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
      cur = new_token(TK_ELSE, cur, p, 4);
      p += 4;
      continue;
    }

    if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
      cur = new_token(TK_WHILE, cur, p, 5);
      p += 5;
      continue;
    }

    if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
      cur = new_token(TK_FOR, cur, p, 3);
      p += 3;
      continue;
    }

    if (is_al(*p)) {
      int token_len = 1;
      while (is_alnum(p[token_len])) {
        token_len++;
      }

      cur = new_token(TK_IDENT, cur, p, token_len);
      p += token_len;
      continue;
    }

    if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0 ||
        strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0) {
      TokenKind kind;
      if (strncmp(p, "==", 2) == 0) {
        kind = TK_EQ;
      } else if (strncmp(p, "!=", 2) == 0) {
        kind = TK_NE;
      } else if (strncmp(p, "<=", 2) == 0) {
        kind = TK_LE;
      } else if (strncmp(p, ">=", 2) == 0) {
        kind = TK_GE;
      }
      cur = new_token(kind, cur, p, 2);
      p += 2;
      continue;
    }

    if (strncmp(p, "<", 1) == 0 || strncmp(p, ">", 1) == 0 ||
        strncmp(p, "+", 1) == 0 || strncmp(p, "-", 1) == 0 ||
        strncmp(p, "*", 1) == 0 || strncmp(p, "/", 1) == 0 ||
        strncmp(p, "(", 1) == 0 || strncmp(p, ")", 1) == 0 ||
        strncmp(p, ";", 1) == 0 || strncmp(p, "=", 1) == 0 ||
        strncmp(p, "{", 1) == 0 || strncmp(p, "}", 1) == 0 ||
        strncmp(p, ",", 1) == 0 || strncmp(p, "&", 1) == 0) {
      cur = new_token(*p, cur, p, 1);
      p++;
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *start = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - start;
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}
