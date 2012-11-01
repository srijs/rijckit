#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define NS(id) lex ## id
#include "lex.h"

void print_tok(lexCtx *const ctx, const lexType type, const lexTok tok) {

  char *tok_str[] = {
    [lexUndefined] = "Undefined",
    [lexNumber] = "Number",
    [lexIdentifier] = "Identifier",
    [lexWhitespace] = "Whitespace",
    [lexString] = "String",
    [lexCharacter] = "Character",
    [lexPunctuation] = "Punctuation"
  };

  switch (tok.state) {

    case lexSuccess:
    if (tok.len < ctx->sz) {
      ctx->sz  = ctx->sz - tok.len;
      ctx->buf = &ctx->buf[tok.len];
      return lex(ctx, print_tok);
    }
    return exit(0);

    case lexFail:
    printf("Fail, Tok: %s, Char: %u\n", tok_str[type], ctx->buf[0]);
    return exit(0);

    case lexUndecided:
    printf("Undecided, Tok: %s\n", tok_str[type]);
    if (ctx->sz < ctx->back_sz) {
      memmove(ctx->back_buf, ctx->buf, ctx->sz);
      ssize_t rd = read(0, &ctx->back_buf[ctx->sz], ctx->back_sz - ctx->sz);
      if (rd > 0) {
        ctx->sz  = rd;
        ctx->buf = ctx->back_buf;
        return lex(ctx, print_tok);
      }
      else if (rd == 0) {
        ctx->sz = 1;
        ctx->buf = ctx->back_buf;
        ctx->buf[0] = '\0';
        return lex(ctx, print_tok);
      }
    }
    return exit(0);

    case lexEnd:
    printf("End\n");
    return exit(0);

  }

}

int main(void) {

  char buf[4096];
  lexCtx ctx = {4096, 4096, buf, buf};

  read(0, ctx.buf, 4096);
  lex(&ctx, print_tok);

  return 0;

}
