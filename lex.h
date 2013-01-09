// ## Prologue
//
// This is the interface to a simple and general lexer function for
// c-like languages.
// See the source file `lex.c` for more information.

#ifndef NS
#define NS(id) id
#endif

// ## Fundamental Types

// ### Context Structure
// The context of our lexer includes two kinds of buffers:
// A variable frontbuffer, which acts as a window to the constant
// backbuffer and changes in size and position as we process the
// contents of the latter.

typedef enum {
  NS(Success), NS(Fail),
  NS(Undecided), NS(End)
} NS(State);

typedef struct {
  char *buf;
  size_t sz, cap, off;
  NS(State) state;
} NS(Ctx);

// ### Token Structure
// The emitted tokens include a type a success-indicating state and an
// optional length.
// We operate on a small set of tokens types.
// Keywords, typenames and identifiers are summarized under the
// `Identifier` token type. All kinds of punctuations have a common
// type, comments are also understood as punctuation.

typedef struct {
  enum {
    NS(Undefined),
    NS(Number),
    NS(Identifier),
    NS(Whitespace),
    NS(String),
    NS(Character),
    NS(Punctuation),
    NS(Directive)
  } type;
  size_t off;
  size_t len;
#ifdef BENCH
  unsigned long long int t;
#endif
} NS(Tok);

// ## Interface

int lex(NS(Ctx) *const, NS(Tok) *, int);

#undef NS
