#pragma once

#include <vector>
#include <variant>
#include <optional>
#include <memory>
#include <stdexcept>
#include <string>
#include "tokenization.h"
#include "arena.h"


//we need nodes that represent a single expression.
struct NodeExprIntLit {
    Token int_lit; // this alone means that the expression should only accept integer literals like 1, 2, 3 etc.
};

struct NodeExprIdentifier {
    Token identifier; // represents the variables like x, y, z etc.
};

struct NodeExpr;

struct BinaryExpr {
    std::unique_ptr<NodeExpr> left;
    std::string op;
    std::unique_ptr<NodeExpr> right;
};

struct NodeExpr {
    std::variant<NodeExprIntLit, NodeExprIdentifier, std::unique_ptr<BinaryExpr> > expr;
};


struct NodeStmntExit {
    NodeExpr expr;
};

struct NodeStmntLet {
    Token identifier;
    NodeExpr expr;
};

struct NodeStmnt {
    std::variant<NodeStmntExit, NodeStmntLet> stmnt;
};

struct NodeProgram {
    std::vector<NodeStmnt> statements;
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

    [[nodiscard]] std::optional<NodeStmnt> parse_stmt() {
        // Parse:
        // set x = expression;

        if (peek() && peek()->type == TypeOfToken::set) {
            consume(); // eat "set"


            if (!peek() || peek()->type != TypeOfToken::identifier)
                throw std::runtime_error("Expected identifier after set");


            Token identifier = consume();


            if (!peek() || peek()->type != TypeOfToken::equals)
                throw std::runtime_error("Expected '=' after identifier");


            consume(); // eat '='


            auto expr = parse_expr();


            if (!expr)
                throw std::runtime_error("Expected expression after '='");


            if (!peek() || peek()->type != TypeOfToken::semi)
                throw std::runtime_error("Expected ';'");


            consume(); // eat ';'


            return NodeStmnt{
                NodeStmntLet{
                    identifier,
                    std::move(expr.value())
                }
            };
        }


        // Parse:
        // exit(expression);

        if (peek() && peek()->type == TypeOfToken::exit) {
            consume(); // eat "exit"


            if (!peek() || peek()->type != TypeOfToken::open_paren)
                throw std::runtime_error("Expected '(' after exit");


            consume(); // eat '('


            auto expr = parse_expr();


            if (!expr)
                throw std::runtime_error("Expected expression");


            if (!peek() || peek()->type != TypeOfToken::close_paren)
                throw std::runtime_error("Expected ')'");


            consume(); // eat ')'


            if (!peek() || peek()->type != TypeOfToken::semi)
                throw std::runtime_error("Expected ';'");


            consume(); // eat ';'


            return NodeStmnt{
                NodeStmntExit{
                    std::move(expr.value())
                }
            };
        }


        return std::nullopt;
    }

    // Tries to parse the whole program. Right now "the whole program"
    // is just one exit statement: exit(<expr>);
    [[nodiscard]] std::optional<NodeProgram> parse() {
        NodeProgram program;


        while (peek()) {
            auto statement = parse_stmt();


            if (!statement)
                throw std::runtime_error("Parsing failed");


            program.statements.push_back(
                std::move(*statement)
            );
        }


        return program;
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
