// ## Prologue
//
// This is a simple and general lexer function for c-like languages.
// The main design goals are as follows:
//
// 1. To provide a very readable implementation of a lexer function.
// 2. To be useful in sequential work-flows, as well as systems
//    working with asynchronous IO and / or threads.
// 3. To be completely independent from any external libraries.
// 4. To perform all data dependent computations as fast as possible
//    and leave the rest to other software components.
// 5. To maximize the mean throughput by optimizing the code-paths
//    for small tokens.
// 6. To use no heap space by utilizing continuation-passing-style and
//    performing allocations solely on the stack.

#include <stddef.h>
#include <stdbool.h>

// Type-declarations and interfaces of this lexer are found in the
// `lex.h` header file.

#include "lex.h"

// In order to optimize code-paths, we define two convenience macros
// to give branch-prediction information to the compiler.

#define likely(x)   (__builtin_expect(x, true))
#define unlikely(x) (__builtin_expect(x, false))



// ## Lexemes
//
// To provide good readability and understandability, the
// functionality of the lexer as a whole is factored into its
// individual lexemes. Each of those is provided as a static function,
// taking the current context of the lexing process as a parameter
// and returning the matched token structure.

// ### Lexeme: Number
// Stub.

static inline Tok number (Ctx *const ctx) {

  size_t len;
  for (len = 1; len < ctx->sz; len++) {
    if (ctx->buf[len] < '0' || ctx->buf[len] > '9') {
      return (Tok){Success, len};
    }
  }
  return (Tok){Undecided};

}

// ### Lexeme: Identifier & Whitespace
//
// `identifier ::= ( A-Z | a-z | _ ) { ( A-z | a-z | 0-9 | _ ) }`
//
// `whitespace ::= ( Space | Tab | NL | CR ) { whitespace }`
//
// An identifier is an _arbitrary long_ sequence of alphanumerics
// and underscore characters.
//
// Whitespace is an _arbitrary long_ sequence of space, tab,
// new-line and carriage-return characters.

static inline bool is_alnum (char c) {
  return (((c >= 'A') & (c <= 'Z')) | ((c >= 'a') & (c <= 'Z'))
        | ((c >= '0') & (c <= '9')) | (c == '_'));
}

static inline bool is_whitespace (char c) {
  return ((c == ' ') | (c == '\t') | (c == '\n') | (c == '\r'));
}

static inline Tok identifier_or_whitespace (bool (*check)(char),
                                            Ctx *const ctx) {

  size_t len;

  if (check(ctx->buf[1]) == 0) {
    return (Tok){Success, 1};
  }

  if (check(ctx->buf[2]) == 0) {
    return (Tok){Success, 2};
  }

  if (check(ctx->buf[3]) == 0) {
    return (Tok){Success, 3};
  }

  for (len = 4; len < ctx->sz; len++) {
    if (check(ctx->buf[len]) == 0) return (Tok){Success, len};
  }

  return (Tok){Undecided};

}

// ### Lexeme: String & Character
//
// `string    ::= Dbl-Quote ( Char-Seq ) Dbl-Quote`
//
// `character ::= S-Quote   ( Char-Seq ) S-Quote`
//
// A string literal is a string of ASCII-identifier between
// two double quotes.
//
// A character literal is a string of ASCII-identifier between
// two single quotes.

static inline Tok string_or_character (const char termn,
                                       Ctx *const ctx) {

  size_t len;
  bool escape = false;

  if (ctx->buf[1] == termn) {
    return (Tok){Success, 2};
  }

  if (ctx->buf[2] == termn &&
      ctx->buf[1] != '\\') {
    return (Tok){Success, 3};
  }

  if (ctx->buf[3] == termn &&
      ctx->buf[2] != '\\') {
    return (Tok){Success, 4};
  }

  if (ctx->buf[3] == termn &&
      ctx->buf[2] == '\\'  &&
      ctx->buf[1] == '\\') {
    return (Tok){Success, 4};
  }

  for (len = 1; len < ctx->sz; len++) {

    if (escape == false) {
      if (ctx->buf[len] == termn) return (Tok){Success, len + 1};
      if (ctx->buf[len] == '\\')  escape = true;
    }
    else escape = false;

  }

  return (Tok){Undecided};

}

// ### Lexeme: Punctuation

static inline Tok punctuation (Ctx *const ctx) {

  Tok tok;
  size_t len;
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
    tok = (Tok){Success, unlikely (suc == '&'
                                 | suc == '=') ? 2 : 1};
    break;

    // Is double-or or or-equals?

    case '|':
    tok = (Tok){Success, unlikely (suc == '|'
                                 | suc == '=') ? 2 : 1};
    break;

    // Is three-way-conditional?

    case '?':
    tok = (Tok){Success, unlikely (suc == ':') ? 2 : 1};
    break;

    // Is double-plus or plus-equals?

    case '+':
    tok = (Tok){Success, unlikely (suc == '+'
                                 | suc == '=') ? 2 : 1};
    break;

    // Is double-minus, arrow, or minus-equals?

    case '-':
    tok = (Tok){Success, unlikely (suc == '-'
                                 | suc == '>'
                                 | suc == '=') ? 2 : 1};
    break;

    // Is double-left-arrow or less-or-equal?

    case '<':
    tok = (Tok){Success, unlikely (suc == '<'
                                 | suc == '=') ? 2 : 1};
    break;

    // Is double-right-arrow or greater-or-equal?

    case '>':
    tok = (Tok){Success, unlikely (suc == '>'
                                 | suc == '=') ? 2 : 1};
    break;

    // Is ellipsis or dot?

    case '.':
    if unlikely (suc == '.') {
      tok = (Tok){Success, likely (ctx->buf[2] == '.') ? 3 : 1};
    }
    else tok = (Tok){Success, 1};
    break;

    // Is comment or divide-equals?

    case '/':
    if unlikely (suc == '/') {
      for (len = 2; len < ctx->sz; len++) {
        if unlikely (ctx->buf[len] == '\n'
                   | ctx->buf[len] == '\r') {
          tok = (Tok){Success, len};
          goto end;
        }
      }
      tok = (Tok){Undecided};
    }
    else tok = (Tok){Success, unlikely (suc == '=') ? 2 : 1};
    end: break;

    case ',': case ';':
    case '(': case ')':
    case '[': case ']':
    case '{': case '}':
    case ':':
    tok = (Tok){Success, 1};
    break;

    // Since we have handled all possible cases, we can declare the
    // following codepath as undefined.

    default:
    __builtin_unreachable();

  }

  return tok;

}



// ## Routing

void lex (Ctx *const ctx, const Cont ret) {

  // Based on the first character of the input buffer, we route the
  // tokenization process to a specific lexeme.
  // We require that the length of our buffer is at least four
  // characters, else the behaviour of this function is undefined.

  if (ctx->sz < 4) {
    __builtin_unreachable();
  }

  switch (ctx->buf[0]) {

    case '\0':
    return ret(ctx, Undefined, (Tok){End});

    case '0'...'9':
    return ret(ctx, Number, number(ctx));

    case 'A'...'Z': case 'a'...'z':
    case '_':
    return ret(ctx, Identifier, identifier_or_whitespace(is_alnum, ctx));

    case ' ': case '\t':
    case '\n': case '\r':
    return ret(ctx, Whitespace, identifier_or_whitespace(is_whitespace, ctx));

    case '"':
    return ret(ctx, String, string_or_character('"', ctx));
    case '\'':
    return ret(ctx, Character, string_or_character('\'', ctx));

    case '!': case '%':
    case '<': case '>':
    case '=': case '?':
    case '*': case '/':
    case '+': case '-':
    case '.': case '^':
    case '&': case '|':
    case ',': case ';':
    case '(': case ')':
    case '[': case ']':
    case '{': case '}':
    case ':':
    return ret(ctx, Punctuation, punctuation(ctx));

    default:
    return ret(ctx, Undefined, (Tok){Fail});

  }

}
