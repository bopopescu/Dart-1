// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/scanner.h"

#include "vm/assert.h"
#include "vm/flags.h"
#include "vm/object.h"
#include "vm/thread.h"
#include "vm/token.h"

namespace dart {

DEFINE_FLAG(bool, print_tokens, false, "Print scanned tokens.");
DECLARE_FLAG(bool, expose_core_impl);

void Scanner::InitKeywordTable() {
  for (int i = 0; i < Token::numKeywords; i++) {
    Token::Kind token = static_cast<Token::Kind>(Token::kFirstKeyword + i);
    keywords_[i].kind = token;
    keywords_[i].keyword_chars = Token::Str(token);
    keywords_[i].keyword_len = strlen(Token::Str(token));
    keywords_[i].keyword_symbol = NULL;
  }
}


Scanner::Scanner(const String& src, const String& private_key)
    : source_(src),
      source_length_(src.Length()),
      lookahead_pos_(0),
      token_start_(0),
      c0_(source_length_ == 0 ? '\0' : src.CharAt(0)),
      newline_seen_(false),
      saved_context_(NULL),
      string_delimiter_('\0'),
      string_is_multiline_(false),
      brace_level_(0),
      private_key_(String::ZoneHandle(private_key.raw())) {
  c0_pos_.line = 1;
  c0_pos_.column = 1;
  InitKeywordTable();
}

void Scanner::ErrorMsg(const char* msg) {
  current_token_.kind = Token::kERROR;
  current_token_.literal = &String::ZoneHandle(String::NewSymbol(msg));
  current_token_.position = c0_pos_;
  token_start_ = lookahead_pos_;
  current_token_.offset = lookahead_pos_;
}


void Scanner::PushContext() {
  ScanContext* ctx = new ScanContext;
  ctx->next = saved_context_;
  saved_context_ = ctx;
  ctx->string_delimiter = string_delimiter_;
  ctx->string_is_multiline = string_is_multiline_;
  ctx->brace_level = brace_level_;
  string_delimiter_ = '\0';
  string_is_multiline_ = false;
  brace_level_ = 1;  // Account for the opening ${ token.
}


void Scanner::PopContext() {
  ASSERT(saved_context_ != NULL);
  ASSERT(brace_level_ == 0);
  ASSERT(string_delimiter_ == '\0');
  ScanContext* ctx = saved_context_;
  saved_context_ = ctx->next;
  string_delimiter_ = ctx->string_delimiter;
  ASSERT(string_delimiter_ != '\0');
  string_is_multiline_ = ctx->string_is_multiline;
  brace_level_ = ctx->brace_level;
  delete ctx;
}


void Scanner::BeginStringLiteral(const char delimiter) {
  string_delimiter_ = delimiter;
}


void Scanner::EndStringLiteral() {
  string_delimiter_ = '\0';
  string_is_multiline_ = false;
}


bool Scanner::IsLetter(int32_t c) {
  return (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z'));
}


bool Scanner::IsDecimalDigit(int32_t c) {
  return '0' <= c && c <= '9';
}


bool Scanner::IsHexDigit(int32_t c) {
  return IsDecimalDigit(c)
         || (('A' <= c) && (c <= 'F'))
         || (('a' <= c) && (c <= 'f'));
}


bool Scanner::IsIdentStartChar(int32_t c) {
  return IsLetter(c) || (c == '_') || (c == '$');
}


bool Scanner::IsIdentChar(int32_t c) {
  return IsLetter(c) || IsDecimalDigit(c) || (c == '_') || (c == '$');
}


void Scanner::ReadChar() {
  if (lookahead_pos_ < source_length_) {
    if (c0_ == '\n') {
      newline_seen_ = true;
      c0_pos_.line++;
      c0_pos_.column = 0;
    }
    lookahead_pos_++;
    c0_pos_.column++;
    c0_ = LookaheadChar(0);
    // Replace '\r' with '\n' and a sequence of
    // '\r' '\n' with a single '\n'.
    if (c0_ == '\r') {
      c0_ = '\n';
      if (LookaheadChar(1) == '\n') {
        lookahead_pos_++;
      }
    }
  }
}


// Look ahead 'how_many' characters. Returns the character, or '\0' if
// the lookahead position is beyond the end of the string. Does not
// normalize line end characters into '\n'.
int32_t Scanner::LookaheadChar(int how_many) {
  ASSERT(how_many >= 0);
  int32_t lookahead_char = '\0';
  if (lookahead_pos_ + how_many < source_length_) {
    lookahead_char = source_.CharAt(lookahead_pos_ + how_many);
  }
  return lookahead_char;
}


void Scanner::ConsumeWhiteSpace() {
  while (c0_ == ' ' || c0_ == '\t' || c0_ == '\n') {
    ReadChar();
  }
}


void Scanner::ConsumeLineComment() {
  ASSERT(c0_ == '/');
  while (c0_ != '\n' && c0_ != '\0') {
    ReadChar();
  }
  ReadChar();
  current_token_.kind = Token::kWHITESP;
}


void Scanner::ConsumeBlockComment() {
  ASSERT(c0_ == '*');
  ReadChar();
  int nesting_level = 1;

  while (true) {
    const char c = c0_;
    ReadChar();
    if (c0_ == '\0') {
      break;
    }
    if (c == '/' && c0_ == '*') {
      nesting_level++;
      ReadChar();  // Consume asterisk.
    } else if (c == '*' && c0_ == '/') {
      nesting_level--;
      ReadChar();  // Consume slash.
      if (nesting_level == 0) {
        break;
      }
    }
  }
  current_token_.kind =
    (nesting_level == 0) ? Token::kWHITESP : Token::kILLEGAL;
}


void Scanner::ScanIdentChars(bool allow_dollar) {
  ASSERT(IsIdentStartChar(c0_));
  ASSERT(allow_dollar || (c0_ != '$'));
  int ident_length = 0;
  int ident_pos = lookahead_pos_;
  int32_t ident_char0 = source_.CharAt(ident_pos);
  while (IsIdentChar(c0_) && (allow_dollar || (c0_ != '$'))) {
    ReadChar();
    ident_length++;
  }

  // Check whether the characters we read are a known keyword.
  // Note, can't use strcmp since token_chars is not null-terminated.
  int i = 0;
  while (i < Token::numKeywords &&
         keywords_[i].keyword_chars[0] <= ident_char0) {
    if (keywords_[i].keyword_len == ident_length) {
      const char* keyword = keywords_[i].keyword_chars;
      int char_pos = 0;
      while ((char_pos < ident_length) &&
             (keyword[char_pos] == source_.CharAt(ident_pos + char_pos))) {
        char_pos++;
      }
      if (char_pos == ident_length) {
        if (keywords_[i].keyword_symbol == NULL) {
          keywords_[i].keyword_symbol = &String::ZoneHandle(
              String::NewSymbol(source_, ident_pos, ident_length));
        }
        current_token_.literal = keywords_[i].keyword_symbol;
        current_token_.kind = keywords_[i].kind;
        return;
      }
    }
    i++;
  }

  // We did not read a keyword.
  current_token_.kind = Token::kIDENT;
  String& literal =
      String::ZoneHandle(String::NewSymbol(source_, ident_pos, ident_length));
  if ((ident_char0 == kPrivateIdentifierStart) && !FLAG_expose_core_impl) {
    // Private identifiers are mangled on a per script basis.
    literal = String::Concat(literal, private_key_);
    literal = String::NewSymbol(literal);
  }
  current_token_.literal = &literal;
}


// Parse integer or double number literal of format:
// NUMBER = INTEGER | DOUBLE
// INTEGER = D+ | (("0x" | "0X") H+)
// DOUBLE = ((D+ ["." D*]) | ("." D+)) [ EXPONENT ]
// EXPONENT = ("e" | "E") ["+" | "-"] D+
void Scanner::ScanNumber(bool dec_point_seen) {
  ASSERT(IsDecimalDigit(c0_));
  char first_digit = c0_;

  Recognize(dec_point_seen ? Token::kDOUBLE : Token::kINTEGER);
  if (!dec_point_seen && first_digit == '0' && (c0_ == 'x' || c0_ == 'X')) {
    ReadChar();
    if (!IsHexDigit(c0_)) {
      ErrorMsg("hexadecimal digit expected");
      return;
    }
    while (IsHexDigit(c0_)) {
      ReadChar();
    }
  } else {
    while (IsDecimalDigit(c0_)) {
      ReadChar();
    }
    if (c0_ == '.' && !dec_point_seen && IsDecimalDigit(LookaheadChar(1))) {
      Recognize(Token::kDOUBLE);
      while (IsDecimalDigit(c0_)) {
        ReadChar();
      }
    }
    if ((c0_ == 'e') || (c0_ == 'E')) {
      Recognize(Token::kDOUBLE);
      if ((c0_ == '-') || (c0_ == '+')) {
        ReadChar();
      }
      if (!IsDecimalDigit(c0_)) {
        ErrorMsg("missing exponent digits");
        return;
      }
      while (IsDecimalDigit(c0_)) {
        ReadChar();
      }
    } else if (IsIdentStartChar(c0_)) {
      ErrorMsg("illegal character in number");
      return;
    }
  }
  if (current_token_.kind != Token::kILLEGAL) {
    intptr_t len = lookahead_pos_ - token_start_;
    current_token_.literal =
        &String::ZoneHandle(
            String::SubString(source_, token_start_, len, Heap::kOld));
  }
}


RawString* Scanner::ConsumeIdentChars(bool allow_dollar) {
  ASSERT(IsIdentStartChar(c0_));
  ASSERT(allow_dollar || (c0_ != '$'));
  int ident_length = 0;
  int32_t ident_pos = lookahead_pos_;
  while (IsIdentChar(c0_) && (allow_dollar || (c0_ != '$'))) {
    ReadChar();
    ident_length++;
  }
  return String::NewSymbol(source_, ident_pos, ident_length);
}


void Scanner::ScanLibraryTag() {
  ReadChar();
  if (c0_ == '!') {
    // The script tag extends to the end of the line. Just treat this
    // similar to a line comment.
    while (c0_ != '\n' && c0_ != '\0') {
      ReadChar();
    }
    Recognize(Token::kSCRIPTTAG);
    return;
  }
  const String& kLibrary = String::Handle(String::NewSymbol("library"));
  const String& kImport = String::Handle(String::NewSymbol("import"));
  const String& kSource = String::Handle(String::NewSymbol("source"));
  const String& ident = String::Handle(ConsumeIdentChars(false));
  if (ident.Equals(kLibrary)) {
    current_token_.kind = Token::kLIBRARY;
    return;
  }
  if (ident.Equals(kImport)) {
    current_token_.kind = Token::kIMPORT;
    return;
  }
  if (ident.Equals(kSource)) {
    current_token_.kind = Token::kSOURCE;
    return;
  }
  ErrorMsg("Unrecognized library token");
}


void Scanner::ScanLiteralString(bool is_raw) {
  ASSERT(!IsScanningString());
  ASSERT(c0_ == '"' || c0_ == '\'');

  // Entering string scanning mode.
  BeginStringLiteral(c0_);
  string_is_multiline_ = (LookaheadChar(1) == c0_) &&
      (LookaheadChar(2) == c0_);

  ReadChar();  // Skip opening delimiter.
  if (string_is_multiline_) {
    ReadChar();  // Skip two additional string delimiters.
    ReadChar();
    if (c0_ == '\n') {
      // Skip first character of multiline string if it is a newline.
      ReadChar();
    }
  }
  ScanLiteralStringChars(is_raw);
}


bool Scanner::ScanHexDigits(int digits, uint32_t* value) {
  *value = 0;
  for (int i = 0; i < digits; ++i) {
    ReadChar();
    if (!IsHexDigit(c0_)) {
      ErrorMsg("too few hexadecimal digits");
      return false;
    }
    *value <<= 4;
    *value |= Utils::HexDigitToInt(c0_);
  }
  return true;
}


bool Scanner::ScanHexDigits(int min_digits, int max_digits, uint32_t* value) {
  *value = 0;
  ReadChar();
  for (int i = 0; i < max_digits; ++i) {
    if (!IsHexDigit(c0_)) {
      if (i < min_digits) {
        ErrorMsg("hexadecimal digit expected");
        return false;
      }
      break;
    }
    *value <<= 4;
    *value |= Utils::HexDigitToInt(c0_);
    ReadChar();
  }
  return true;
}


bool Scanner::ScanEscapedCodePoint(uint32_t* code_point) {
  ASSERT(c0_ == 'u' || c0_ == 'x');
  bool is_valid;
  if (c0_ == 'x') {
    is_valid = ScanHexDigits(2, code_point);
  } else if (c0_ == 'u' && LookaheadChar(1) != '{') {
    is_valid = ScanHexDigits(4, code_point);
  } else {
    ReadChar();  // Skip left curly bracket.
    is_valid = ScanHexDigits(1, 6, code_point);
    if (is_valid) {
      if (c0_ != '}') {
        ErrorMsg("expected '}' after character code");
        return false;
      }
      if (*code_point > 0x10FFFF) {
        ErrorMsg("invalid code point");
        return false;
      }
    }
  }
  return is_valid;
}


void Scanner::ScanLiteralStringChars(bool is_raw) {
  GrowableArray<uint32_t> string_chars(64);

  ASSERT(IsScanningString());
  // We are at the first character of a string literal piece. A string literal
  // can be broken up into multiple pieces by string interpolation.
  while (true) {
    if ((c0_ == '\0') || ((c0_ == '\n') && !string_is_multiline_)) {
      ErrorMsg("unterminated string literal");
      EndStringLiteral();
      return;
    }
    if (c0_ == '\\' && !is_raw) {
      // Parse escape sequence.
      uint32_t escape_char = '\0';
      ReadChar();
      switch (c0_) {
        case 'n':
          escape_char = '\n';
          break;
        case 'r':
          escape_char = '\r';
          break;
        case 'f':
          escape_char = '\f';
          break;
        case 't':
          escape_char = '\t';
          break;
        case 'b':
          escape_char = '\b';
          break;
        case 'v':
          escape_char = '\v';
          break;
        case 'u':
        case 'x':
          if (!ScanEscapedCodePoint(&escape_char)) {
            EndStringLiteral();
            return;
          }
          break;
        case '\0':
          ErrorMsg("illegal escape sequence in string");
          EndStringLiteral();
          return;
        default:
          escape_char = c0_;
          break;
      }
      string_chars.Add(escape_char);
    } else if (c0_ == '$' && !is_raw) {
      // Scanned a string piece.
      ASSERT(string_chars.data() != NULL);
      // Strings are canonicalized: Allocate a symbol.
      current_token_.literal = &String::ZoneHandle(
          String::NewSymbol(string_chars.data(), string_chars.length()));
      current_token_.kind = Token::kSTRING;
      return;
    } else if (c0_ == string_delimiter_) {
      // Check if we are at the end of the string literal.
      if (!string_is_multiline_ ||
          ((LookaheadChar(1) == string_delimiter_) &&
           (LookaheadChar(2) == string_delimiter_))) {
        if (string_is_multiline_) {
          ReadChar();  // Skip two string delimiters.
          ReadChar();
        }
        Recognize(Token::kSTRING);
        ASSERT(string_chars.data() != NULL);
        // Strings are canonicalized: Allocate a symbol.
        current_token_.literal = &String::ZoneHandle(
            String::NewSymbol(string_chars.data(), string_chars.length()));
        EndStringLiteral();
        return;
      } else {
        string_chars.Add(string_delimiter_);
      }
    } else {
      string_chars.Add(c0_);
    }
    ReadChar();
  }
}


void Scanner::Scan() {
  newline_seen_ = false;

  do {
    if (!IsScanningString()) {
      ConsumeWhiteSpace();
    }
    token_start_ = lookahead_pos_;
    current_token_.offset = lookahead_pos_;
    current_token_.position = c0_pos_;
    current_token_.literal = NULL;
    current_token_.kind = Token::kILLEGAL;
    if (IsScanningString()) {
      if (c0_ == '$') {
        ReadChar();  // Skip the '$' character.
        if (IsIdentStartChar(c0_) && (c0_ != '$')) {
          ScanIdentNoDollar();
          current_token_.kind = Token::kINTERPOL_VAR;
        } else if (c0_ == '{') {
          Recognize(Token::kINTERPOL_START);
          PushContext();
        } else {
          ErrorMsg("illegal character after $ in string interpolation");
          EndStringLiteral();
          break;
        }
      } else {
        ScanLiteralStringChars(false);
      }
      break;
    }
    switch (c0_) {
      case '\0':
        current_token_.kind = Token::kEOS;
        break;

      case '+':  // +  ++  +=
        Recognize(Token::kADD);
        if (c0_ == '+') {
          Recognize(Token::kINCR);
        } else if (c0_ == '=') {
          Recognize(Token::kASSIGN_ADD);
        }
        break;

      case '-':  // -  --  -=
        Recognize(Token::kSUB);
        if (c0_ == '-') {
          Recognize(Token::kDECR);
        } else if (c0_ == '=') {
          Recognize(Token::kASSIGN_SUB);
        }
        break;

      case '*':  // *  *=
        Recognize(Token::kMUL);
        if (c0_ == '=') {
          Recognize(Token::kASSIGN_MUL);
        }
        break;

      case '%':  // %  %=
        Recognize(Token::kMOD);
        if (c0_ == '=') {
          Recognize(Token::kASSIGN_MOD);
        }
        break;

      case '/':  //  /  /=  //  /*
        Recognize(Token::kDIV);
        if (c0_ == '/') {
          ConsumeLineComment();
        } else if (c0_ == '*') {
          ConsumeBlockComment();
        } else if (c0_ == '=') {
          Recognize(Token::kASSIGN_DIV);
        }
        break;

      case '&':  // &  &=  &&
        Recognize(Token::kBIT_AND);
        if (c0_ == '=') {
          Recognize(Token::kASSIGN_AND);
        } else if (c0_ == '&') {
          Recognize(Token::kAND);
        }
        break;

      case '|':  // |  |=  ||
        Recognize(Token::kBIT_OR);
        if (c0_ == '=') {
          Recognize(Token::kASSIGN_OR);
        } else if (c0_ == '|') {
          Recognize(Token::kOR);
        }
        break;

      case '^':  // ^  ^=
        Recognize(Token::kBIT_XOR);
        if (c0_ == '=') {
          Recognize(Token::kASSIGN_XOR);
        }
        break;

      case '[':  // [  []  []=
        Recognize(Token::kLBRACK);
        if (c0_ == ']') {
          Recognize(Token::kINDEX);
          if (c0_ == '=') {
            Recognize(Token::kASSIGN_INDEX);
          }
        }
        break;

      case ']':  //  ]
        Recognize(Token::kRBRACK);
        break;

      case '<':  // <  <=  <<  <<=
        Recognize(Token::kLT);
        if (c0_ == '=') {
          Recognize(Token::kLTE);
        } else if (c0_ == '<') {
          Recognize(Token::kSHL);
          if (c0_ == '=') {
            Recognize(Token::kASSIGN_SHL);
          }
        }
        break;

      case '>':  // >  >=  >>  >>=  >>>  >>>=
        Recognize(Token::kGT);
        if (c0_ == '=') {
          Recognize(Token::kGTE);
        } else if (c0_ == '>') {
          Recognize(Token::kSAR);
          if (c0_ == '=') {
            Recognize(Token::kASSIGN_SAR);
          } else if (c0_ == '>') {
            Recognize(Token::kSHR);
            if (c0_ == '=') {
              Recognize(Token::kASSIGN_SHR);
            }
          }
        }
        break;

      case '!':  // !  !=  !==
        Recognize(Token::kNOT);
        if (c0_ == '=') {
          Recognize(Token::kNE);
          if (c0_ == '=') {
            Recognize(Token::kNE_STRICT);
          }
        }
        break;

      case '~':
        Recognize(Token::kBIT_NOT);
        if (c0_ == '/') {
          Recognize(Token::kTRUNCDIV);
          if (c0_ == '=') {
            Recognize(Token::kASSIGN_TRUNCDIV);
          }
        }
        break;

      case '=':  // =  ==  ===  =>
        Recognize(Token::kASSIGN);
        if (c0_ == '=') {
          Recognize(Token::kEQ);
          if (c0_ == '=') {
            Recognize(Token::kEQ_STRICT);
          }
        } else if (c0_ == '>') {
          Recognize(Token::kARROW);
        }
        break;

      case '.':  // .  Number
        Recognize(Token::kPERIOD);
        if (IsDecimalDigit(c0_)) {
          ScanNumber(true);
        }
        break;

      case '?':
        Recognize(Token::kCONDITIONAL);
        break;

      case ':':
        Recognize(Token::kCOLON);
        break;

      case ';':
        Recognize(Token::kSEMICOLON);
        break;

      case '{':
        Recognize(Token::kLBRACE);
        if (IsNestedContext()) {
          brace_level_++;
        }
        break;

      case '}':
        Recognize(Token::kRBRACE);
        if (IsNestedContext()) {
          ASSERT(brace_level_ > 0);
          brace_level_--;
          if (brace_level_ == 0) {
            current_token_.kind = Token::kINTERPOL_END;
            PopContext();
          }
        }
        break;

      case '(':
        Recognize(Token::kLPAREN);
        break;

      case ')':
        Recognize(Token::kRPAREN);
        break;

      case ',':
        Recognize(Token::kCOMMA);
        break;

      case '@':
        Recognize(Token::kILLEGAL);
        if ((c0_ == '"') || (c0_ == '\'')) {
          ScanLiteralString(true);
        }
        break;
      case '"':
      case '\'':
        ScanLiteralString(false);
        break;

      case '#':
        ScanLibraryTag();
        break;

      default:
        if (IsIdentStartChar(c0_)) {
          ScanIdent();
        } else if (IsDecimalDigit(c0_)) {
          ScanNumber(false);
        } else {
          char msg[128];
          OS::SNPrint(msg, sizeof(msg),
                      "unexpected character: %c  (%02x)\n", c0_, c0_);
          ErrorMsg(msg);
          ReadChar();
        }
    }
  } while (current_token_.kind == Token::kWHITESP);
  current_token_.length = lookahead_pos_ - token_start_;
}


void Scanner::ScanAll(GrowableTokenStream* token_stream) {
  do {
    Scan();
    token_stream->Add(current_token_);
  } while (current_token_.kind != Token::kEOS);
}

void Scanner::ScanTo(intptr_t token_index) {
  int index = 0;
  do {
    Scan();
    index++;
  } while ((token_index >= index) && (current_token_.kind != Token::kEOS));
}


const Scanner::GrowableTokenStream& Scanner::GetStream() {
  GrowableTokenStream* ts = new GrowableTokenStream(128);
  ScanAll(ts);
  if (FLAG_print_tokens) {
    Scanner::PrintTokens(*ts);
  }
  return *ts;
}


void Scanner::PrintTokens(const GrowableTokenStream& ts) {
  int currentLine = -1;
  for (int i = 0; i < ts.length(); i++) {
    const TokenDescriptor& td = ts[i];
    if (currentLine != td.position.line) {
      currentLine = td.position.line;
      OS::Print("\n%d (%d): ", currentLine, i);
    }
    OS::Print("%s ", Token::Name(td.kind));
  }
  OS::Print("\n");
}


RawString* Scanner::AllocatePrivateKey(const Library& library) {
  const String& url = String::Handle(library.url());
  intptr_t key_value = url.Hash();
  while (Library::IsKeyUsed(key_value)) {
    key_value++;
  }
  char private_key[32];
  OS::SNPrint(private_key, sizeof(private_key),
              "%c%"PRIxPTR, kPrivateKeySeparator, key_value);
  const String& result = String::Handle(String::New(private_key));
  return result.raw();
}


void Scanner::InitOnce() {
}

}  // namespace dart