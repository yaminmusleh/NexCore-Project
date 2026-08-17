#pragma once

#include "arena.h"
#include "ast.h"
#include "Scanner.h"

#include <cwchar>
#include <stdexcept>
#include <string>

inline std::string token_string(Token *token)
{
    if (!token || !token->val)
        return {};

    std::wstring wide(token->val);

    return {wide.begin(), wide.end()};
}

extern ArenaAllocator *arena;
extern NodeProgram program;

NodeExpr *make_int_expr(const std::string &value);
NodeExpr *make_string_expr(const std::string &value);
NodeExpr *make_identifier_expr(const std::string &value);

NodeExpr *make_binary_expr(
    NodeExpr *left,
    const std::string &op,
    NodeExpr *right);

NodeExpr *make_unary_expr(
    const std::string &op,
    NodeExpr *operand);

NodeScope *make_scope();

NodeStmnt *make_statement();

NodeStmnt *make_let(
    const std::string &identifier,
    NodeExpr *expr);

NodeStmnt *make_assign(
    const std::string &identifier,
    NodeExpr *expr);

NodeStmnt *make_exit(NodeExpr *expr);

NodeStmnt *make_scope_statement(NodeScope *scope);

NodeStmnt *make_if(
    NodeExpr *condition,
    NodeScope *scope,
    NodeScope *else_scope,
    NodeStmntIf *else_if);

NodeStmnt *make_for(
    NodeStmnt *init,
    NodeExpr *condition,
    NodeStmnt *increment,
    NodeScope *scope);

NodeStmntIf *get_if_statement(NodeStmnt *statement);

NodeStmnt *make_while(
    NodeExpr *condition,
    NodeScope *scope);