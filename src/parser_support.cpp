#include "parser_support.h"
#include <new>
#include <utility>

ArenaAllocator *arena = nullptr;
NodeProgram program;

NodeExpr *make_int_expr(const std::string &value)
{
    if (!arena)
        throw std::runtime_error("Nex parser arena is not initialized");

    NodeExpr *expr = arena->alloc<NodeExpr>(); // it makes a place in memory for that expression.

    if (!expr)
        throw std::bad_alloc{};

    return new (expr) NodeExpr{
        NodeExprIntLit{value}};
}

NodeStmnt *make_print_statement(NodeExpr *expr)
{
    if (!arena)
        throw std::runtime_error("Nex parser arena is not initialized");

    NodeStmnt *print_expr = arena->alloc<NodeStmnt>();

    if (!print_expr)
        throw std::bad_alloc{};
        
    return new (print_expr) NodeStmnt{
        NodeStmntPrint{expr}};
}
NodeExpr *make_string_expr(const std::string &value)
{
    if (!arena)
        throw std::runtime_error(
            "Nex parser arena is not initialized");
    NodeExpr *expr = arena->alloc<NodeExpr>();

    if (!expr)
        throw std::bad_alloc{};

    return new (expr) NodeExpr(
        NodeExpr{NodeExprStringLit{value}});
}

NodeExpr *make_identifier_expr(const std::string &value)
{
    if (!arena)
        throw std::runtime_error(
            "Nex parser arena is not initialized");

    NodeExpr *expr = arena->alloc<NodeExpr>();

    if (!expr)
        throw std::bad_alloc{};

    return new (expr) NodeExpr{
        NodeExprIdentifier{value}};
}

NodeExpr *make_binary_expr(
    NodeExpr *left,
    const std::string &op,
    NodeExpr *right)
{
    if (!arena)
        throw std::runtime_error(
            "Nex parser arena is not initialized");

    BinaryExpr *binary =
        arena->alloc<BinaryExpr>();

    if (!binary)
        throw std::bad_alloc{};

    new (binary) BinaryExpr{
        left,
        op,
        right};

    NodeExpr *expr =
        arena->alloc<NodeExpr>();

    if (!expr)
        throw std::bad_alloc{};

    return new (expr) NodeExpr{
        binary};
}

NodeExpr *make_unary_expr(
    const std::string &op,
    NodeExpr *operand)
{
    if (!arena)
        throw std::runtime_error(
            "Nex parser arena is not initialized");

    UnaryExpr *unary =
        arena->alloc<UnaryExpr>();

    if (!unary)
        throw std::bad_alloc{};

    new (unary) UnaryExpr{
        op,
        operand};

    NodeExpr *expr =
        arena->alloc<NodeExpr>();

    if (!expr)
        throw std::bad_alloc{};

    return new (expr) NodeExpr{
        unary};
}

NodeScope *make_scope()
{
    if (!arena)
        throw std::runtime_error(
            "Nex parser arena is not initialized");

    NodeScope *scope =
        arena->alloc<NodeScope>();

    if (!scope)
        throw std::bad_alloc{};

    return new (scope) NodeScope{};
}

NodeStmnt *make_statement()
{
    if (!arena)
        throw std::runtime_error(
            "Nex parser arena is not initialized");

    NodeStmnt *statement =
        arena->alloc<NodeStmnt>();

    if (!statement)
        throw std::bad_alloc{};

    return statement;
}

NodeStmnt *make_let(
    const std::string &identifier,
    NodeExpr *expr)
{
    NodeStmnt *statement =
        make_statement();

    return new (statement) NodeStmnt{
        NodeStmntLet{
            identifier,
            expr}};
}

NodeStmnt *make_assign(
    const std::string &identifier,
    NodeExpr *expr)
{
    NodeStmnt *statement =
        make_statement();

    return new (statement) NodeStmnt{
        NodeStmntAssign{
            identifier,
            expr}};
}

NodeStmnt *make_exit(NodeExpr *expr)
{
    NodeStmnt *statement =
        make_statement();

    return new (statement) NodeStmnt{
        NodeStmntExit{
            expr}};
}

NodeStmnt *make_scope_statement(NodeScope *scope)
{
    NodeStmnt *statement =
        make_statement();

    return new (statement) NodeStmnt{
        scope};
}

NodeStmnt *make_if(
    NodeExpr *condition,
    NodeScope *scope,
    NodeScope *else_scope,
    NodeStmntIf *else_if)
{
    if (!arena)
        throw std::runtime_error(
            "Nex parser arena is not initialized");

    NodeStmntIf *if_statement =
        arena->alloc<NodeStmntIf>();

    if (!if_statement)
        throw std::bad_alloc{};

    new (if_statement) NodeStmntIf{
        condition,
        scope,
        else_scope,
        else_if};

    NodeStmnt *statement =
        make_statement();

    return new (statement) NodeStmnt{
        if_statement};
}

NodeStmntIf *get_if_statement(NodeStmnt *statement)
{
    if (!statement)
        throw std::runtime_error(
            "Expected if statement");

    return std::get<NodeStmntIf *>(
        statement->stmnt);
}

NodeStmnt *make_while(
    NodeExpr *condition,
    NodeScope *scope)
{
    if (!arena)
        throw std::runtime_error(
            "Nex parser arena is not initialized");

    NodeStmntWhile *while_statement =
        arena->alloc<NodeStmntWhile>();

    if (!while_statement)
        throw std::bad_alloc{};

    new (while_statement) NodeStmntWhile{
        condition,
        scope};

    NodeStmnt *statement =
        make_statement();

    return new (statement) NodeStmnt{
        while_statement};
}
NodeStmnt *make_for(
    NodeStmnt *init,
    NodeExpr *condition,
    NodeStmnt *increment,
    NodeScope *scope)
{
    if (!arena)
        throw std::runtime_error(
            "Nex parser arena is not initialized");

    NodeStmntFor *for_statement =
        arena->alloc<NodeStmntFor>();

    if (!for_statement)
        throw std::bad_alloc{};

    new (for_statement) NodeStmntFor{
        init,
        condition,
        increment,
        scope};

    NodeStmnt *statement =
        make_statement();

    return new (statement) NodeStmnt{
        for_statement};
}