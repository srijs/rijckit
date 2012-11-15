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

  size_t sub = 0;

  switch (tok.state) {

    case lexFail:
    printf("Fail, Tok: %s, Char: %u\n", tok_str[type], ctx->buf[0]);
    return exit(0);

    case lexSuccess:
    if (ctx->sz - tok.len >= 4) {
      ctx->sz  = ctx->sz - tok.len;
      ctx->buf = &ctx->buf[tok.len];
      return lex(ctx, print_tok);
    }
    sub = tok.len;

    case lexUndecided:
    if (ctx->sz - sub < ctx->back_sz) {
      memmove(ctx->back_buf, ctx->buf, ctx->sz - sub);
      ssize_t rd = read(0, &ctx->back_buf[ctx->sz - sub], ctx->back_sz - (ctx->sz - sub));
      if ((ctx->sz - sub) + rd >= 4) {
        ctx->sz  = (ctx->sz - sub) + rd;
        ctx->buf = ctx->back_buf;
        return lex(ctx, print_tok);
      }
      else {
        for (int i = 0; i < (4 - (ctx->sz - sub + rd)); i++) {
          ctx->back_buf[ctx->sz - sub + rd + i] = '\0';
        }
        ctx->buf = ctx->back_buf;
        ctx->sz = (ctx->sz - sub) + 4;
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

  unsigned char buf[4096];
  lexCtx ctx = {4096, 4096, buf, buf};

  read(0, ctx.buf, 4096);
  lex(&ctx, print_tok);

  return 0;

}
