// ## Prologue
//
// This is a simple and general lexer function for c-like languages.
// The main design goals are as follows:
//
// 1. To provide a very readable implementation of a lexer function.
// 2. To be useful in sequential work-flows, as well as systems working with
//    asynchronous IO and / or threads.
// 3. To be completely independent from any external libraries.
// 4. To perform all data dependent computations as fast as possible and leave
//    the rest to other software components.
// 5. To maximize the mean throughput by optimizing the code-paths for small
//    tokens.
// 6. To use no heap space by utilizing continuation-passing-style and
//    performing allocations solely on the stack.

#include <sys/types.h>
#include <stdbool.h>

// Type-declarations and interfaces of this lexer are found in the `lex.h`
// header file.

#include "lex.h"

// In order to optimize code-paths, we define two convenience macros to give
// branch-prediction information to the compiler.

#define likely(x)   (__builtin_expect(x, true))
#define unlikely(x) (__builtin_expect(x, false))

// ## Utils
//
// To reduce dependencies and allow the compiler to produce more efficient
// code through inlining, we provide implementations of some standard functions.

static inline bool ut_isalnum (char c) {
  return ((c >= 'A') & (c <= 'Z')) | ((c >= 'a') & (c <= 'z'))
       | ((c >= '0') & (c <= '9')) | (c == '_');
}

static inline bool ut_isspace (char c) {
  return (c == ' ') | (c == '\t') | (c == '\n') | (c == '\r');
}

// ## Lexemes
//
// To provide good readability and understandability, the functionality of the
// lexer as a whole is factored into several matchers. Each of those is
// provided as a static function that scans the buffer it is passed for a
// specific configurable pattern and writing the match into a provided token-
// buffer.

// ### Matcher: Number
// Stub.

static inline State nu (Tok *tok, size_t sz, char *buf) {

  size_t len;
  for (len = 1; len < sz; len++) {
    if (buf[len] < '0' || buf[len] > '9') {
      return (*tok = (Tok){Number, 0, len}, Success);
    }
  }
  return (*tok = (Tok){Number}, Undecided);

}

// ### Matcher: Alpha
// The alpha-matcher accepts characters from the input buffer,
// as long as they match with the supplied checking-function.

static inline State alpha (Tok *tok, size_t sz, char *buf, int type, bool (*check)(char)) {

  size_t len;

  if (check(buf[1]) == 0)
    return (*tok = (Tok){type, 0, 1}, Success);

  if (check(buf[2]) == 0)
    return (*tok = (Tok){type, 0, 2}, Success);

  if (check(buf[3]) == 0)
    return (*tok = (Tok){type, 0, 3}, Success);

  for (len = 4; len < sz; len++) {
    if (check(buf[len]) == 0) return (*tok = (Tok){type, 0, len}, Success);
  }

  return (*tok = (Tok){type}, Undecided);

}

// ### Matcher: Tau
// The tau matcher accepts characters from the input-buffer until it sees an
// unescaped termination character.
// It also accepts a parameter `plus` which indicates whether the terminator
// should be part of the tokens length.
// This function is currently used to match string- and character-literals as
// well as preprocessor directives.

static inline State tau (Tok *tok, size_t sz, char *buf, int type, int plus, char termn) {

  size_t len;
  bool escape = false;
  char b = buf[1], c = buf[2], d = buf[3];

  if (b == termn)
    return (*tok = (Tok){type, 0, 1 + plus}, Success);

  if (c == termn && b != '\\')
    return (*tok = (Tok){type, 0, 2 + plus}, Success);

  if (d == termn && c != '\\')
    return (*tok = (Tok){type, 0, 3 + plus}, Success);

  if (d == termn && c == '\\'  && b == '\\')
    return (*tok = (Tok){type, 0, 3 + plus}, Success);

  for (len = 1; len < sz; len++) {
    if (escape == false) {
      if (buf[len] == termn) return (*tok = (Tok){type, 0, len + plus}, Success);
      if (buf[len] == '\\')  escape = true;
    }
    else escape = false;
  }

  return (*tok = (Tok){type}, Undecided);

}

// ### Matcher: Punctuation
// In this function we switch on the first character of the buffer,
// and then perform further checks to determine the type of punctuation.
// Because the switch statement uses falltrough-techniques, we'll explain
// it's functionality in greater extend.
//
// In the first part of the switch statement, we match the arrow (`->`) as
// well as all punctuation that potentially may repeat itself or be followed
// by an equal-sign to form a multicharacter-punctuation.
//
// The second part matches the teriary-expression punctuation `?` and `?:`.
//
// After that, all monocharacter punctuation is matched.
//
// The last block checks if `/` stands for itself, is part of an `/=` or
// introduces a comment.
//
// Since at the end of the switch-statement all valid punctuation should have
// been matched and returned from the function, we can declare the following
// codepath as undefined.

static inline State pi (Tok *tok, size_t sz, char *buf) {

  char a = buf[0], b = buf[1], c = buf[2];

  switch (a) {

    case '-':
    if unlikely (b == '>')
      return (*tok = (Tok){Punctuation, 0, 2}, Success);
    case '&': case '<': case '>':
    case '|': case '+':
    if unlikely (b == a)
      return (*tok = (Tok){Punctuation, 0, 2 + ((a == '<' | a == '>') & (c == '='))}, Success);
    case '^': case '=': case '*':
    case '%': case '!':
    return (*tok = (Tok){Punctuation, 0, 1 + (b == '=')}, Success);

    case '?':
    return (*tok = (Tok){Punctuation, 0, 1 + (b == ':')}, Success);

    case '(': case ')': case '[': case ']': case '.':
    case '{': case '}': case ':': case ';': case ',': case '~':
    return (*tok = (Tok){Punctuation, 0, 1 + 2 * (a == '.' & b == '.' & c == '.')}, Success);

    case '/':
    if unlikely (b == '/') return tau(tok, sz - 2, &buf[2], Punctuation, 2, '\n');
    else                   return (*tok = (Tok){Punctuation, 0, 1 + (b == '=')}, Success);

    default: __builtin_unreachable();

  }

}

// ## Main

// ### Classification & Dispatch

// We classify an ASCII-character into one of seven categories, depending on
// which type of token it may introduce. We use this information later to
// dispatch into subfunctions.

#define STRING      case '"'
#define CHARACTER   case '\''
#define DIRECTIVE   case '#'
#define NUMBER      case '0'...'9'
#define WHITESPACE  case ' ': case '\t': case '\n': case '\r'
#define IDENTIFIER  case 'A'...'Z': case 'a'...'z': case '_'
#define PUNCTUATION case '!': case '%': case '&':   \
                    case '('...'/': case ':'...'?': \
                    case '['...'^': case '{'...'~'

// Based on the first character of the input buffer, we route the
// tokenization process to a specific lexeme function.
// We require that the length of our buffer is at least four characters, else
// the behaviour of this function is undefined.

static inline State dispatch (Tok *tok, size_t sz, char *buf) {

  if (sz < 4) {
    __builtin_unreachable();
  }

  switch (buf[0]) {

    STRING:      return tau(tok, sz, buf, String,    1, '"');
    CHARACTER:   return tau(tok, sz, buf, Character, 1, '\'');
    DIRECTIVE:   return tau(tok, sz, buf, Directive, 0, '\n');
    NUMBER:      return nu(tok, sz, buf);
    WHITESPACE:  return alpha(tok, sz, buf, Whitespace, ut_isspace);
    IDENTIFIER:  return alpha(tok, sz, buf, Identifier, ut_isalnum);
    PUNCTUATION: return pi(tok, sz, buf);
    default:     return (buf[0] ? Fail : End);

  }

}

// ### Lex

int lex (Ctx *const ctx, Tok *toks, int len) {

  State s;
  int num;

  #ifdef BENCH
  unsigned long long t0 = 0, t1 = 0;
  #endif

  for (num = 0; num < len; num++) {

    #ifdef BENCH
    t0 = __builtin_readcyclecounter();
    #endif

    ctx->state = (ctx->sz >= 4) ? dispatch(&toks[num], ctx->sz, &ctx->buf[ctx->off]) : Undecided;

    #ifdef BENCH
    t1 = __builtin_readcyclecounter();
    toks[num].t = t1 - t0;
    #endif

    if (ctx->state == Success) {
      toks[num].off = ctx->off;
      ctx->off  += toks[num].len;
      ctx->sz   -= toks[num].len;
    }

    else break;

  }

  return num;

}
