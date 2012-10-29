// ## Fundamental Types


// ### Context Structure
// The context of our lexer includes two kinds of buffers:
// A variable frontbuffer, which acts as a window to the constant backbuffer
// and changes in size and position as we process the contents of the
// latter.

typedef struct {
  size_t sz, back_sz;
  char *buf, *back_buf;
} Ctx;


// ### Token Types
// We operate on a small set of tokens types.
// Keywords, typenames and identifiers are summarized under the
// `IdentifierType` token type.
// All kinds of punctuations have a common type,
// comments are also understood as punctuation.

typedef enum {
  UndefinedType,
  NumberType,
  IdentifierType,
  WhitespaceType,
  StringType,
  CharacterType,
  PunctuationType
} Type;


// ### Token Structure
// The emitted tokens include a success-indicating state and an optional
// length.

typedef struct {
  enum {
    Success, Fail,
    Undecided, End
  } state;
  size_t len;
} Tok;


// ### Continuation Type
// Since we'll program in Continuation-Passing-Style, we define
// the type of our continuation.

typedef void (*Cont)
  (Ctx *const,
   const Type,
   const Tok);



// ## Interface

void route(Ctx *const ctx, const Cont ret);