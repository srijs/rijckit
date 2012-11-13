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

// We include the `stdlib` header just for the `size_t` typedef.

#include <stddef.h>

// Type-declarations and interfaces of this lexer are found in the
// `lex.h` header file.

#include "lex.h"

// In order to optimize code-paths, we define two convenience macros
// to give branch-prediction information to the compiler.

#define likely(x)   (__builtin_expect(x, 1))
#define unlikely(x) (__builtin_expect(x, 0))



// ## Lexemes
//
// To provide good readability and understandability, the
// functionality of the lexer as a whole is factored into its
// individual lexemes. Each of those is provided as a static function,
// taking the current context of the lexing process as a parameter
// and returning the matched token structure.

// ### Lexeme: Number
// Fail.

static Tok number (Ctx *const ctx) {

  return (Tok){Fail};

}

// ### Lexeme: Identifier
// `identifier ::= ( A-Z | a-z | _ ) { ( A-z | a-z | 0-9 | _ ) }`
//
// An identifier is an _arbitrary long_ sequence of alphanumerics
// and underscore characters.

static Tok identifier (Ctx *const ctx) {
  Tok tok;

  // Thus the length of the token is undecided, as long as we don't
  // see a terminating character (that can't be contained in an
  // identifier).

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

  return tok;

};

// ### Lexeme: Whitespace
// `whitespace ::= ( Space | Tab | NL | CR ) { whitespace }`
//
// Whitespace is an _arbitrary long_ sequence of space, tab,
// new-line and carriage-return characters.

static Tok whitespace (Ctx *const ctx) {
  Tok tok;

  // Thus the length of the token is undecided, as long as we don't
  // see a terminating character (that can't be contained in
  // whitespace).

  tok = (Tok){Undecided};

  #ifndef FAST

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

  #else

  if (((ctx->buf[1] == ' ')  | (ctx->buf[1] == '\t')
     | (ctx->buf[1] == '\n') | (ctx->buf[1] == '\r')) != 0)
    return (Tok){Success, 1};

  if (((ctx->buf[2] == ' ')  | (ctx->buf[2] == '\t')
     | (ctx->buf[2] == '\n') | (ctx->buf[2] == '\r')) != 0)
    return (Tok){Success, 2};

  if (((ctx->buf[3] == ' ')  | (ctx->buf[3] == '\t')
     | (ctx->buf[3] == '\n') | (ctx->buf[3] == '\r')) != 0)
    return (Tok){Success, 3};

  for (size_t len = 4; len < ctx->sz; len++) {

    if (((ctx->buf[len] == ' ')  | (ctx->buf[len] == '\t')
       | (ctx->buf[len] == '\n') | (ctx->buf[len] == '\r')) != 0)
      return (Tok){Success, len};

  }

  #endif

  return tok;

}

// ### Lexeme: String
// Fail.

static Tok string (Ctx *const ctx) {
  Tok tok;

  tok = (Tok){Undecided};
  int escape = 0;

  for (size_t len = 1; len < ctx->sz; len++) {

    if (!escape) {

      switch (ctx->buf[len]) {

        case '\\':
        escape = 1;
        continue;

        case '"':
        break;

        default:
        continue;

      }

    }

    else {
      escape = 0;
      continue;
    }

    tok = (Tok){Success, len+1};
    break;

  }

  return tok;

}

// ### Lexeme: Character
// `character ::= S-Quote ( Esc-Seq | Char ) S-Quote`
//
// A character literal is an ASCII-identifier between two single
// quotes.

static Tok character (Ctx *const ctx) {
  Tok tok;

  // If the first character inside the quotes is not a backslash,
  // it is a simple literal with a single character between the
  // quotes.
  // In this case, when the current buffer holds less than three
  // characters, the token is undecided. If the third character is a
  // terminating single quote, the tokenization is successful with
  // length three.

  if likely (ctx->buf[1] != '\\') {
    if likely (ctx->buf[2] == '\'') {
       tok = (Tok){Success, 3};
    }
    else tok = (Tok){Fail};
  }

  // Otherwise, the literal is more complex with a character escape
  // sequence between the quotes.

  else {
    if likely (ctx->buf[3] == '\'') {
      tok = (Tok){Success, 4};
    }
    else tok = (Tok){Fail};
  }

  return tok;

}

// ### Lexeme: Punctuation (short)

static Tok punctuation_short (Ctx *const ctx) {
  Tok tok;

  tok = (Tok){Success, 1};

  return tok;

}

// ### Lexeme: Punctuation (long)

static Tok punctuation_long (Ctx *const ctx) {
  Tok tok;

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
                                || suc == '=') ? 2 : 1};
    break;

    // Is double-or or or-equals?

    case '|':
    tok = (Tok){Success, unlikely (suc == '|'
                                || suc == '=') ? 2 : 1};
    break;

    // Is three-way-conditional?

    case '?':
    tok = (Tok){Success, unlikely (suc == ':') ? 2 : 1};
    break;

    // Is double-plus or plus-equals?

    case '+':
    tok = (Tok){Success, unlikely (suc == '+'
                                || suc == '=') ? 2 : 1};
    break;

    // Is double-minus, arrow, or minus-equals?

    case '-':
    tok = (Tok){Success, unlikely (suc == '-'
                                || suc == '>'
                                || suc == '=') ? 2 : 1};
    break;

    // Is double-left-arrow or less-or-equal?

    case '<':
    tok = (Tok){Success, unlikely (suc == '<'
                                || suc == '=') ? 2 : 1};
    break;

    // Is double-right-arrow or greater-or-equal?

    case '>':
    tok = (Tok){Success, unlikely (suc == '>'
                                || suc == '=') ? 2 : 1};
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
      for (size_t len = 2; len < ctx->sz; len++) {
        if unlikely (ctx->buf[len] == '\n'
                  || ctx->buf[len] == '\r') {
          tok = (Tok){Success, len};
          goto end;
        }
      }
      tok = (Tok){Undecided};
    }
    else tok = (Tok){Success, unlikely (suc == '=') ? 2 : 1};
    end: break;

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
    return ret(ctx, Identifier, identifier(ctx));

    case ' ': case '\t':
    case '\n': case '\r':
    return ret(ctx, Whitespace, whitespace(ctx));

    case '"':
    return ret(ctx, String, string(ctx));

    case '\'':
    return ret(ctx, Character, character(ctx));

    case ',': case ';':
    case '(': case ')':
    case '[': case ']':
    case '{': case '}':
    case ':':
    return ret(ctx, Punctuation, punctuation_short(ctx));

    case '!': case '%':
    case '<': case '>':
    case '=': case '?':
    case '*': case '/':
    case '+': case '-':
    case '.': case '^':
    case '&': case '|':
    return ret(ctx, Punctuation, punctuation_long(ctx));

    default:
    return ret(ctx, Undefined, (Tok){Fail});

  }

}
