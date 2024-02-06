#pragma once
#include <climits>
#include <set>
#include <variant>
#include <cmath>

#include "arena_allocator.hpp"
#include "tokenizer.hpp"

using namespace std;

struct NodeExpr;

struct NodeTermIntLit {
    string value;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeTerm {
    variant<NodeTermIntLit *, NodeTermIdent *, NodeTermParen *> var;
};

struct NodeArithExpr {
    TokenType type;
    NodeExpr* left;
    NodeExpr* right;
};

struct NodeExpr {
    variant<NodeTerm *, NodeArithExpr *> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtVariable {
    Token ident;
    NodeExpr* expr{};
};

struct NodeStatement {
    variant<NodeStmtExit *, NodeStmtVariable *> var;
};

struct NodeStart {
    vector<NodeStatement *> statements;
};


class Parser {
public:
    explicit Parser(const vector<Token>&tokens) : tokens(tokens), allocator(1024 * 1024 * 4) {
    }


    static int digit_count(const int i) {
        return static_cast<int>(log10(static_cast<double>(i))) + 1;
    }

    NodeTermIntLit* parse_number() {
        next_token(int_tokens, true);

        int result = 0;

        int multiplier = 1;
        int prev_result = INT_MIN;

        do {
            if (it->type == TokenType::int_lit_mul) {
                const int new_multiplier = multipliers[it->value.value()];

                if (new_multiplier <= multiplier) {
                    cerr << "Invalid number" << endl;
                    exit(EXIT_FAILURE);
                }

                multiplier = new_multiplier;
                continue;
            }

            const int value = num_values[it->value.value()];
            const int curr_result = multiplier * value;

            if (digit_count(prev_result) >= digit_count(curr_result)) {
                cerr << "Invalid number" << endl;
                exit(EXIT_FAILURE);
            }

            prev_result = curr_result;
            result += prev_result;
        }
        while (next_token(int_tokens, false));

        auto* int_lit = allocator.alloc<NodeTermIntLit>();
        const string&result_str = to_string(result);
        int_lit->value = result_str;

        return int_lit;
    }

    [[nodiscard]] NodeExpr* parse_expr(const int min_prec = 0) {
        next_token(term_tokens, true);

        auto* node_expr = allocator.alloc<NodeExpr>();
        NodeTerm* node_term = parse_term();
        node_expr->var = node_term;

        while (true) {
            if (!next_token(arithmetic_tokens, false)) {
                break;
            }

            const TokenType op = it->type;
            const int prec = get_prec(op);

            if (prec < min_prec) {
                --it;
                break;
            }

            const auto expr_rhs = parse_expr(prec + 1);

            auto* node_arith_expr = allocator.alloc<NodeArithExpr>();
            const auto expr_lhs = allocator.alloc<NodeExpr>();

            expr_lhs->var = node_expr->var;

            node_arith_expr->left = expr_lhs;
            node_arith_expr->right = expr_rhs;
            node_arith_expr->type = op;


            node_expr->var = node_arith_expr;
        }


        return node_expr;
    }

    [[nodiscard]] NodeTerm* parse_term() {
        const auto term = allocator.alloc<NodeTerm>();

        switch (it->type) {
            case TokenType::sq_brkt_open: {
                NodeTermIntLit* int_lit = parse_number();
                term->var = int_lit;

                next_token({TokenType::sq_brkt_close}, true);
                break;
            }
            case TokenType::backtick: {
                next_token({TokenType::var_ident}, true);

                auto term_ident = allocator.alloc<NodeTermIdent>();
                term_ident->ident = *it;
                term->var = term_ident;

                next_token({TokenType::backtick}, true);
                break;
            }
            case TokenType::paren_open: {
                const auto expr = parse_expr();

                next_token({TokenType::paren_close}, true);
                auto term_paren = allocator.alloc<NodeTermParen>();

                term_paren->expr = expr;
                term->var = term_paren;
                break;
            }
            default: assert(false); //Unreachable
        }


        return term;
    }

    NodeStatement* parse_statement() {
        auto* node_statement = allocator.alloc<NodeStatement>();

        if (it->type == TokenType::exit) {
            next_token({TokenType::paren_open}, true);

            NodeExpr* expr = parse_expr();

            next_token({TokenType::paren_close}, true);

            auto* node_stmt_exit = allocator.alloc<NodeStmtExit>();
            node_stmt_exit->expr = expr;

            node_statement->var = node_stmt_exit;
        }
        else if (it->type == TokenType::var_decl) {
            next_token({TokenType::var_type}, true);

            next_token({TokenType::backtick}, true);
            next_token({TokenType::var_ident}, true);
            const Token ident = *it;
            next_token({TokenType::backtick}, true);

            next_token({TokenType::var_assign}, true);

            NodeExpr* expr = parse_expr();

            auto* node_stmt_variable = allocator.alloc<NodeStmtVariable>();
            node_stmt_variable->ident = ident;
            node_stmt_variable->expr = expr;

            node_statement->var = node_stmt_variable;
        }
        else {
            cerr << "Not a statement '" << token_names[it->type] << "'!" << endl;
            exit(EXIT_FAILURE);
        }

        return node_statement;
    }

    optional<NodeStart> parse_program() {
        NodeStart start_node;

        for (; it != tokens.end(); ++it) {
            start_node.statements.push_back(parse_statement());
        }

        return start_node;
    }

private:
    vector<Token> tokens;
    vector<Token>::iterator it = tokens.begin();
    ArenaAllocator allocator;

    bool next_token(const set<TokenType>&expected, const bool required) {
        const auto next = (it + 1);

        if (next == tokens.end()) {
            if (!required) return false;

            cerr << "Expected '" << get_token_names(expected) << "', found nothing" << endl;
            exit(EXIT_FAILURE);
        }

        if (!expected.contains(next->type)) {
            if (!required) return false;

            cerr << "Expected '" << get_token_names(expected) << "', found '" << token_names[next->type] << "'" << endl;
            exit(EXIT_FAILURE);
        }

        ++it;
        return true;
    }
};
