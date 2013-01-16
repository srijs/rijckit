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
#include "lex.h"

// ## Utils

// In order to optimize code-paths, we define two convenience macros to give
// branch-prediction information to the compiler.

#define likely(x)   (__builtin_expect(x, true))
#define unlikely(x) (__builtin_expect(x, false))

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

static inline State nu (Tok *tok, size_t sz, const char *buf) {

  size_t len;
  for (len = 1; len < sz; len++) {
    if ((buf[len] < '0') | (buf[len] > '9')) {
      return (tok->len = len, Success);
    }
  }
  return Undecided;

}

// ### Matcher: Alpha
//
// The alpha-matcher accepts characters from the input buffer,
// as long as they match with the supplied checking-function.

static inline State alpha (Tok *tok, size_t sz, const char *buf, bool (*check)(char)) {

  size_t len;

  if (check(buf[1]) == 0)
    return (tok->len = 1, Success);

  if (check(buf[2]) == 0)
    return (tok->len = 2, Success);

  if (check(buf[3]) == 0)
    return (tok->len = 3, Success);

  for (len = 4; len < sz; len++) {
    if (check(buf[len]) == 0) return (tok->len = len, Success);
  }

  return Undecided;

}

// ### Matcher: Tau
//
// The tau matcher accepts characters from the input-buffer until it sees an
// unescaped termination character.
// It also accepts a parameter `plus` which indicates whether the terminator
// should be part of the tokens length.
// This function is currently used to match string- and character-literals as
// well as preprocessor directives.

static inline State tau (Tok *tok, size_t sz, const char *buf, int plus, char termn) {

  size_t len;
  bool escape = false;
  char b = buf[1], c = buf[2], d = buf[3];

  if (b == termn)
    return (tok->len = 1 + plus, Success);

  if (c == termn && b != '\\')
    return (tok->len = 2 + plus, Success);

  if (d == termn && c != '\\')
    return (tok->len = 3 + plus, Success);

  if (d == termn && c == '\\'  && b == '\\')
    return (tok->len = 3 + plus, Success);

  for (len = 1; len < sz; len++) {
    if (escape == false) {
      if (buf[len] == termn) return (tok->len = len + plus, Success);
      if (buf[len] == '\\')  escape = true;
    }
    else escape = false;
  }

  return Undecided;

}

// ### Matcher: Punctuation
//
// In this function we switch on the first character of the buffer,
// and then perform further checks to determine the type of punctuation.
// Because the switch statement uses falltrough-techniques, we'll explain
// it's functionality in greater extend.
//
// 1. In the first part of the switch statement, we match the arrow (`->`) as
//    well as all punctuation that potentially may repeat itself or be followed
//    by an equal-sign to form a multicharacter-punctuation.
// 2. The second part matches the teriary-expression punctuation `?` and `?:`.
// 3. After that, the ellipsis and all monocharacter punctuation is matched.
// 4. The last block checks if `/` stands for itself, is part of an `/=` or
//    introduces a comment.
// 5. Since at the end of the switch-statement all valid punctuation should have
//    been matched and returned from the function, we can declare the following
//    codepath as undefined.

#define PI_ARROW     case '-'
#define PI_REPEAT    case '&': case '<': case '>': case '|': case '+'
#define PI_EQ_FOLLOW case '^': case '=': case '*': case '%': case '!'
#define PI_TERTIARY  case '?'
#define PI_DOT       case '.'
#define PI_MONO      case '(': case ')': case '[': case ']': case '~': \
                     case '{': case '}': case ':': case ';': case ','
#define PI_SLASH     case '/'

static inline State pi (Tok *tok, size_t sz, const char *buf) {

  char a = buf[0], b = buf[1], c = buf[2];

  switch (a) {
    PI_ARROW:     if unlikely (b == '>') return (tok->len = 2, Success);
    PI_REPEAT:    if unlikely (b == a)   return (tok->len = 2 + ((a == '<' | a == '>') & (c == '=')), Success);
    PI_EQ_FOLLOW: return (tok->len = 1 + (b == '='), Success);
    PI_TERTIARY:  return (tok->len = 1 + (b == ':'), Success);
    PI_DOT:       if unlikely (b == '.' & c == '.') return (tok->len = 3, Success);
    PI_MONO:      return (tok->len = 1, Success);
    PI_SLASH:     if unlikely (b == '/') return tau(tok, sz - 2, &buf[2], 2, '\n');
                  else                   return (tok->len = 1 + (b == '='), Success);
    default:      __builtin_unreachable();
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

static inline State dispatch (Tok *tok, size_t sz, const char *buf) {

  if (sz < 4) __builtin_unreachable();

  switch (buf[0]) {
    STRING:      {tok->type = String;      return tau(tok, sz, buf, 1, '"'); }
    CHARACTER:   {tok->type = Character;   return tau(tok, sz, buf, 1, '\''); }
    DIRECTIVE:   {tok->type = Directive;   return tau(tok, sz, buf, 0, '\n'); }
    NUMBER:      {tok->type = Number;      return nu(tok, sz, buf); }
    WHITESPACE:  {tok->type = Whitespace;  return alpha(tok, sz, buf, ut_isspace); }
    IDENTIFIER:  {tok->type = Identifier;  return alpha(tok, sz, buf, ut_isalnum); }
    PUNCTUATION: {tok->type = Punctuation; return pi(tok, sz, buf); }
    default:     return (buf[0] ? Fail : End);
  }

}

// ### Lex

int lex (Ctx * ctx, Tok *toks, int len) {

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