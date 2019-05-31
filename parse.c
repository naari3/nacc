#include "nacc.h"

int pos;
int if_counter = 0;
int else_counter = 0;
int while_counter = 0;
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

Int *new_int(int i) {
  Int *in = malloc(sizeof(Int));
  in->num = i;
  return in;
}

int consume(int ty) {
  if (((Token *)tokens->data[pos])->ty != ty) return 0;
  pos++;
  return 1;
}

// program    = stmt*
// stmt       = expr ";"
//            | "if" "(" expr ")" stmt ("else" stmt)?
//            | "while" "(" expr ")" stmt
//            | "return" expr ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? term
// term       = num | ident | "(" expr ")"

void program();
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
  program();
};

void program() {
  int i = 0;
  while (((Token *)tokens->data[pos])->ty != TK_EOF) code[i++] = stmt();
  code[i] = NULL;
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

  if (((Token *)tokens->data[pos])->ty == TK_IDENT) {
    map_put(vars, ((Token *)tokens->data[pos])->name,
            new_int(8 * (vars->keys->len + 1)));
    return new_node_ident(((Token *)tokens->data[pos++])->name);
  }

  error_at(((Token *)tokens->data[pos])->input,
           "数値でも開きカッコでもないトークンです");
}

int is_al(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }

int is_alnum(char c) {
  return is_al(c) || ('0' <= c && c <= '9') || (c == '_');
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

    if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
      Token *token = malloc(sizeof(Token));
      token->ty = TK_RETURN;
      token->input = p;
      p += 6;
      vec_push(tokens, token);
      continue;
    }

    if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
      Token *token = malloc(sizeof(Token));
      token->ty = TK_IF;
      token->input = p;
      token->id = if_counter++;
      p += 2;
      vec_push(tokens, token);
      continue;
    }

    if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
      Token *token = malloc(sizeof(Token));
      token->ty = TK_ELSE;
      token->input = p;
      token->id = else_counter++;
      p += 4;
      vec_push(tokens, token);
      continue;
    }

    if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
      Token *token = malloc(sizeof(Token));
      token->ty = TK_WHILE;
      token->input = p;
      token->id = while_counter++;
      p += 5;
      vec_push(tokens, token);
      continue;
    }

    if (is_al(*p)) {
      int token_len = 1;
      while (is_al(p[token_len])) {
        token_len++;
      }
      Token *token = malloc(sizeof(Token));
      token->ty = TK_IDENT;
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
        strncmp(p, ";", 1) == 0 || strncmp(p, "=", 1) == 0) {
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
