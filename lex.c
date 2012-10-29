#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define likely(x) (__builtin_expect(x, 1))
#define unlikely(x) (__builtin_expect(x, 0))



// ## Fundamental Types


// ### Context Structure
// The context of our lexer includes two kinds of buffers:
// A variable frontbuffer, which acts as a window to the constant backbuffer
// and changes in size and position as we process the contents of the
// latter.

typedef struct {
  size_t sz, back_sz;
  char *buf, *back_buf;
} Ctx;


// ### Token Types
// We operate on a small set of tokens types.
// Keywords, typenames and identifiers are summarized under the
// `IdentifierType` token type.
// All kinds of punctuations have a common type,
// comments are also understood as punctuation.

typedef enum {
  UndefinedType,
  NumberType,
  IdentifierType,
  WhitespaceType,
  StringType,
  CharacterType,
  PunctuationType
} Type;


// ### Token Structure
// The emitted tokens include a success-indicating state and an optional
// length.

typedef struct {
  enum {
    Success, Fail,
    Undecided, End
  } state;
  size_t len;
} Tok;


// ### Continuation Type
// Since we'll program in Continuation-Passing-Style, we define
// the type of our continuation.

typedef void (*Cont)
  (Ctx *const,
   const Type,
   const Tok);



// ## Lexemes


// ### Framework Macro

#define lexeme(name, type, ctx, tok, expr...) \
          static void name \
          (Ctx *const ctx, const Cont ret) \
          { Tok tok; return ret(ctx, type, ((expr), tok)); }
                        

// ### Lexeme: Number
// Fail.

lexeme (number, NumberType, ctx, tok, {

  tok = (Tok){Fail};

});


// ### Lexeme: Identifier
// `identifier ::= ( A-Z | a-z | _ ) { ( A-z | a-z | 0-9 | _ ) }`
//
// An identifier is an _arbitrary long_ sequence of alphanumerics and
// underscore characters.
// Thus the length of the token is undecided, as long as we don't see a
// terminating character (that can't be contained in an identifier).

lexeme (identifier, IdentifierType, ctx, tok, {

  tok = (Tok){Undecided};

  for (size_t len = 1; len < ctx->sz; len++) {

    switch (ctx->buf[len]) {
      case 'A'...'Z': case 'a'...'z':
      case '0'...'9': case '_': {
        continue;
      }
    }

    tok = (Tok){Success, len};
    break;

  }

});


// ### Lexeme: Whitespace
// `whitespace ::= ( Space | Tab | NL | CR ) { whitespace }`
//
// Whitespace is an _arbitrary long_ sequence of space, tab, new-line
// and carriage-return characters.
// Thus the length of the token is undecided, as long as we don't see a
// terminating character (that can't be contained in whitespace).

lexeme (whitespace, WhitespaceType, ctx, tok, {

  tok = (Tok){Undecided};

  for (size_t len = 1; len < ctx->sz; len++) {

    switch (ctx->buf[len]) {
      case ' ': case '\t':
      case '\n': case '\r': {
        continue;
      }
    }

    tok = (Tok){Success, len};
    break;

  }

});


// ### Lexeme: String
// Fail.

lexeme (string, StringType, ctx, tok, {

  tok = (Tok){Fail};

});


// ### Lexeme: Character
// `character ::= S-Quote ( Esc-Seq | Char ) S-Quote`
//
// A character literal is an ASCII-identifier between
// two single quotes.

lexeme (character, CharacterType, ctx, tok, {

  // If the first character inside the quotes
  // is not a backslash, it is a simple literal
  // with a single character between the quotes.
  //
  // In this case, when the current buffer holds
  // less than three characters, the token is undecided.
  // If the third character is a terminating single quote,
  // the tokenization is successful with length three.

  if likely (ctx->buf[1] != '\\') {
    if likely (ctx->sz >= 3) {
      if likely (ctx->buf[2] == '\'') {
        tok = (Tok){Success, 3};
      }
      else tok = (Tok){Fail};
    }
    else tok = (Tok){Undecided};
  }

  // Otherwise, the literal is more complex
  // with a character escape sequence between
  // the quotes.

  else {
    if likely (ctx->sz >= 4) {
      if likely (ctx->buf[3] == '\'') {
        tok = (Tok){Success, 4};
      }
      else tok = (Tok){Fail};
    }
    else tok = (Tok){Undecided};
  }

});


// ### Lexeme: Punctuation (short)

lexeme (punctuation_short, PunctuationType, ctx, tok, {

  tok = (Tok){Success, 1};

});


// ### Lexeme: Punctuation (long)

lexeme (punctuation_long, PunctuationType, ctx, tok, {

  if unlikely (ctx->sz == 1) tok = (Tok){Undecided};

  else {

    const char suc = ctx->buf[1];

    switch (ctx->buf[0]) {

      // Is three-way-conditional?

      case '?':
      tok = (Tok){Success, unlikely (suc == ':') ? 2 : 1};
      break;

      // Is double-plus or plus-equals?

      case '+':
      tok = (Tok){Success, unlikely (suc == '+' || suc == '=') ? 2 : 1};
      break;

      // Is double-minus, arrow, or minus-equals?

      case '-':
      tok = (Tok){Success, unlikely (suc == '-' || suc == '>' || suc == '=') ? 2 : 1};
      break;

      // Is double-left-arrow or less-or-equal?

      case '<':
      tok = (Tok){Success, unlikely (suc == '<' || suc == '=') ? 2 : 1};
      break;

      // Is double-right-arrow or greater-or-equal?

      case '>':
      tok = (Tok){Success, unlikely (suc == '>' || suc == '=') ? 2 : 1};
      break;

      // Is ellipsis or dot?

      case '.':
      if unlikely (suc == '.') {
        if likely (ctx->sz >= 3) tok = (Tok){Success, likely (ctx->buf[2] == '.') ? 3 : 1};
        else                     tok = (Tok){Undecided};
      }
      else tok = (Tok){Success, 1};
      break;

      // Is comment or divide-equals?

      case '/':
      if unlikely (suc == '/') {
        for (size_t len = 2; len < ctx->sz; len++) {
          if unlikely (ctx->buf[len] == '\n' || ctx->buf[len] == '\r') {
            tok = (Tok){Success, len};
            goto end;
          }
        }
        tok = (Tok){Undecided};
      }
      else tok = (Tok){Success, unlikely (suc == '=') ? 2 : 1};
      end: break;

      // Else equal-sign may follow...

      default:
      tok = (Tok){Success, unlikely (suc == '=') ? 2 : 1};
      break;

    }

  }

});



// ## Routing
// Based on the first character of the input buffer,
// we route the tokenization process to a specific lexeme.

void route(Ctx *const ctx, const Cont ret) {

  switch (ctx->buf[0]) {

    case '\0':
    return ret(ctx, UndefinedType, (Tok){End});

    case '0'...'9':
    return number(ctx, ret);

    case 'A'...'Z': case 'a'...'z':
    case '_':
    return identifier(ctx, ret);

    case ' ': case '\t':
    case '\n': case '\r':
    return whitespace(ctx, ret);

    case '"':
    return string(ctx, ret);

    case '\'':
    return character(ctx, ret);

    case ',': case ';':
    case '(': case ')':
    case '[': case ']':
    case '{': case '}':
    case ':':
    return punctuation_short(ctx, ret);

    case '!': case '%':
    case '<': case '>':
    case '=': case '?':
    case '*': case '/':
    case '+': case '-':
    case '.':
    return punctuation_long(ctx, ret);

    default:
    return ret(ctx, UndefinedType, (Tok){Fail});

  }

}






// ## TEST CASE

#include <stdio.h>

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

