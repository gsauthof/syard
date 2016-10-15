// Copyright 2016, Georg Sauthoff <mail@georg.so>

/* {{{ LGPLv3

    This file is part of syard.

    syard is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    syard is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with syard.  If not, see <http://www.gnu.org/licenses/>.

}}} */
#include "syard.hh"

#include <algorithm>
#include <array>
#include <functional>
//#include <iostream>
#include <stdexcept>
#include <string.h>
#include <utility>

using namespace std;

namespace syard {

  Operator::Operator() =default;

  Operator::Operator(uint8_t id)
    :
      left_associative(true),
      function(true),
      sign_overload(false),
      precedence(0),
      id(id)
  {
  }

  Operator_Table::Result::Result(const char *begin, const char *end,
      uint8_t id, Operator *op)
    : p(begin, end), id(id), op(op)
  {
  }
  Operator_Table::Result::Result(
      const unsigned char *begin, const unsigned char *end,
      uint8_t id, Operator *op)
    : Result(reinterpret_cast<const char*>(begin),
        reinterpret_cast<const char *>(end), id, op)
  {
  }

  Operator_Table::Operator_Table()
  {
    table_.reserve(8);
  }

  void Operator_Table::insert(const char *begin, const char *end,
      uint8_t id, uint8_t precedence, bool left_associative,
      bool sign_overload)
  {
    if (begin >= end)
      return;
    if (end-begin > MAX_OPERATOR_SIZE)
      throw overflow_error("only supports operators up to "
          + to_string(MAX_OPERATOR_SIZE) + " chars");

    table_.emplace_back();
    sorted_ = false;
    auto &t = table_.back();
    copy(begin, end, t.first.begin());
    fill(t.first.begin() + (end-begin), t.first.end(), 0+0);
    t.second.left_associative = left_associative;
    t.second.sign_overload = sign_overload;
    t.second.function = false;
    t.second.precedence = precedence;
    t.second.id = id;
  }
  void Operator_Table::sort()
  {
    if (sorted_)
      return;
    std::sort(table_.begin(), table_.end(), [](auto &a, auto &b) {
        return a.first > b.first; });
    sorted_ = true;
  }
  void Operator_Table::insert(char c, uint8_t id,
      uint8_t precedence, bool left_associative, bool sign_overload)
  {
    insert(&c, &c+1, id, precedence, left_associative, sign_overload);
  }
  void Operator_Table::insert(const char *s, uint8_t id,
      uint8_t precedence, bool left_associative, bool sign_overload)
  {
    insert(s, s+strlen(s), id, precedence, left_associative, sign_overload);
  }

  struct Op_Compare {
    bool operator()(const pair<const unsigned char*, const unsigned char*> &a,
        const std::pair<std::array<unsigned char, MAX_OPERATOR_SIZE>,
                        Operator> &b) const
    {
      bool r = lexicographical_compare(
          a.first, min(a.first + b.first.size(), a.second),
          b.first.begin(), b.first.end(), std::greater<unsigned char>());
      return r;
    }
    bool operator()(
        const std::pair<std::array<unsigned char, MAX_OPERATOR_SIZE>,
                        Operator> &a,
        const pair<const unsigned char*, const unsigned char*> &b) const
    {
      bool r = lexicographical_compare(
          a.first.begin(), a.first.end(),
          b.first, min(b.first + a.first.size(), b.second),
          std::greater<char>());
      return r;
    }
  };

  template <typename I1, typename I2>
    I1 common_prefix(I1 b1, I1 e1, I2 b2, I2 e2)
    {
      for ( ; b1 != e1 && b2 != e2; ++b1, ++b2)
        if (*b1 != *b2)
          break;
      return b1;
    }

  // we operate (i.e. compare) unsigned bytes to be able to deal with utf8
  // strings
  Operator_Table::Result Operator_Table::lex(
      const char *beginx, const char *endx)
  {
    sort();
    auto begin = reinterpret_cast<const unsigned char*>(beginx);
    auto end   = reinterpret_cast<const unsigned char*>(endx);
    auto p = begin;
    p = find_if_not(p, end, [](unsigned char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r'; });
    if (p == end)
      return Result(end, end, EPSILON, nullptr);
    auto r = upper_bound(table_.begin(), table_.end(), make_pair(p, end),
        Op_Compare());
    if (r != table_.end()) {
      for ( ; r != table_.end(); ++r) {
        Operator *op = &(*r).second;
        auto e = find((*r).first.begin(), (*r).first.end(), 0+0);
        auto f = common_prefix((*r).first.begin(), e, p, end);
        if (f == (*r).first.begin())
          break;
        if (e == f) {
          auto n = e - (*r).first.begin();
          return Result(p, p+n, op->id, op);
        }
      }
    }
    auto b = p;
    if ((*p >= '0' && *p <= '9') || *p == '.') {
      p = find_if_not(p, end, [](char c) {
          return (c >= '0' && c <= '9') || c == '.'; });
      return Result(b, p, OPERAND, nullptr);
    } else if ((*p >= 'a' && *p <= 'z') || *p == '_') {
      p = find_if_not(p, end, [](char c) {
          return (c >= 'a' && c <= 'z') || c == '_'; });
      return Result(b, p, FUNCTION, nullptr);
    }
    throw range_error("no operator/operand to lex");
  }
  void Operator_Table::insert_default_arithmetic()
  {
    insert('^'  , 10 , 10 , false );
    insert("**" , 10 , 10 , false );
    insert("*"  , 11 , 9  , true  );
    insert("/"  , 12 , 9  , true  );
    insert("+"  , 13 , 8  , true  );
    insert("-"  , 14 , 8  , true  ,  true );
    sort();
  }

  bool Function_Compare::operator()(
      const std::array<char, MAX_FUNCTION_SIZE>   &a,
      const std::pair<const char *, const char *> &b) const
  {
    auto n = b.second - b.first;
    return lexicographical_compare(a.begin(), min(a.begin() + n, a.end()),
        b.first, b.second);
  }
  bool Function_Compare::operator()(
      const std::pair<const char *, const char *> &a,
      const std::array<char, MAX_FUNCTION_SIZE>   &b) const
  {
    auto n = a.second - a.first;
    return lexicographical_compare(a.first, a.second,
        b.begin(), min(b.begin() + n, b.end()));
  }
  bool Function_Compare::operator()(
      const std::array<char, MAX_FUNCTION_SIZE> &a,
      const std::array<char, MAX_FUNCTION_SIZE> &b) const
  {
    return a < b;
  }

  Function_Table::Function_Table() =default;

  void Function_Table::insert(const char *begin, const char *end, uint8_t id)
  {
    if (end-begin >= MAX_FUNCTION_SIZE)
      throw length_error("only function names up to "
          + to_string(MAX_FUNCTION_SIZE) + " chars");
    array<char, MAX_FUNCTION_SIZE> a;
    copy(begin, end, a.begin());
    fill(a.begin() + (end-begin), a.end(), 0);
    table_.emplace(a, make_unique<Operator>(id));
  }
  void Function_Table::insert(const char *s, uint8_t id)
  {
    insert(s, s+strlen(s), id);
  }
  const Operator *Function_Table::at(
      const pair<const char*, const char*> &p) const
  {
    auto i = table_.find(p);
    if (i == table_.end())
      throw range_error("unknown function: " + string(p.first, p.second));
    return (*i).second.get();
  }

  Parser::Parser()
  {
    vector<const Operator*> v;
    v.reserve(8);
    op_stack_ = Stack<const Operator*>(std::move(v));
    vector<string> w;
    w.reserve(8);
    a_stack_ = Stack<string>(std::move(w));

    op_table_.insert('(', LEFT_PAREN , 0, true);
    op_table_.insert(')', RIGHT_PAREN, 0, true);
    op_table_.insert(',', COMMA      , 0, true);
    op_table_.sort();
  }
  Operator_Table &Parser::operator_table()
  {
    return op_table_;
  }
  Stack<std::string> &Parser::arg_stack()
  {
    return a_stack_;
  }
  Function_Table &Parser::function_table()
  {
    return function_table_;
  }
  void Parser::parse(const char *s, std::function<void(uint8_t id)> f)
  {
    parse(s, s+strlen(s), f);
  }
  void Parser::parse(const char *begin, const char *end,
      std::function<void(uint8_t id)> f)
  {
    auto b = begin;
    uint8_t last_id = EPSILON;
    string sign;
    for (;;) {
      auto r = op_table_.lex(b, end);
      switch (r.id) {
        case EPSILON:
          while (!op_stack_.empty()) {
            if (op_stack_.top()->id < FIRST_ID)
              throw underflow_error("unmatched element");
            f(op_stack_.top()->id);
            op_stack_.pop();
          }
          return;
        case FUNCTION:
          op_stack_.push(function_table_.at(r.p));
          break;
        case OPERAND:
          if (sign.empty())
            a_stack_.push(string(r.p.first, r.p.second));
          else {
            a_stack_.push(sign + string(r.p.first, r.p.second));
            sign.clear();
          }
          break;
        case LEFT_PAREN:
          op_stack_.push(r.op);
          break;
        case RIGHT_PAREN:
          while (!op_stack_.empty() && op_stack_.top()->id != LEFT_PAREN) {
            f(op_stack_.top()->id);
            op_stack_.pop();
          }
          if (op_stack_.empty() || op_stack_.top()->id != LEFT_PAREN)
            throw underflow_error("unmatched paren");
          op_stack_.pop();
          if (!op_stack_.empty() && op_stack_.top()->function) {
            if (op_stack_.top()->id < FIRST_ID)
              throw underflow_error("unexpected operator");
            f(op_stack_.top()->id);
            op_stack_.pop();
          }
          break;
        case COMMA:
          while (!op_stack_.empty() && op_stack_.top()->id != LEFT_PAREN) {
            f(op_stack_.top()->id);
            op_stack_.pop();
          }
          if (op_stack_.empty() || op_stack_.top()->id != LEFT_PAREN)
            throw underflow_error("unmatched paren");
          break;
        default:
          if ( (last_id == OPERATOR || last_id == LEFT_PAREN
                || last_id == COMMA || last_id == EPSILON)
              && r.op->sign_overload) {
            sign.insert(sign.end(), r.p.first, r.p.second);
          } else {
            while (!op_stack_.empty()
                && op_stack_.top()->id >= FIRST_ID
                && (  (r.op->left_associative
                    && r.op->precedence <= op_stack_.top()->precedence)
                  || (!r.op->left_associative
                    && r.op->precedence <  op_stack_.top()->precedence))) {
              f(op_stack_.top()->id);
              op_stack_.pop();
            }
            op_stack_.push(r.op);
          }
      }
      b = r.p.second;
      last_id = r.id < FIRST_ID ? r.id : OPERATOR;
    }
  }

} // syard

