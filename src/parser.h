#pragma once

#include <vector>
#include <variant>
#include <optional>
#include <stdexcept>
#include <string>
#include <new>
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
    NodeExpr *left;
    std::string op;
    NodeExpr *right;
};

struct NodeExpr {
    std::variant<NodeExprIntLit, NodeExprIdentifier, BinaryExpr *> expr;
};


struct NodeStmntExit {
    NodeExpr *expr;
};

struct NodeStmntLet {
    Token identifier;
    NodeExpr *expr;
};

struct NodeScope;

struct NodeStmnt {
    std::variant<NodeStmntExit, NodeStmntLet, NodeScope *> stmnt;
};

struct NodeScope {
    std::vector<NodeStmnt *> statements;
};

struct NodeProgram {
    std::vector<NodeStmnt *> statements;
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
    Parser(std::vector<Token> tokens, ArenaAllocator &arena)
        : p_tokens(std::move(tokens)),
          m_arena(arena) {
    }


    [[nodiscard]] NodeExpr *parse_expr() {
        NodeExpr *left = parse_term();

        while (peek()) {
            TypeOfToken type = peek()->type;

            if (type != TypeOfToken::plus &&
                type != TypeOfToken::minus)
                break;


            Token op = consume();

            NodeExpr *right = parse_term();

            if (!right)
                throw std::runtime_error("Expected expression");


            BinaryExpr *binary = m_arena.alloc<BinaryExpr>();

            if (!binary)
                throw std::bad_alloc{};

            new(binary) BinaryExpr{
                left,
                op.value.value(),
                right
            };


            NodeExpr *expr = m_arena.alloc<NodeExpr>();

            left = new(expr) NodeExpr{
                binary
            };
        }

        return left;
    }

    NodeExpr *parse_term() {
        NodeExpr *left = parse_primary();


        if (!left)
            throw std::runtime_error("Expected expression");

        while (peek()) {
            TypeOfToken type = peek()->type;


            if (type != TypeOfToken::star &&
                type != TypeOfToken::slash)
                break;


            Token op = consume();

            NodeExpr *right = parse_primary();

            if (!right)
                throw std::runtime_error("Expected expression after operator");


            BinaryExpr *binary = m_arena.alloc<BinaryExpr>();

            if (!binary)
                throw std::bad_alloc{};
            new(binary) BinaryExpr{
                left,
                op.value.value(),
                right
            };


            NodeExpr *expr = m_arena.alloc<NodeExpr>();
            if (!expr)
                throw std::bad_alloc{};

            left = new(expr) NodeExpr{
                binary
            };
        }


        return left;
    }

    [[nodiscard]] NodeScope parse_scope() {
        if (!peek() || peek()->type != TypeOfToken::open_scope)
            throw std::runtime_error("Expected '{'");

        consume(); // {

        NodeScope scope;

        while (peek() && peek()->type != TypeOfToken::close_scope) {
            NodeStmnt *stmt = parse_stmt();

            if (!stmt)
                throw std::runtime_error("Expected statement");

            scope.statements.push_back(stmt);
        }

        if (!peek())
            throw std::runtime_error("Expected '}'");

        consume(); // }

        return scope;
    }

    // Tries to parse a single expression.
    // Currently supports:
    //  - integer literals
    //  - identifiers
    [[nodiscard]] NodeExpr *parse_primary() {
        if (peek() && peek()->type == TypeOfToken::int_lit) {
            NodeExpr *memory = m_arena.alloc<NodeExpr>();

            if (!memory)
                throw std::bad_alloc{};

            return new(memory) NodeExpr{
                NodeExprIntLit{consume()}
            };
        }

        if (peek() && peek()->type == TypeOfToken::open_paren) {
            consume();

            NodeExpr *expr = parse_expr();

            if (!peek() || peek()->type != TypeOfToken::close_paren)
                throw std::runtime_error("Expected ')'");

            consume();

            return expr;
        }

        if (peek() && peek()->type == TypeOfToken::identifier) {
            NodeExpr *memory = m_arena.alloc<NodeExpr>();

            if (!memory)
                throw std::bad_alloc{};

            return new(memory) NodeExpr{
                NodeExprIdentifier{consume()}
            };
        }


        return nullptr;
    }


    [[nodiscard]] NodeStmnt *parse_stmt() {

        if (peek() && peek()->type == TypeOfToken::open_scope)
        {
            NodeScope parsed = parse_scope();

            NodeScope* scope = m_arena.alloc<NodeScope>();

            if (!scope)
                throw std::bad_alloc{};

            new(scope) NodeScope(std::move(parsed));

            NodeStmnt* stmt = m_arena.alloc<NodeStmnt>();

            if (!stmt)
                throw std::bad_alloc{};

            return new(stmt) NodeStmnt{scope};
        }
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


            NodeExpr *expr = parse_expr();


            if (!expr)
                throw std::runtime_error("Expected expression after '='");


            if (!peek() || peek()->type != TypeOfToken::semi)
                throw std::runtime_error("Expected ';'");


            consume(); // eat ';'


            NodeStmnt *memory = m_arena.alloc<NodeStmnt>();

            if (!memory)
                throw std::bad_alloc{};


            return new(memory) NodeStmnt{
                NodeStmntLet{
                    identifier,
                    expr
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


            NodeExpr *expr = parse_expr();


            if (!expr)
                throw std::runtime_error("Expected expression");


            if (!peek() || peek()->type != TypeOfToken::close_paren)
                throw std::runtime_error("Expected ')'");


            consume(); // eat ')'


            if (!peek() || peek()->type != TypeOfToken::semi)
                throw std::runtime_error("Expected ';'");


            consume(); // eat ';'


            NodeStmnt *memory = m_arena.alloc<NodeStmnt>();

            if (!memory)
                throw std::bad_alloc{};


            return new(memory) NodeStmnt{
                NodeStmntExit{
                    expr
                }
            };
        }


        return nullptr;
    }


    // Tries to parse the whole program.
    // Currently supports multiple statements:
    // set x = expression;
    // exit(expression);
    [[nodiscard]] NodeProgram parse() {
        NodeProgram program;


        while (peek()) {
            NodeStmnt *statement = parse_stmt();


            if (!statement)
                throw std::runtime_error("Parsing failed");


            program.statements.push_back(statement);
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

    // The arena is owned outside the parser.
    // This keeps AST nodes alive after parsing finishes.
    ArenaAllocator &m_arena;
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
