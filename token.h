/*
 * Copyright (c) 2003-2008, John Wiegley.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of New Artisans LLC nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _TOKEN_H
#define _TOKEN_H

#include "expr.h"

namespace ledger {

struct expr_t::token_t : public noncopyable, public supports_flags<>
{
#define TOKEN_SHORT_ACCOUNT_MASK  0x01
#define TOKEN_CODE_MASK           0x02
#define TOKEN_COMMODITY_MASK      0x04
#define TOKEN_PAYEE_MASK          0x08
#define TOKEN_NOTE_MASK           0x10
#define TOKEN_ACCOUNT_MASK        0x20

  enum kind_t {
    VALUE,			// any kind of literal value
    IDENT,			// [A-Za-z_][-A-Za-z0-9_:]*
    MASK,			// /regexp/

    LPAREN,			// (
    RPAREN,			// )

    EQUAL,			// ==
    NEQUAL,			// !=
    LESS,			// <
    LESSEQ,			// <=
    GREATER,			// >
    GREATEREQ,			// >=

    ASSIGN,			// =
    MINUS,			// -
    PLUS,			// +
    STAR,			// *
    KW_DIV,			// /

    EXCLAM,			// !
    KW_AND,			// &
    KW_OR,			// |
    KW_MOD,			// %

    QUERY,			// ?
    COLON,			// :

    COMMA,			// ,

    TOK_EOF,
    UNKNOWN

  } kind;

  char	      symbol[3];
  value_t     value;
  std::size_t length;

  explicit token_t() : supports_flags<>(), kind(UNKNOWN), length(0) {
    TRACE_CTOR(token_t, "");
  }
  ~token_t() throw() {
    TRACE_DTOR(token_t);
  }

  token_t& operator=(const token_t& other) {
    if (&other == this)
      return *this;
    assert(false);
    return *this;
  }

  void clear() {
    kind   = UNKNOWN;
    length = 0;
    value  = NULL_VALUE;

    symbol[0] = '\0';
    symbol[1] = '\0';
    symbol[2] = '\0';
  }

  void parse_ident(std::istream& in);
  void next(std::istream& in, unsigned int flags);
  void rewind(std::istream& in);
  void unexpected();

  static void expected(char wanted, char c = '\0');
};

} // namespace ledger

#endif // _TOKEN_H