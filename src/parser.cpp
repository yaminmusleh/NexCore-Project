#include "parser.h"

#include <cctype>
#include <new>
#include <stdexcept>

// ============================================================
// Constructor
// ============================================================

Parser::Parser(
    const std::string& source,
    ArenaAllocator& arena
)
    : source(source),
      arena(arena) {}


// ============================================================
// Basic helpers
// ============================================================

void Parser::skip_whitespace() {
    while (
        position < source.size() &&
        std::isspace(
            static_cast<unsigned char>(
                source[position]
            )
        )
    ) {
        ++position;
    }
}


char Parser::peek() const {
    if (position >= source.size())
        return '\0';

    return source[position];
}


bool Parser::match(char c) {
    skip_whitespace();

    if (peek() != c)
        return false;

    ++position;
    return true;
}


bool Parser::match(const std::string& text) {
    skip_whitespace();

    if (
        source.compare(
            position,
            text.size(),
            text
        ) != 0
    ) {
        return false;
    }

    position += text.size();

    return true;
}


// ------------------------------------------------------------
// Keyword matching
//
// Unlike match("if"), this checks that the keyword is not
// followed by an identifier character.
//
// So:
//
//     if
//
// matches.
//
// But:
//
//     iffy
//
// does not.
// ------------------------------------------------------------

bool Parser::match_keyword(
    const std::string& keyword
) {
    skip_whitespace();

    if (
        source.compare(
            position,
            keyword.size(),
            keyword
        ) != 0
    ) {
        return false;
    }

    std::size_t end =
        position + keyword.size();

    if (end < source.size()) {
        char next = source[end];

        if (
            std::isalnum(
                static_cast<unsigned char>(next)
            ) ||
            next == '_'
        ) {
            return false;
        }
    }

    position = end;

    return true;
}


void Parser::expect(char c) {
    if (!match(c)) {
        throw std::runtime_error(
            std::string("Expected '") + c + "'"
        );
    }
}


void Parser::expect(
    const std::string& text
) {
    if (!match(text)) {
        throw std::runtime_error(
            "Expected '" + text + "'"
        );
    }
}


void Parser::expect_keyword(
    const std::string& keyword
) {
    if (!match_keyword(keyword)) {
        throw std::runtime_error(
            "Expected '" + keyword + "'"
        );
    }
}


// ============================================================
// Identifier
// ============================================================

std::string Parser::parse_identifier() {
    skip_whitespace();

    char c = peek();

    if (
        !std::isalpha(
            static_cast<unsigned char>(c)
        ) &&
        c != '_'
    ) {
        throw std::runtime_error(
            "Expected identifier"
        );
    }

    std::string value;

    while (true) {
        c = peek();

        if (
            !std::isalnum(
                static_cast<unsigned char>(c)
            ) &&
            c != '_'
        ) {
            break;
        }

        value += c;
        ++position;
    }

    return value;
}


// ============================================================
// Integer
// ============================================================

std::string Parser::parse_integer() {
    skip_whitespace();

    if (
        !std::isdigit(
            static_cast<unsigned char>(peek())
        )
    ) {
        throw std::runtime_error(
            "Expected integer"
        );
    }

    std::string value;

    while (
        std::isdigit(
            static_cast<unsigned char>(peek())
        )
    ) {
        value += peek();
        ++position;
    }

    return value;
}


// ============================================================
// AST constructors
// ============================================================

NodeExpr* Parser::make_int(
    const std::string& value
) {
    NodeExpr* node =
        arena.alloc<NodeExpr>();

    new (node) NodeExpr{
        NodeExprIntLit{value}
    };

    return node;
}


NodeExpr* Parser::make_identifier(
    const std::string& value
) {
    NodeExpr* node =
        arena.alloc<NodeExpr>();

    new (node) NodeExpr{
        NodeExprIdentifier{value}
    };

    return node;
}


NodeExpr* Parser::make_binary(
    NodeExpr* left,
    const std::string& op,
    NodeExpr* right
) {
    BinaryExpr* binary =
        arena.alloc<BinaryExpr>();

    new (binary) BinaryExpr{
        left,
        op,
        right
    };

    NodeExpr* node =
        arena.alloc<NodeExpr>();

    new (node) NodeExpr{
        binary
    };

    return node;
}


NodeExpr* Parser::make_unary(
    const std::string& op,
    NodeExpr* expr
) {
    UnaryExpr* unary =
        arena.alloc<UnaryExpr>();

    new (unary) UnaryExpr{
        op,
        expr
    };

    NodeExpr* node =
        arena.alloc<NodeExpr>();

    new (node) NodeExpr{
        unary
    };

    return node;
}


// ============================================================
// Primary
//
// primary:
//
//     integer
//     identifier
//     ( expression )
// ============================================================

NodeExpr* Parser::parse_primary() {
    skip_whitespace();

    if (peek() == '\0') {
        throw std::runtime_error(
            "Unexpected end of input"
        );
    }

    // Integer
    if (
        std::isdigit(
            static_cast<unsigned char>(peek())
        )
    ) {
        return make_int(
            parse_integer()
        );
    }

    // Identifier
    if (
        std::isalpha(
            static_cast<unsigned char>(peek())
        ) ||
        peek() == '_'
    ) {
        return make_identifier(
            parse_identifier()
        );
    }

    // Parenthesized expression
    if (match('(')) {
        NodeExpr* expr =
            parse_expression();

        expect(')');

        return expr;
    }

    throw std::runtime_error(
        std::string(
            "Unexpected character: "
        ) + peek()
    );
}


// ============================================================
// Unary
//
// unary:
//
//     -unary
//     !unary
//     primary
// ============================================================

NodeExpr* Parser::parse_unary() {
    if (match('-')) {
        return make_unary(
            "-",
            parse_unary()
        );
    }

    if (match('!')) {
        return make_unary(
            "!",
            parse_unary()
        );
    }

    return parse_primary();
}


// ============================================================
// Multiplication / division / modulo
//
// multiplicative:
//
//     unary
//     multiplicative * unary
//     multiplicative / unary
//     multiplicative % unary
// ============================================================

NodeExpr* Parser::parse_multiplicative() {
    NodeExpr* left =
        parse_unary();

    while (true) {
        if (match('*')) {
            left = make_binary(
                left,
                "*",
                parse_unary()
            );
        }
        else if (match('/')) {
            left = make_binary(
                left,
                "/",
                parse_unary()
            );
        }
        else if (match('%')) {
            left = make_binary(
                left,
                "%",
                parse_unary()
            );
        }
        else {
            break;
        }
    }

    return left;
}


// ============================================================
// Addition / subtraction
// ============================================================

NodeExpr* Parser::parse_additive() {
    NodeExpr* left =
        parse_multiplicative();

    while (true) {
        if (match('+')) {
            left = make_binary(
                left,
                "+",
                parse_multiplicative()
            );
        }
        else if (match('-')) {
            left = make_binary(
                left,
                "-",
                parse_multiplicative()
            );
        }
        else {
            break;
        }
    }

    return left;
}


// ============================================================
// Comparisons
//
// comparison:
//
//     additive
//     additive == additive
//     additive != additive
//     additive < additive
//     additive <= additive
//     additive > additive
//     additive >= additive
// ============================================================

NodeExpr* Parser::parse_comparison() {
    NodeExpr* left =
        parse_additive();

    skip_whitespace();

    if (match("==")) {
        return make_binary(
            left,
            "==",
            parse_additive()
        );
    }

    if (match("!=")) {
        return make_binary(
            left,
            "!=",
            parse_additive()
        );
    }

    if (match("<=")) {
        return make_binary(
            left,
            "<=",
            parse_additive()
        );
    }

    if (match(">=")) {
        return make_binary(
            left,
            ">=",
            parse_additive()
        );
    }

    if (match("<")) {
        return make_binary(
            left,
            "<",
            parse_additive()
        );
    }

    if (match(">")) {
        return make_binary(
            left,
            ">",
            parse_additive()
        );
    }

    return left;
}


// ============================================================
// Logical AND
//
// &
//
// ============================================================

NodeExpr* Parser::parse_logical_and() {
    NodeExpr* left =
        parse_comparison();

    while (match('&')) {
        left = make_binary(
            left,
            "&",
            parse_comparison()
        );
    }

    return left;
}


// ============================================================
// Logical OR
//
// |
// ============================================================

NodeExpr* Parser::parse_logical_or() {
    NodeExpr* left =
        parse_logical_and();

    while (match('|')) {
        left = make_binary(
            left,
            "|",
            parse_logical_and()
        );
    }

    return left;
}


// ============================================================
// Expression
// ============================================================

NodeExpr* Parser::parse_expression() {
    return parse_logical_or();
}


// ============================================================
// set
//
// set x = expression;
// ============================================================

NodeStmnt* Parser::parse_set() {
    expect_keyword("set");

    std::string identifier =
        parse_identifier();

    expect('=');

    NodeExpr* expr =
        parse_expression();

    expect(';');

    NodeStmnt* statement =
        arena.alloc<NodeStmnt>();

    new (statement) NodeStmnt{
        NodeStmntLet{
            identifier,
            expr
        }
    };

    return statement;
}


// ============================================================
// assignment
//
// x = expression;
// ============================================================

NodeStmnt* Parser::parse_assignment() {
    std::string identifier =
        parse_identifier();

    expect('=');

    NodeExpr* expr =
        parse_expression();

    expect(';');

    NodeStmnt* statement =
        arena.alloc<NodeStmnt>();

    new (statement) NodeStmnt{
        NodeStmntAssign{
            identifier,
            expr
        }
    };

    return statement;
}


// ============================================================
// exit
//
// exit(expression);
// ============================================================

NodeStmnt* Parser::parse_exit() {
    expect_keyword("exit");

    expect('(');

    NodeExpr* expr =
        parse_expression();

    expect(')');

    expect(';');

    NodeStmnt* statement =
        arena.alloc<NodeStmnt>();

    new (statement) NodeStmnt{
        NodeStmntExit{
            expr
        }
    };

    return statement;
}


// ============================================================
// Scope
//
// {
//     statements
// }
// ============================================================

NodeScope* Parser::parse_scope() {
    expect('{');

    NodeScope* scope =
        arena.alloc<NodeScope>();

    new (scope) NodeScope{};

    while (true) {
        skip_whitespace();

        if (peek() == '\0') {
            throw std::runtime_error(
                "Unexpected end of input; expected '}'"
            );
        }

        if (match('}')) {
            break;
        }

        NodeStmnt* statement =
            parse_statement();

        scope->statements.push_back(
            statement
        );
    }

    return scope;
}


// ============================================================
// if
//
// if (condition) {
//     ...
// }
//
// else if (condition) {
//     ...
// }
//
// else {
//     ...
// }
// ============================================================

NodeStmnt* Parser::parse_if() {
    expect_keyword("if");

    expect('(');

    NodeExpr* condition =
        parse_expression();

    expect(')');

    NodeScope* scope =
        parse_scope();

    NodeScope* else_scope =
        nullptr;

    NodeStmntIf* else_if =
        nullptr;

    skip_whitespace();

    if (match_keyword("else")) {
        skip_whitespace();

        if (match_keyword("if")) {
            // We consumed "if", so parse the remainder
            // of an if statement manually.

            expect('(');

            NodeExpr* else_condition =
                parse_expression();

            expect(')');

            NodeScope* else_if_scope =
                parse_scope();

            NodeScope* nested_else_scope =
                nullptr;

            NodeStmntIf* nested_else_if =
                nullptr;

            skip_whitespace();

            if (match_keyword("else")) {
                skip_whitespace();

                if (match_keyword("if")) {
                    // Build the nested else-if recursively
                    // by parsing the rest manually.

                    expect('(');

                    NodeExpr* nested_condition =
                        parse_expression();

                    expect(')');

                    NodeScope* nested_scope =
                        parse_scope();

                    nested_else_if =
                        arena.alloc<NodeStmntIf>();

                    new (nested_else_if)
                        NodeStmntIf{
                            nested_condition,
                            nested_scope,
                            nullptr,
                            nullptr
                        };
                }
                else {
                    nested_else_scope =
                        parse_scope();
                }
            }

            else_if =
                arena.alloc<NodeStmntIf>();

            new (else_if)
                NodeStmntIf{
                    else_condition,
                    else_if_scope,
                    nested_else_scope,
                    nested_else_if
                };
        }
        else {
            else_scope =
                parse_scope();
        }
    }

    NodeStmntIf* if_node =
        arena.alloc<NodeStmntIf>();

    new (if_node)
        NodeStmntIf{
            condition,
            scope,
            else_scope,
            else_if
        };

    NodeStmnt* statement =
        arena.alloc<NodeStmnt>();

    new (statement) NodeStmnt{
        if_node
    };

    return statement;
}


// ============================================================
// whilst
//
// whilst (condition) {
//     ...
// }
// ============================================================

NodeStmnt* Parser::parse_whilst() {
    expect_keyword("whilst");

    expect('(');

    NodeExpr* condition =
        parse_expression();

    expect(')');

    NodeScope* scope =
        parse_scope();

    NodeStmntWhile* while_node =
        arena.alloc<NodeStmntWhile>();

    new (while_node)
        NodeStmntWhile{
            condition,
            scope
        };

    NodeStmnt* statement =
        arena.alloc<NodeStmnt>();

    new (statement) NodeStmnt{
        while_node
    };

    return statement;
}


// ============================================================
// Statement dispatcher
// ============================================================

NodeStmnt* Parser::parse_statement() {
    skip_whitespace();

    if (peek() == '\0') {
        throw std::runtime_error(
            "Unexpected end of input"
        );
    }

    // IMPORTANT:
    // Check keywords before checking generic identifiers.

    if (match_keyword("set")) {
        // We already consumed "set".
        // Parse the remainder of a set statement.

        std::string identifier =
            parse_identifier();

        expect('=');

        NodeExpr* expr =
            parse_expression();

        expect(';');

        NodeStmnt* statement =
            arena.alloc<NodeStmnt>();

        new (statement) NodeStmnt{
            NodeStmntLet{
                identifier,
                expr
            }
        };

        return statement;
    }

    if (match_keyword("if")) {
        expect('(');

        NodeExpr* condition =
            parse_expression();

        expect(')');

        NodeScope* scope =
            parse_scope();

        NodeScope* else_scope =
            nullptr;

        NodeStmntIf* else_if =
            nullptr;

        skip_whitespace();

        if (match_keyword("else")) {
            skip_whitespace();

            if (match_keyword("if")) {
                expect('(');

                NodeExpr* else_condition =
                    parse_expression();

                expect(')');

                NodeScope* else_if_scope =
                    parse_scope();

                else_if =
                    arena.alloc<NodeStmntIf>();

                new (else_if)
                    NodeStmntIf{
                        else_condition,
                        else_if_scope,
                        nullptr,
                        nullptr
                    };

                // Handle one more else branch.
                skip_whitespace();

                if (match_keyword("else")) {
                    else_if->else_scope =
                        parse_scope();
                }
            }
            else {
                else_scope =
                    parse_scope();
            }
        }

        NodeStmntIf* if_node =
            arena.alloc<NodeStmntIf>();

        new (if_node)
            NodeStmntIf{
                condition,
                scope,
                else_scope,
                else_if
            };

        NodeStmnt* statement =
            arena.alloc<NodeStmnt>();

        new (statement) NodeStmnt{
            if_node
        };

        return statement;
    }

    if (match_keyword("whilst")) {
        expect('(');

        NodeExpr* condition =
            parse_expression();

        expect(')');

        NodeScope* scope =
            parse_scope();

        NodeStmntWhile* while_node =
            arena.alloc<NodeStmntWhile>();

        new (while_node)
            NodeStmntWhile{
                condition,
                scope
            };

        NodeStmnt* statement =
            arena.alloc<NodeStmnt>();

        new (statement) NodeStmnt{
            while_node
        };

        return statement;
    }

    if (match_keyword("exit")) {
        expect('(');

        NodeExpr* expr =
            parse_expression();

        expect(')');

        expect(';');

        NodeStmnt* statement =
            arena.alloc<NodeStmnt>();

        new (statement) NodeStmnt{
            NodeStmntExit{
                expr
            }
        };

        return statement;
    }

    // Anything remaining that starts like an identifier
    // is an assignment.

    if (
        std::isalpha(
            static_cast<unsigned char>(peek())
        ) ||
        peek() == '_'
    ) {
        return parse_assignment();
    }

    throw std::runtime_error(
        std::string("Expected statement, got '")
        + peek()
        + "'"
    );
}


// ============================================================
// Program
// ============================================================

NodeProgram Parser::parse() {
    NodeProgram program{};

    while (true) {
        skip_whitespace();

        if (peek() == '\0')
            break;

        program.statements.push_back(
            parse_statement()
        );
    }

    return program;
}