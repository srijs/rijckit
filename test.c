#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "lex.h"

void print_tok(Ctx *const ctx, const Type type, const Tok tok) {

  char *tok_str[] = {
    [UndefinedType] = "Undefined",
    [NumberType] = "Number",
    [IdentifierType] = "Identifier",
    [WhitespaceType] = "Whitespace",
    [StringType] = "String",
    [CharacterType] = "Character",
    [PunctuationType] = "Punctuation"
  };

  switch (tok.state) {

    case Success:
    if (tok.len < ctx->sz) {
      ctx->sz  = ctx->sz - tok.len;
      ctx->buf = &ctx->buf[tok.len];
      return route(ctx, print_tok);
    }
    return exit(0);

    case Fail:
    printf("Fail, Tok: %s, Char: %u\n", tok_str[type], ctx->buf[0]);
    return exit(0);

    case Undecided:
    printf("Undecided, Tok: %s\n", tok_str[type]);
    if (ctx->sz < ctx->back_sz) {
      memmove(ctx->back_buf, ctx->buf, ctx->sz);
      ssize_t rd = read(0, &ctx->back_buf[ctx->sz], ctx->back_sz - ctx->sz);
      if (rd > 0) {
        ctx->sz  = rd;
        ctx->buf = ctx->back_buf;
        return route(ctx, print_tok);
      }
      else if (rd == 0) {
        ctx->sz = 1;
        ctx->buf = ctx->back_buf;
        ctx->buf[0] = '\0';
        return route(ctx, print_tok);
      }
    }
    return exit(0);

    case End:
    printf("End\n");
    return exit(0);

  }

}

int main(void) {

  char buf[1024];
  Ctx ctx = {1024, 1024, buf, buf};

  read(0, ctx.buf, 1024);
  route(&ctx, print_tok);

  return 0;

}