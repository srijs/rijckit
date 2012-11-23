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
      return (Tok){Number, Success, len};
    }
  }
  return (Tok){Number, Undecided};

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
  return ((c >= 'A') & (c <= 'Z')) | ((c >= 'a') & (c <= 'z'))
       | ((c >= '0') & (c <= '9')) | (c == '_');
}

static inline bool is_whitespace (char c) {
  return (c == ' ') | (c == '\t') | (c == '\n') | (c == '\r');
}

static inline Tok identifier_or_whitespace (int type, Ctx *const ctx) {

  bool (*check)(char) = (bool (*[])(char)) {[Whitespace] = is_whitespace,
                                            [Identifier] = is_alnum} [type];
  size_t len;

  if (check(ctx->buf[1]) == 0)
    return (Tok){type, Success, 1};

  if (check(ctx->buf[2]) == 0)
    return (Tok){type, Success, 2};

  if (check(ctx->buf[3]) == 0)
    return (Tok){type, Success, 3};

  for (len = 4; len < ctx->sz; len++) {
    if (check(ctx->buf[len]) == 0) return (Tok){type, Success, len};
  }

  return (Tok){type, Undecided};

}

// ### Lexeme: String, Character & Pre-Processor Directive
//
// `string    ::= Dbl-Quote ( Char-Seq ) Dbl-Quote`
//
// `character ::= S-Quote   ( Char-Seq ) S-Quote`
//
// `directive ::= Hash      ( Seq      ) Newline`
//
// A string literal is a string of ASCII-identifier between
// two double quotes.
//
// A character literal is a string of ASCII-identifier between
// two single quotes.
//
// A preprocessor directive is introduced by a hash character (`#`)
// and end with an unescaped newline.

static inline Tok str_or_char_or_pp (int type, Ctx *const ctx) {

  const int  plus  = (type == String || type == Character) ? 1 : 0;
  const char termn = (char[]) {[String] = '"', [Character] = '\'',
                               [Directive] = '\n'} [type];
  size_t len;
  bool   escape = false;

  if (ctx->buf[1] == termn)
    return (Tok){type, Success, 1 + plus};

  if (ctx->buf[2] == termn &&
      ctx->buf[1] != '\\')
    return (Tok){type, Success, 2 + plus};

  if (ctx->buf[3] == termn &&
      ctx->buf[2] != '\\')
    return (Tok){type, Success, 3 + plus};

  if (ctx->buf[3] == termn &&
      ctx->buf[2] == '\\'  &&
      ctx->buf[1] == '\\')
    return (Tok){type, Success, 3 + plus};

  for (len = 1; len < ctx->sz; len++) {
    if (escape == false) {
      if (ctx->buf[len] == termn) return (Tok){type, Success, len + plus};
      if (ctx->buf[len] == '\\')  escape = true;
    }
    else escape = false;
  }

  return (Tok){type, Undecided};

}

// ### Lexeme: Punctuation

static inline Tok punctuation (Ctx *const ctx) {

  size_t len;
  char a = ctx->buf[0], b = ctx->buf[1], c = ctx->buf[2];

  switch (ctx->buf[0]) {

    // arrow?
    case '-':
    if unlikely (b == '>')
      return (Tok){Punctuation, Success, 2};

    // repeats itself?
    case '&': case '<': case '>':
    case '|': case '+':
    if unlikely (b == a)
      return (Tok){Punctuation, Success, 2 + ((a == '<' | a == '>') &
                                              (c == '='))};
    // equal follows?
    case '^': case '=': case '*':
    case '%': case '!':
    return (Tok){Punctuation, Success, 1 + (b == '=')};

    case '?':
    return (Tok){Punctuation, Success, 1 + (b == ':')};

    case '(': case ')': case '[': case ']': case '.':
    case '{': case '}': case ':': case ';': case ',': case '~':
    return (Tok){Punctuation, Success, 1 + 2 * (a == '.' &
                                                b == '.' &
                                                c == '.')};
    // Is comment or divide-equals?
    case '/':
    if unlikely (b == '/') {
      for (len = 2; len < ctx->sz; len++) {
        if unlikely (ctx->buf[len] == '\n' | ctx->buf[len] == '\r') {
          return (Tok){Punctuation, Success, len};
        }
      }
      return (Tok){Punctuation, Undecided};
    }
    return (Tok){Punctuation, Success, unlikely (b == '=') ? 2 : 1};

  }

  // Since we have handled all possible cases, we can declare the
  // following codepath as undefined.
  __builtin_unreachable();

}



// ## Routing

void lex (Ctx *const ctx, const Cont ret) {

  // Based on the first character of the input buffer, we route the
  // tokenization process to a specific lexeme.
  // We require that the length of our buffer is at least four
  // characters, else the behaviour of this function is undefined.

  if (ctx->sz < 4) __builtin_unreachable();

  switch (ctx->buf[0]) {

    case '\0': return ret(ctx, (Tok){Undefined, End});

    case '0'...'9': return ret(ctx, number(ctx));

    case 'A'...'Z':
    case 'a'...'z': case '_':
    return ret(ctx, identifier_or_whitespace(Identifier, ctx));

    case ' ':  case '\t':
    case '\n': case '\r':
    return ret(ctx, identifier_or_whitespace(Whitespace, ctx));

    case '"':  return ret(ctx, str_or_char_or_pp(String,    ctx));
    case '\'': return ret(ctx, str_or_char_or_pp(Character, ctx));
    case '#':  return ret(ctx, str_or_char_or_pp(Directive, ctx));

    case ':': case '~': case '!': case '%':
    case '<': case '>': case '=': case '?':
    case '*': case '/': case '+': case '-':
    case '.': case '^': case '&': case '|':
    case ',': case ';': case '(': case ')':
    case '[': case ']': case '{': case '}':
    return ret(ctx, punctuation(ctx));

    default: return ret(ctx, (Tok){Undefined, Fail});

  }

}
