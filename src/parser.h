#pragma once

#include <vector>
#include <variant>
#include <optional>
#include <memory>
#include <stdexcept>

#include "tokenization.h"

//we need nodes that represent a single expression.

struct NodeExprBinary;

struct NodeExprIntLit {
    Token int_lit; // this alone means that the expression should only accept integer literals like 1, 2, 3 etc.
};

struct NodeExprIdentifier {
    Token identifier; // represents the variables like x, y, z etc.
};

using NodeExprVariant = std::variant<
    NodeExprIntLit,
    NodeExprIdentifier,
    std::unique_ptr<NodeExprBinary>
>;

struct NodeExpr {
    NodeExprVariant expr;
};

struct NodeExprBinary {
    std::unique_ptr<NodeExpr> lhs; // on the left hand side
    Token op; // operations
    std::unique_ptr<NodeExpr> rhs; // on the right hand side

    //this represents mathematical operations like 4+3 or x * y etc.
};

struct NodeExit {
    // represents "exit" statement in the compiler.
    NodeExpr expr;
    //here we put the expression from the node before it inside the exit statement.
};

/*
NodeExpr represents the general idea of an "expression".

The std::variant inside NodeExpr can hold exactly ONE expression node type
at a time. It does not decide which type to store—the parser does.

"When parse_expr() reads the next token(s), it determines what kind of
expression it has found and constructs the appropriate node."

For example:

    42      -> NodeExpr contains NodeExprIntLit
    x       -> NodeExpr contains NodeExprIdentifier
    5 + 3   -> NodeExpr contains NodeExprBinary

No matter what kind of expression is parsed, parse_expr() always returns
a NodeExpr. The variant remembers the concrete expression type, allowing
the rest of the compiler to treat everything uniformly as an expression.
*/

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : p_tokens(std::move(tokens)) {
    }

    // Tries to parse a single expression.
    // Currently supports:
    //  - integer literals
    //  - identifiers
    [[nodiscard]] std::optional<NodeExpr> parse_expr() {
        if (peek() && peek()->type == TypeOfToken::int_lit) {
            return NodeExpr{
                NodeExprIntLit{consume()}
            };
        }

        if (peek() && peek()->type == TypeOfToken::identifier) {
            return NodeExpr{
                NodeExprIdentifier{consume()}
            };
        }

        return std::nullopt;
    }

    // Tries to parse the whole program. Right now "the whole program"
    // is just one exit statement: exit(<expr>);
    [[nodiscard]] std::optional<NodeExit> parse() {
        std::optional<NodeExit> exit_node;

        if (!peek() || peek()->type != TypeOfToken::exit)
            return std::nullopt;

        consume(); // eat "exit"

        if (!peek() || peek()->type != TypeOfToken::open_paren)
            throw std::runtime_error("Expected '(' after exit");

        consume(); // eat '('

        auto node_expr = parse_expr();

        if (!node_expr)
            throw std::runtime_error("Invalid expression after exit");

        exit_node = NodeExit{
            node_expr.value()
        };

        /*
        If a valid expression
        is found, it creates a `NodeExit` containing that expression.
        */

        if (!peek() || peek()->type != TypeOfToken::close_paren)
            throw std::runtime_error("Expected ')' after expression");

        consume(); // eat ')'

        if (!peek() || peek()->type != TypeOfToken::semi)
            throw std::runtime_error("Expected ';'");

        consume(); // eat ';'

        /*
        Finally, it verifies that the statement ends with a semicolon (`;`).
        If any required part is missing, a runtime error is thrown.
        */

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
    [[nodiscard]] std::optional<Token> peek(int offset = 0) const {
        if (m_index + offset < p_tokens.size())
            return p_tokens[m_index + offset];

        return std::nullopt;
    }

    // Same idea as the tokenizer's consume: return the current token
    // and move the index forward.
    [[nodiscard]] Token consume() {
        // it will consume the token not the character like before
        return p_tokens[m_index++];

        /*
        peek() returns optional<Token>, consume() returns Token directly (not optional) — because by the time I call consume(),
        I should've already checked peek() confirms a token exists.
        If I call consume() past the end here it'll crash (out-of-bounds),
        unlike the tokenizer's consume() which safely returns {}.
        */
    }

    std::vector<Token> p_tokens;
    std::size_t m_index = 0;
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
