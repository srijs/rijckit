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

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// Type-declarations and interfaces of this lexer are found in the `lex.h`
// header file.

#include "lex.h"

// In order to optimize code-paths, we define two convenience macros to give
// branch-prediction information to the compiler.

#define likely(x)   (__builtin_expect(x, true))
#define unlikely(x) (__builtin_expect(x, false))

// ## Lexemes
//
// To provide good readability and understandability, the functionality of the
// lexer as a whole is factored into its individual lexemes. Each of those is
// provided as a static function, taking the current context of the lexing
// process as a parameter, writing the matched token structure and returning
// its state.

// ### Lexeme: Number
// Stub.

static inline State nu (Tok *tok, size_t sz, char *buf) {

  size_t len;
  for (len = 1; len < sz; len++) {
    if (buf[len] < '0' || buf[len] > '9') {
      return (*tok = (Tok){Number, buf, len}, Success);
    }
  }
  return (*tok = (Tok){Number}, Undecided);

}

// ### Lexeme: Identifier & Whitespace
//
// `identifier ::= ( A-Z | a-z | _ ) { ( A-z | a-z | 0-9 | _ ) }`
//
// `whitespace ::= ( Space | Tab | NL | CR ) { whitespace }`
//
// An identifier is an _arbitrary long_ sequence of alphanumerics and underscore
// characters.
//
// Whitespace is an _arbitrary long_ sequence of space, tab, new-line and
// carriage-return characters.

static inline bool is_alnum (char c) {
  return ((c >= 'A') & (c <= 'Z')) | ((c >= 'a') & (c <= 'z'))
       | ((c >= '0') & (c <= '9')) | (c == '_');
}

static inline bool is_whitespace (char c) {
  return (c == ' ') | (c == '\t') | (c == '\n') | (c == '\r');
}

static inline State alpha (Tok *tok, size_t sz, char *buf, int type, bool (*check)(char)) {

  size_t len;

  if (check(buf[1]) == 0)
    return (*tok = (Tok){type, buf, 1}, Success);

  if (check(buf[2]) == 0)
    return (*tok = (Tok){type, buf, 2}, Success);

  if (check(buf[3]) == 0)
    return (*tok = (Tok){type, buf, 3}, Success);

  for (len = 4; len < sz; len++) {
    if (check(buf[len]) == 0) return (*tok = (Tok){type, buf, len}, Success);
  }

  return (*tok = (Tok){type}, Undecided);

}

// ### Lexeme: String, Character & Pre-Processor Directive
//
// `string    ::= Dbl-Quote ( Char-Seq ) Dbl-Quote`
//
// `character ::= S-Quote   ( Char-Seq ) S-Quote`
//
// `directive ::= Hash      ( Seq      ) Newline`
//
// A string literal is a string of ASCII-identifier between two double quotes.
//
// A character literal is a string of ASCII-identifier between two single quotes.
//
// A preprocessor directive is introduced by a hash character (`#`) and end with
// an unescaped newline.

static inline State tau (Tok *tok, size_t sz, char *buf, int type, int plus, char termn) {

  size_t len;
  bool escape = false;
  char b = buf[1], c = buf[2], d = buf[3];

  if (b == termn)
    return (*tok = (Tok){type, buf, 1 + plus}, Success);

  if (c == termn && b != '\\')
    return (*tok = (Tok){type, buf, 2 + plus}, Success);

  if (d == termn && c != '\\')
    return (*tok = (Tok){type, buf, 3 + plus}, Success);

  if (d == termn && c == '\\'  && b == '\\')
    return (*tok = (Tok){type, buf, 3 + plus}, Success);

  for (len = 1; len < sz; len++) {
    if (escape == false) {
      if (buf[len] == termn) return (*tok = (Tok){type, buf, len + plus}, Success);
      if (buf[len] == '\\')  escape = true;
    }
    else escape = false;
  }

  return (*tok = (Tok){type}, Undecided);

}

// ### Lexeme: Punctuation
//
// In this function we switch on the first character of the buffer,
// and then perform further checks to determine the type of punctuation.
// Because the switch statement uses falltrough-techniques, we'll explain
// it's functionality carefully.
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
      return (*tok = (Tok){Punctuation, buf, 2}, Success);
    case '&': case '<': case '>':
    case '|': case '+':
    if unlikely (b == a)
      return (*tok = (Tok){Punctuation, buf, 2 + ((a == '<' | a == '>') & (c == '='))}, Success);
    case '^': case '=': case '*':
    case '%': case '!':
    return (*tok = (Tok){Punctuation, buf, 1 + (b == '=')}, Success);

    case '?':
    return (*tok = (Tok){Punctuation, buf, 1 + (b == ':')}, Success);

    case '(': case ')': case '[': case ']': case '.':
    case '{': case '}': case ':': case ';': case ',': case '~':
    return (*tok = (Tok){Punctuation, buf, 1 + 2 * (a == '.' & b == '.' & c == '.')}, Success);

    case '/':
    if unlikely (b == '/') return tau(tok, sz - 2, &buf[2], Punctuation, 2, '\n');
    else                   return (*tok = (Tok){Punctuation, buf, 1 + (b == '=')}, Success);

    default: __builtin_unreachable();

  }

}

// ## Main

// ### Classification
//
// We classify an ASCII-character into one of eight categories, depending on
// which type of token it may introduce. We use this information later to
// dispatch into subfunctions.
//
// <script type="math/tex">
// \begin{array}{lrl}
//   T_S &:=& 34 \in \text{ASCII} \\
//   T_C &:=& 39 \in \text{ASCII} \\
//   T_D &:=& 35 \in \text{ASCII} \\
//   N   &:=& \{a \in \text{ASCII} : \text{$a$ is numerical}\}    = [48,57] \\
//   A_W &:=& \{a \in \text{ASCII} : \text{$a$ is whitespace}\}   = \{9,10,13,32\} \\
//   A_I &:=& \{a \in \text{ASCII} : \text{$a$ is alphabetical}\} \\
//        &=& [65, 122] \setminus \{91,92,93,94,96\} \\
//   P   &:=& \{a \in \text{ASCII} : \text{$a$ introduces punctuation}\} \\
//        &=& \{33,37,38\} \cup [40,47] \cup [58,63] \cup [91,94] \cup [123,126] \\
//   U   &:=& \text{ASCII} \setminus (T_S \cup T_C \cup T_D \cup N \cup A_W \cup A_I \cup P)
// \end{array}
// </script>

static inline int classify (char c) {

  switch (c) {

    case '"':                       return String;
    case '\'':                      return Character;
    case '#':                       return Directive;
    case '0'...'9':                 return Number;
    case ' ': case '\t':
    case '\n': case '\r':           return Whitespace;
    case 'A'...'Z':
    case 'a'...'z': case '_':       return Identifier;
    case '!': case '%': case '&':
    case '('...'/': case ':'...'?':
    case '['...'^': case '{'...'~': return Punctuation;
    default:                        return Undefined;

  }

}

// ### Dispatch
//
// Based on the first character of the input buffer, we route the
// tokenization process to a specific lexeme function.
// We require that the length of our buffer is at least four characters, else
// the behaviour of this function is undefined.
//
// <script type="math/tex">
// \delta (n, s) :=
//   \begin{cases}
//     \text{undefined}                             &\mbox{if } n < 4       \\
//     \tau   (n, s, \text{String},    1, T_S)      &\mbox{if } s_0 = T_S   \\
//     \tau   (n, s, \text{Character}, 1, T_C)      &\mbox{if } s_0 = T_C   \\
//     \tau   (n, s, \text{Directive}, 0, T_D)      &\mbox{if } s_0 = T_D   \\
//     \nu    (n, s, \text{Number})                 &\mbox{if } s_0 \in N   \\
//     \alpha (n, s, \text{Whitespace}, A_S)        &\mbox{if } s_0 \in A_S \\
//     \alpha (n, s, \text{Identifier}, A_I \cup N) &\mbox{if } s_0 \in A_I \\
//     \pi    (n, s, \text{Punctuation})            &\mbox{if } s_0 \in P   \\
//     \langle\text{Undefined},\text{End},0\rangle  &\mbox{if } s_0 = 0     \\
//     \langle\text{Undefined},\text{Fail},0\rangle &\mbox{otherwise}
//   \end{cases}
// </script>

static inline State dispatch (Tok *tok, size_t sz, char *buf) {

  if (sz < 4) {
    __builtin_unreachable();
  }

  switch (classify(buf[0])) {

    case String:      return tau(tok, sz, buf, String,    1, '"');
    case Character:   return tau(tok, sz, buf, Character, 1, '\'');
    case Directive:   return tau(tok, sz, buf, Directive, 0, '\n');
    case Number:      return nu(tok, sz, buf);
    case Whitespace:  return alpha(tok, sz, buf, Whitespace, is_whitespace);
    case Identifier:  return alpha(tok, sz, buf, Identifier, is_alnum);
    case Punctuation: return pi(tok, sz, buf);
    case Undefined:   return (buf[0] ? Fail : End);
    default:          __builtin_unreachable();

  }

}

// ### Lex

Return lex (Ctx *const ctx, Tok *token) {

  unsigned long long t0 = 0, t1 = 0;

  State s = Undecided;

  if (ctx->sz >= 4) {

    #ifdef BENCH
      t0 = __builtin_readcyclecounter();
    #endif

    s = dispatch(token, ctx->sz, ctx->buf);

    #ifdef BENCH
      t1 = __builtin_readcyclecounter();
    #endif

  }

  if (s == Undecided) {
    memmove(ctx->back_buf, ctx->buf, ctx->sz);
    ctx->buf = ctx->back_buf;
  }

  if (s == Success) {
    ctx->buf += token->len;
    ctx->sz  -= token->len;
  }

  return (Return){s, t1 - t0};

}

// <script type="text/javascript"
//         src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS_HTML">
// </script>
