#pragma once

#include <string>
#include <variant>
#include <vector>
#include "arena.h"

struct NodeExprIntLit {
    std::string value;
};

struct NodeExprIdentifier {
    std::string value;
};

struct NodeExpr;

struct BinaryExpr {
    NodeExpr *left;
    std::string op;
    NodeExpr *right = nullptr;
};

struct UnaryExpr {
    std::string op;
    NodeExpr *expr;
};

struct NodeExpr {
    std::variant<
        NodeExprIntLit,
        NodeExprIdentifier,
        BinaryExpr *,
        UnaryExpr *
    > expr;
};

struct NodeStmntExit {
    NodeExpr *expr;
};

struct NodeStmntLet {
    std::string identifier;
    NodeExpr *expr;
};

struct NodeStmntAssign {
    std::string identifier;
    NodeExpr *expr;
};

struct NodeScope;
struct NodeStmntIf;
struct NodeStmntWhile;

struct NodeStmnt {
    std::variant<
        NodeStmntExit,
        NodeStmntIf *,
        NodeStmntLet,
        NodeScope *,
        NodeStmntAssign,
        NodeStmntWhile *
    > stmnt;
};

struct NodeScope {
    std::vector<NodeStmnt *> statements;
};

struct NodeStmntIf {
    NodeExpr *condition;
    NodeScope *scope;
    NodeScope *else_scope = nullptr;
    NodeStmntIf *else_if = nullptr;
};

struct NodeStmntWhile {
    NodeExpr *condition;
    NodeScope *scope;
};

struct NodeProgram {
    std::vector<NodeStmnt *> statements;
};
