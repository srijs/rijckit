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

typedef struct {
  size_t sz, back_sz;
  unsigned char *buf, *back_buf;
} NS(Ctx);


// ### Token Types
// We operate on a small set of tokens types.
// Keywords, typenames and identifiers are summarized under the
// `Identifier` token type. All kinds of punctuations have a common
// type, comments are also understood as punctuation.

typedef enum {
  NS(Undefined),
  NS(Number),
  NS(Identifier),
  NS(Whitespace),
  NS(String),
  NS(Character),
  NS(Punctuation)
} NS(Type);


// ### Token Structure
// The emitted tokens include a success-indicating state and an
// optional length.

typedef struct {
  enum {
    NS(Success), NS(Fail),
    NS(Undecided), NS(End)
  } state;
  size_t len;
} NS(Tok);


// ### Continuation Type
// Since we'll program in Continuation-Passing-Style, we define the
// type of our continuation.

typedef void (*NS(Cont))
  (NS(Ctx) *const,
   const NS(Type),
   const NS(Tok));



// ## Interface

void lex(NS(Ctx) *const, const NS(Cont));



#undef NS
