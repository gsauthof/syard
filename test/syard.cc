// 2016, Georg Sauthoff <mail@georg.so>

#include <catch/catch.hpp>

#include <syard/syard.hh>
#include <string>
#include <math.h>
#include <assert.h>
#include <iostream>

using namespace std;
using namespace syard;

TEST_CASE("syard_" "lex function", "[syard][lex]" )
{
  Operator_Table t;
  const char inp[] = "sin";
  auto r = t.lex(inp, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp);
  CHECK(r.p.second == inp+sizeof(inp)-1);
  CHECK(r.id == FUNCTION);
  CHECK(r.op == nullptr);

  r = t.lex(r.p.second, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp+sizeof(inp)-1);
  CHECK(r.p.second == inp+sizeof(inp)-1);
  CHECK(r.id == EPSILON);
  CHECK(r.op == nullptr);
}

TEST_CASE("syard_" "function with space", "[syard][lex]" )
{
  Operator_Table t;
  const char inp[] = "  sin  ";
  auto r = t.lex(inp, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp+2);
  CHECK(r.p.second == inp+2+3);
  CHECK(r.id == FUNCTION);
  CHECK(r.op == nullptr);

  r = t.lex(r.p.second, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp+sizeof(inp)-1);
  CHECK(r.p.second == inp+sizeof(inp)-1);
  CHECK(r.id == EPSILON);
  CHECK(r.op == nullptr);
}


TEST_CASE("syard_" "numbers", "[syard][lex]" )
{
  Operator_Table t;
  const char inp[] = "23 0.22 .42";
  auto r = t.lex(inp, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp);
  CHECK(r.p.second == inp+2);
  CHECK(r.id == OPERAND);
  CHECK(r.op == nullptr);
  r = t.lex(r.p.second, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp+2+1);
  CHECK(r.p.second == inp+2+1+4);
  CHECK(r.id == OPERAND);
  CHECK(r.op == nullptr);
  r = t.lex(r.p.second, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp+2+1+4+1);
  CHECK(r.p.second == inp+2+1+4+1+3);
  CHECK(r.id == OPERAND);
  CHECK(r.op == nullptr);
  r = t.lex(r.p.second, inp+sizeof(inp)-1);
  CHECK(r.id == EPSILON);
}

TEST_CASE("syard_" "ops", "[syard][lex]" )
{
  Operator_Table t;
  t.insert('^', 10, 11, false);
  const char pp[] = "**";
  t.insert(pp, pp+sizeof(pp)-1, 10, 11, false);
  t.insert('*', 11, 10, true);
  t.insert('/', 12, 10, true);
  t.insert('+', 13,  9, true);
  t.insert('-', 14,  9, true);
  t.sort();

  const char inp[] = "+   * **";
  auto r = t.lex(inp, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp);
  CHECK(r.p.second == inp+1);
  CHECK(r.id == 13);
  CHECK(r.op != nullptr);
  CHECK(r.op->left_associative);
  CHECK(r.op->precedence == 9);
  r = t.lex(r.p.second, inp+sizeof(inp)-1);
  CHECK(r.id == 11);
  r = t.lex(r.p.second, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp+6);
  CHECK(r.p.second == inp+sizeof(inp)-1);
  CHECK(r.id == 10);
  CHECK(r.op != nullptr);
  CHECK(!r.op->left_associative);
  CHECK(r.op->precedence == 11);
}

TEST_CASE("syard_" "ops2", "[syard][lex]" )
{
  Parser p;
  Operator_Table &t = p.operator_table();
  t.insert_default_arithmetic();

  const char inp[] = "* *2";
  auto r = t.lex(inp, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp);
  CHECK(r.p.second == inp+1);
  CHECK(r.id == 11);
  CHECK(r.op != nullptr);
  CHECK(r.op->left_associative);
  CHECK(r.op->precedence == 9);

  r = t.lex(r.p.second, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp+2);
  CHECK(r.p.second == inp+3);
  CHECK(r.id == 11);
  CHECK(r.op != nullptr);
  CHECK(r.op->left_associative);
  CHECK(r.op->precedence == 9);

  r = t.lex(r.p.second, inp+sizeof(inp)-1);
  CHECK(r.p.first == inp+3);
  CHECK(r.p.second == inp+sizeof(inp)-1);
  CHECK(r.id == 1);
  CHECK(r.op == nullptr);
}

TEST_CASE("syard_" "addmult", "[syard][parse]" )
{
  Parser p;
  auto &t = p.operator_table();
  t.insert('+', 13,  9, true);
  t.insert('*', 11, 10, true);
  auto &o = p.arg_stack();
  p.parse("1+2*3", [&o](uint8_t id) {
      auto b = stol(o.top()); o.pop();
      auto a = stol(o.top()); o.pop();
      long c = 0;
      switch (id) {
      case 11: c = a * b; break;
      case 13: c = a + b; break;
      }
      o.push(to_string(c));
      });
  CHECK(o.top() == "7");
  CHECK(o.size() == 1);
}

TEST_CASE("syard_" "parentheses", "[syard][parse]" )
{
  Parser p;
  auto &t = p.operator_table();
  t.insert('*', 11, 10, true);
  t.insert('+', 13,  9, true);
  t.sort();
  auto &o = p.arg_stack();
  p.parse("(1+2)*3", [&o](uint8_t id) {
      auto b = stol(o.top()); o.pop();
      auto a = stol(o.top()); o.pop();
      long c = 0;
      switch (id) {
      case 11: c = a * b; break;
      case 13: c = a + b; break;
      }
      o.push(to_string(c));
      });
  CHECK(o.top() == "9");
  CHECK(o.size() == 1);
}


TEST_CASE("syard_" "function", "[syard][parse]" )
{
  Parser p;
  auto &t = p.operator_table();
  t.insert('*', 11, 10, true);
  t.sort();
  auto &f = p.function_table();
  f.insert("plus", 13);
  f.insert("minus", 14);
  f.insert("zoo", 15);
  auto &o = p.arg_stack();
  p.parse("plus(1, 2) * 3", [&o](uint8_t id) {
      assert(o.size() >= 2);
      auto b = stol(o.top()); o.pop();
      auto a = stol(o.top()); o.pop();
      long c = 0;
      switch (id) {
      case MULT: c = a * b; break;
      case PLUS: c = a + b; break;
      }
      o.push(to_string(c));
      });
  CHECK(o.top() == "9");
  CHECK(o.size() == 1);
}

TEST_CASE("syard_" "expr with parens", "[syard][parse]" )
{
  Parser p;
  auto &t = p.operator_table();
  t.insert_default_arithmetic();
  auto &o = p.arg_stack();
  p.parse(" (1+2)*(2+3)*4^(2*3+4)+2 ", [&o](uint8_t id) {
      assert(o.size() >= 2);
      auto b = stol(o.top()); o.pop();
      auto a = stol(o.top()); o.pop();
      long c = 0;
      switch (id) {
      case 10: c = pow(a, b); break;
      case 11: c = a * b; break;
      case 13: c = a + b; break;
      }
      o.push(to_string(c));
      });
  CHECK(o.top() == "15728642");
  CHECK(o.size() == 1);
}

TEST_CASE("syard_" "unary overload", "[syard][parse]" )
{
  Parser p;
  p.operator_table().insert_default_arithmetic();
  auto &o = p.arg_stack();
  p.parse("-2*-3*-4", [&o](uint8_t id) {
      assert(o.size() >= 2);
      auto b = stol(o.top()); o.pop();
      auto a = stol(o.top()); o.pop();
      long c = 0;
      switch (id) {
      case 10: c = pow(a, b); break;
      case 11: c = a * b; break;
      case 13: c = a + b; break;
      }
      o.push(to_string(c));
      });
  CHECK(o.top() == "-24");
  CHECK(o.size() == 1);
}

TEST_CASE("syard_" "throws", "[syard][parse]" )
{
  Parser p;
  p.operator_table().insert_default_arithmetic();
  auto &o = p.arg_stack();
  CHECK_THROWS_AS(
  p.parse("(-2*(-3*-4)", [&o](uint8_t id) {
      assert(o.size() >= 2);
      auto b = stol(o.top()); o.pop();
      auto a = stol(o.top()); o.pop();
      long c = 0;
      switch (id) {
      case 10: c = pow(a, b); break;
      case 11: c = a * b; break;
      case 13: c = a + b; break;
      }
      o.push(to_string(c));
      }), std::underflow_error);
}
TEST_CASE("syard_" "throws2", "[syard][parse]" )
{
  Parser p;
  p.operator_table().insert_default_arithmetic();
  auto &o = p.arg_stack();
  CHECK_THROWS_AS(
  p.parse("-2*-3*-4)", [&o](uint8_t id) {
      assert(o.size() >= 2);
      auto b = stol(o.top()); o.pop();
      auto a = stol(o.top()); o.pop();
      long c = 0;
      switch (id) {
      case 10: c = pow(a, b); break;
      case 11: c = a * b; break;
      case 13: c = a + b; break;
      }
      o.push(to_string(c));
      }), std::underflow_error);
}

