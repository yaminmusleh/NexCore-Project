#pragma once

#include <string>
#include <variant>
#include <vector>
#include "arena.h"

struct NodeExprIntLit
{
    std::string value;
};

struct NodeExprStringLit
{
    std::string value;
};
struct NodeExprFloatLit
{
    std::string value;
};
struct NodeExprDoubleLit
{
    std::string value;
};

struct NodeExprIdentifier
{
    std::string value;
};

struct NodeExpr;

struct BinaryExpr
{
    NodeExpr *left;
    std::string op;
    NodeExpr *right = nullptr;
};

struct UnaryExpr
{
    std::string op;
    NodeExpr *expr;
};

struct NodeExpr
{
    std::variant<
        NodeExprIntLit,
        NodeExprFloatLit,
        NodeExprDoubleLit,
        NodeExprIdentifier,
        NodeExprStringLit,
        BinaryExpr *,
        UnaryExpr *>
        expr;
};

struct NodeStmntExit
{
    NodeExpr *expr;
};

struct NodeStmntLet
{
    std::string identifier;
    NodeExpr *expr;
};

struct NodeStmntAssign
{
    std::string identifier;
    NodeExpr *expr;
};

struct NodeStmntPrint
{
    NodeExpr *expr;
};

struct NodeScope;
struct NodeStmntIf;
struct NodeStmntWhile;
struct NodeStmntFor;

struct NodeStmnt
{
    std::variant<
        NodeStmntExit,
        NodeStmntIf *,
        NodeStmntLet,
        NodeScope *,
        NodeStmntAssign,
        NodeStmntPrint,
        NodeStmntWhile *,
        NodeStmntFor *>
        stmnt;
    // Simple statement nodes are stored by value.
    // Recursive/complex AST structures are stored by pointer
    // so they can be separately allocated and referenced.
};

struct NodeScope
{
    std::vector<NodeStmnt *> statements;
};

struct NodeStmntIf
{
    NodeExpr *condition;
    NodeScope *scope;
    NodeScope *else_scope = nullptr;
    NodeStmntIf *else_if = nullptr;
};

struct NodeStmntWhile
{
    NodeExpr *condition;
    NodeScope *scope;
};

struct NodeStmntFor
{
    NodeStmnt *init = nullptr;
    NodeExpr *condition = nullptr;
    NodeStmnt *increment = nullptr;
    NodeScope *scope = nullptr;
};

struct NodeProgram
{
    std::vector<NodeStmnt *> statements;
};
