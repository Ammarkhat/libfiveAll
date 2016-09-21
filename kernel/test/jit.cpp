/*
 *  Copyright (C) 2016 Matthew Keeter  <matt.j.keeter@gmail.com>
 *
 *  This file is part of the Ao library.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  Ao is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Ao.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <catch/catch.hpp>

#include "ao/kernel/tree/tree.hpp"
#include "ao/kernel/tree/token.hpp"

TEST_CASE("Constructing a simple shape")
{
    Token* out = Token::operation(Opcode::OP_ADD, Token::X(),
            Token::constant(1));
    REQUIRE(out != nullptr);
}

TEST_CASE("Deduplication of variables")
{
    Tree t;

    Token::Id xa = t.X();
    Token::Id xb = t.X();
    REQUIRE(xa == xb);

    Token::Id ya = t.Y();
    REQUIRE(xa != ya);
}

TEST_CASE("Deduplication of constants")
{
    Tree t;

    Token::Id ca = t.constant(3.14);
    Token::Id cb = t.constant(3.14);
    REQUIRE(ca == cb);

    Token::Id cc = t.constant(4);
    REQUIRE(ca != cc);
}

TEST_CASE("Deduplication of operations")
{
    Store s;

    Token* oa = s.operation(OP_ADD, s.X(), s.constant(1));
    Token* ob = s.operation(OP_ADD, s.X(), s.constant(1));
    REQUIRE(oa == ob);

    Token* oc = s.operation(OP_ADD, s.X(), s.constant(2));
    REQUIRE(oa != oc);
}

TEST_CASE("Found flag propagation")
{
    Store s;

    Token* oa = s.operation(OP_ADD, s.X(), s.constant(1));
    Token* ob = s.operation(OP_MUL, s.Y(), s.constant(1));

    auto f = s.findConnected(oa);

    REQUIRE(f.count(oa));
    REQUIRE(f.count(s.X()));
    REQUIRE(f.count(s.constant(1)));

    REQUIRE(!f.count(ob));
    REQUIRE(!f.count(s.Y()));
}

TEST_CASE("Automatic expression pruning")
{
    Store s;

    SECTION("Addition")
    {
        Token* oa = s.operation(OP_ADD, s.X(), s.constant(0));
        REQUIRE(oa == s.X());

        Token* ob = s.operation(OP_ADD, s.constant(0), s.X());
        REQUIRE(ob == s.X());
    }

    SECTION("Subtraction")
    {
        Token* oa = s.operation(OP_SUB, s.X(), s.constant(0));
        REQUIRE(oa == s.X());

        Token* ob = s.operation(OP_SUB, s.constant(0), s.X());
        REQUIRE(ob->op == OP_NEG);
        REQUIRE(ob->a == s.X());
    }

    SECTION("Multiplication")
    {
        Token* oa = s.operation(OP_MUL, s.X(), s.constant(1));
        REQUIRE(oa == s.X());

        Token* ob = s.operation(OP_MUL, s.constant(1), s.X());
        REQUIRE(ob == s.X());

        Token* oc = s.operation(OP_MUL, s.X(), s.constant(0));
        REQUIRE(oc == s.constant(0));

        Token* od = s.operation(OP_MUL, s.constant(0), s.X());
        REQUIRE(od == s.constant(0));
    }
}
