#ifndef SYARD_SYARD_HH
#define SYARD_SYARD_HH

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

#include <map>
#include <memory>
#include <stack>
#include <stddef.h>
#include <stdint.h>
#include <unordered_map>
#include <vector>

namespace syard {

  struct Operator {
    bool left_associative;
    bool function;
    bool sign_overload;
    uint8_t precedence;
    uint8_t id;
    Operator();
    Operator(uint8_t id);
  };

  enum Token : uint8_t { 
    EPSILON, OPERAND, FUNCTION, LEFT_PAREN, RIGHT_PAREN, COMMA, OPERATOR,
    FIRST_ID = 10
  };

  // default operator ids
  enum Ops : uint8_t {
    POWER = 10,
    MULT  = 11,
    DIV   = 12,
    PLUS  = 13,
    MINUS = 14
  };

  enum { MAX_OPERATOR_SIZE = 4 };
  class Operator_Table {
    private:
      std::vector<std::pair<std::array<unsigned char, MAX_OPERATOR_SIZE>,
                            Operator> > table_;
      bool sorted_ {false};
    public:
      struct Result {
        std::pair<const char *, const char *> p;
        uint8_t id;
        const Operator *op;
        Result(const char *begin, const char *end, uint8_t id, Operator *op);
        Result(const unsigned char *begin, const unsigned char *end,
            uint8_t id, Operator *op);
      };
      Operator_Table();
      Result lex(const char *begin, const char *end);
      void insert(const char *begin, const char *end, uint8_t id,
          uint8_t precedence, bool left_associative,
          bool sign_overload=false);
      void insert(const char *s, uint8_t id, uint8_t precedence,
          bool left_associative, bool sign_overload=false);
      void insert(const char op, uint8_t id, uint8_t precedence,
          bool left_associative, bool sign_overload=false);
      void insert_default_arithmetic();
      void sort();
  };

  enum { MAX_FUNCTION_SIZE = 8 };
  struct Function_Compare {
    using is_transparent = std::true_type;
    bool operator()(const std::array<char, MAX_FUNCTION_SIZE> &a,
        const std::pair<const char *, const char *> &b) const;
    bool operator()(const std::pair<const char *, const char *> &a,
        const std::array<char, MAX_FUNCTION_SIZE> &b) const;
    bool operator()(const std::array<char, MAX_FUNCTION_SIZE> &a,
        const std::array<char, MAX_FUNCTION_SIZE> &b) const;
  };
  class Function_Table {
    private:
      // for unordered_map we need some generic hashing support,
      // e.g. something like boost::hash_combine()
      // using std::string() seems a bit wasteful, because it has 32 byte
      // on 64 bit Linux for short strings up to 15 byte, over that
      // 32 byte plus a heap allocation
      // function names are expected to be relatively short
      std::map<std::array<char, MAX_FUNCTION_SIZE>, std::unique_ptr<Operator>,
        Function_Compare> table_;
    public:
      Function_Table();
      void insert(const char *s, uint8_t id);
      void insert(const char *begin, const char *end, uint8_t id);
      const Operator *at(const std::pair<const char *, const char *> &s) const;
  };

  template <typename T> using Stack = std::stack<T, std::vector<T> >;

  class Parser {
    private:
      Operator_Table op_table_;
      Function_Table function_table_;

      Stack<const Operator*> op_stack_;
      Stack<std::string> a_stack_;

      const char *begin_;
      const char *end_;
    public:
      Parser();
      void parse(const char *begin, const char *end,
          std::function<void(uint8_t id)> f);
      void parse(const char *s, std::function<void(uint8_t id)> f);

      Operator_Table &operator_table();
      Stack<std::string> &arg_stack();
      Function_Table &function_table();
  };

} //syard

#endif // SYARD_SYARD_HH
