#include <stdlib.h>

#include "lex.h"

#define likely(x)   (__builtin_expect(x, 1))
#define unlikely(x) (__builtin_expect(x, 0))



// ## Lexemes


// ### Framework Macro

#define lexeme(name, type, ctx, tok, expr...) \
          static void name \
          (Ctx *const ctx, const Cont ret) \
          { Tok tok; return ret(ctx, type, ((expr), tok)); }
                        

// ### Lexeme: Number
// Fail.

lexeme (number, Number, ctx, tok, {

  tok = (Tok){Fail};

});


// ### Lexeme: Identifier
// `identifier ::= ( A-Z | a-z | _ ) { ( A-z | a-z | 0-9 | _ ) }`
//
// An identifier is an _arbitrary long_ sequence of alphanumerics and
// underscore characters.
// Thus the length of the token is undecided, as long as we don't see a
// terminating character (that can't be contained in an identifier).

lexeme (identifier, Identifier, ctx, tok, {

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

lexeme (whitespace, Whitespace, ctx, tok, {

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

lexeme (string, String, ctx, tok, {

  tok = (Tok){Fail};

});


// ### Lexeme: Character
// `character ::= S-Quote ( Esc-Seq | Char ) S-Quote`
//
// A character literal is an ASCII-identifier between
// two single quotes.

lexeme (character, Character, ctx, tok, {

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

lexeme (punctuation_short, Punctuation, ctx, tok, {

  tok = (Tok){Success, 1};

});


// ### Lexeme: Punctuation (long)

lexeme (punctuation_long, Punctuation, ctx, tok, {

  if unlikely (ctx->sz == 1) tok = (Tok){Undecided};

  else {

    const char suc = ctx->buf[1];

    switch (ctx->buf[0]) {

      // In the simple cases, just an equal-sign may follow...

      case '!': case '^':
      case '=': case '*':
      case '%':
      tok = (Tok){Success, unlikely (suc == '=') ? 2 : 1};
      break;

      // Is double-and or and-equals?

      case '&':
      tok = (Tok){Success, unlikely (suc == '&' || suc == '=') ? 2 : 1};
      break;

      // Is double-or or or-equals?

      case '|':
      tok = (Tok){Success, unlikely (suc == '|' || suc == '=') ? 2 : 1};
      break;

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
      
      // Since we have handled all possible cases,
      // we can declare the following codepath as
      // undefined.
      
      default:
      __builtin_unreachable();

    }

  }

});



// ## Routing
// Based on the first character of the input buffer,
// we route the tokenization process to a specific lexeme.

void lex(Ctx *const ctx, const Cont ret) {

  switch (ctx->buf[0]) {

    case '\0':
    return ret(ctx, Undefined, (Tok){End});

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
    case '.': case '^':
    case '&': case '|':
    return punctuation_long(ctx, ret);

    default:
    return ret(ctx, Undefined, (Tok){Fail});

  }

}
