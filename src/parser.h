#pragma once

#include <string>

#include "arena.h"
#include "ast.h"

class Parser {
public:
    Parser(const std::string& source, ArenaAllocator& arena);

    NodeProgram parse();

private:
    const std::string& source;
    ArenaAllocator& arena;

    std::size_t position = 0;

    // -------------------------
    // Basic parser helpers
    // -------------------------

    void skip_whitespace();

    char peek() const;

    bool match(char c);
    bool match(const std::string& text);

    bool match_keyword(const std::string& keyword);

    void expect(char c);
    void expect(const std::string& text);
    void expect_keyword(const std::string& keyword);

    // -------------------------
    // Lexical helpers
    // -------------------------

    std::string parse_identifier();
    std::string parse_integer();

    // -------------------------
    // Expressions
    // -------------------------

    NodeExpr* parse_expression();

    NodeExpr* parse_logical_or();
    NodeExpr* parse_logical_and();

    NodeExpr* parse_comparison();

    NodeExpr* parse_additive();
    NodeExpr* parse_multiplicative();

    NodeExpr* parse_unary();
    NodeExpr* parse_primary();

    // -------------------------
    // Statements
    // -------------------------

    NodeStmnt* parse_statement();

    NodeStmnt* parse_set();
    NodeStmnt* parse_assignment();
    NodeStmnt* parse_exit();

    NodeStmnt* parse_if();
    NodeStmnt* parse_whilst();

    NodeScope* parse_scope();

    // -------------------------
    // AST construction
    // -------------------------

    NodeExpr* make_int(
        const std::string& value
    );

    NodeExpr* make_identifier(
        const std::string& value
    );

    NodeExpr* make_binary(
        NodeExpr* left,
        const std::string& op,
        NodeExpr* right
    );

    NodeExpr* make_unary(
        const std::string& op,
        NodeExpr* expr
    );
};