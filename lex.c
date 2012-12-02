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
// process as a parameter and returning the matched token structure.

// ### Lexeme: Number
// Stub.

static inline Tok nu (Ctx *const ctx) {

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

static inline Tok alpha (Ctx *const ctx, int type, bool (*check)(char)) {

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
// A string literal is a string of ASCII-identifier between two double quotes.
//
// A character literal is a string of ASCII-identifier between two single quotes.
//
// A preprocessor directive is introduced by a hash character (`#`) and end with
// an unescaped newline.

static inline Tok tau (Ctx *const ctx, int type, int plus, char termn) {

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

static inline Tok pi (Ctx *const ctx) {

  size_t len;
  char a = ctx->buf[0],
       b = ctx->buf[1],
       c = ctx->buf[2];

  switch (ctx->buf[0]) {

    case '-':
    if unlikely (b == '>')
      return (Tok){Punctuation, Success, 2};
    case '&': case '<': case '>':
    case '|': case '+':
    if unlikely (b == a)
      return (Tok){Punctuation, Success, 2 + ((a == '<' | a == '>') & (c == '='))};
    case '^': case '=': case '*':
    case '%': case '!':
    return (Tok){Punctuation, Success, 1 + (b == '=')};

    case '?':
    return (Tok){Punctuation, Success, 1 + (b == ':')};

    case '(': case ')': case '[': case ']': case '.':
    case '{': case '}': case ':': case ';': case ',': case '~':
    return (Tok){Punctuation, Success, 1 + 2 * (a == '.' & b == '.' & c == '.')};

    case '/':
    if unlikely (b == '/') {
      for (len = 2; len < ctx->sz; len++)
        if unlikely (ctx->buf[len] == '\n' | ctx->buf[len] == '\r')
          return (Tok){Punctuation, Success, len};
      return (Tok){Punctuation, Undecided};
    }
    return (Tok){Punctuation, Success, 1 + (b == '=')};

  }

  __builtin_unreachable();

}

// ## Routing

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

static int classify (char c) {

  switch (c) {
    case '"':                                  return String;
    case '\'':                                 return Character;
    case '#':                                  return Directive;
    case '0'...'9':                            return Number;
    case ' ': case '\t': case '\n': case '\r': return Whitespace;
    case 'A'...'Z': case 'a'...'z': case '_':  return Identifier;
    case ':': case '~': case '!': case '%':
    case '<': case '>': case '=': case '?':
    case '*': case '/': case '+': case '-':
    case '.': case '^': case '&': case '|':
    case ',': case ';': case '(': case ')':
    case '[': case ']': case '{': case '}':    return Punctuation;
    default:                                   return Undefined;
  }

}

// Based on the first character of the input buffer, we route the
// tokenization process to a specific lexeme function.
// We require that the length of our buffer is at least four characters, else
// the behaviour of this function is undefined.
//
// <script type="math/tex">
// \lambda (s, n) :=
//   \begin{cases}
//     \text{undefined}                             &\mbox{if } n < 4       \\
//     \tau   (s, n, \text{String},    1, T_S)      &\mbox{if } s_0 = T_S   \\
//     \tau   (s, n, \text{Character}, 1, T_C)      &\mbox{if } s_0 = T_C   \\
//     \tau   (s, n, \text{Directive}, 0, T_D)      &\mbox{if } s_0 = T_D   \\
//     \nu    (s, n, \text{Number})                 &\mbox{if } s_0 \in N   \\
//     \alpha (s, n, \text{Whitespace}, A_S)        &\mbox{if } s_0 \in A_S \\
//     \alpha (s, n, \text{Identifier}, A_I \cup N) &\mbox{if } s_0 \in A_I \\
//     \pi    (s, n, \text{Punctuation})            &\mbox{if } s_0 \in P   \\
//     \langle\text{Undefined},\text{End},0\rangle  &\mbox{if } s_0 = 0     \\
//     \langle\text{Undefined},\text{Fail},0\rangle &\mbox{otherwise}
//   \end{cases}
// </script>

void lex (Ctx *const ctx, const Cont ret) {

  if (ctx->sz < 4) __builtin_unreachable();

  switch (classify(ctx->buf[0])) {
    case String:      return ret(ctx, tau(ctx, String,    1, '"'));
    case Character:   return ret(ctx, tau(ctx, Character, 1, '\''));
    case Directive:   return ret(ctx, tau(ctx, Directive, 0, '\n'));
    case Number:      return ret(ctx, nu(ctx));
    case Whitespace:  return ret(ctx, alpha(ctx, Whitespace, is_whitespace));
    case Identifier:  return ret(ctx, alpha(ctx, Identifier, is_alnum));
    case Punctuation: return ret(ctx, pi(ctx));
    case Undefined:   return ret(ctx, (Tok){.state = ctx->buf[0] ? Fail : End});
  }

}

// <script type="text/javascript"
//         src="http://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS_HTML">
// </script>
