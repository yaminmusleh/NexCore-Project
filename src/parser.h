#pragma once
#include <vector>
#include <optional>
#include "tokenization.h"

//we need nodes that represent a single expression.

struct NodeExpr {
    Token int_lit; // a variable with Type Token. represents the integer literal in the compiler.
};

struct NodeExit {
    // represents "exit" statement in the compiler.
    NodeExpr expr;
    //here we put the expression from the node before it inside the exit statement.
    //exit statement holds the data that got moved into it from the NodeExpr. we will make it a variant of many operations in the future.
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : p_tokens(std::move(tokens)) {
    }

    // Tries to parse a single expression (currently: just an int_lit).
    // Returns nullopt if what's next isn't a valid expression.
    [[nodiscard]] optional<NodeExpr> parse_expr() {
        if (peek().has_value() && peek()->type == TypeOfToken::int_lit) {
            return NodeExpr{.int_lit = consume()};
            //it will take the token and return it back to the struct so the exit then takes that value.
        }
        return {};
    }

    // Tries to parse the whole program. Right now "the whole program"
    // is just one exit statement: exit <expr> ;
    [[nodiscard]] optional<NodeExit> parse() {
        optional<NodeExit> exit_node;

        if (peek().has_value() && peek()->type == TypeOfToken::exit) {
            consume(); // eat "exit"

            if (auto node_expr = parse_expr()) {
                exit_node = NodeExit{.expr = node_expr.value()};
                /*
                If a valid expression
                is found, it creates a `NodeExit` containing that expression.
                */
            } else {
                throw std::runtime_error("Invalid expression after exit");
            }

            if (peek().has_value() && peek()->type == TypeOfToken::semi) {
                consume(); // eat ";"
            } else {
                throw std::runtime_error("Expected ';'");
                /*
                Finally, it verifies that the statement ends with a semicolon (`;`).
                If any required part is missing, a runtime error is thrown.
                */
            }
        }

        return exit_node;

        /*Parses the entire program according to the current grammar

         At the moment, the language only supports a single statement:
             exit <expr> ;

         The parser first checks for the `exit` keyword, then parses the
         expression that follows using `parse_expr()`.

         If the program does not start with `exit`, an empty `optional` is returned.*/
    }

private:
    // Same idea as the tokenizer's peek: look at the current token
    // (or one ahead with offset) without consuming it.
    [[nodiscard]] inline optional<Token> peek(int offset = 0) const {
        if (m_index + offset < p_tokens.size()) {
            return p_tokens[m_index + offset];
        }
        return {};
    }

    // Same idea as the tokenizer's consume: return the current token
    // and move the index forward.
    [[nodiscard]] inline Token consume() {
        // it will consume the token not the character like before
        return p_tokens[m_index++];
    }

    vector<Token> p_tokens;
    size_t m_index = 0;
};


/*
 Notes:
 - the parser works like a tokenizer but for the tokens themselves while the tokenizer splits strings into small chunks
 chunks-> tokens-> parsing

An exit statement contains an expression.
Right now the expression is just an integer literal, but later it could be
an identifier (x), a binary operation (5 + 3), a function call, etc.
By storing a NodeExpr instead of a Token, we can extend the language
without changing the structure of NodeExit.
struct NodeExit {
    NodeExpr expr;
};

 */
