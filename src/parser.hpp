#pragma once
#include <climits>
#include <set>
#include <variant>
#include <cmath>

#include "arena_allocator.hpp"
#include "tokenizer.hpp"

using namespace std;

struct NodeExpr;
struct NodeStatement;

struct NodeTermIntLit {
    string value;
};

struct NodeTermBoolLit {
    bool value;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeTerm {
    variant<NodeTermBoolLit *, NodeTermIntLit *, NodeTermIdent *, NodeTermParen *> var;
};

struct NodeBinExpr {
    TokenType type;
    NodeExpr* left;
    NodeExpr* right;
};

struct NodeExpr {
    variant<NodeTerm *, NodeBinExpr *> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtVariable {
    Token ident;
    TokenType type{};
    NodeExpr* expr{};
};

struct NodeStmtScope {
    vector<NodeStatement *> statements;
};

struct NodeIfPred {
    NodeExpr* expr;
    NodeStatement* stmt;
};

struct NodeIfPredElse {
    NodeStatement* stmt;
};

struct NodeStmtIf {
    NodeIfPred* pred;
    optional<NodeIfPredElse *> pred_else;
    vector<NodeIfPred *> pred_elif;
};

struct NodeStmtWhile {
    optional<NodeExpr *> expr;
    NodeStatement* stmt{};
};

struct NodeStmtBreak {
};

struct NodeStmtContinue {
};

struct NodeStmtAssign {
    Token ident;
    NodeExpr* expr{};
};

struct NodeStmtPrint {
    NodeExpr* expr;
};

struct NodeStatement {
    variant<NodeStmtExit *, NodeStmtVariable *, NodeStmtScope *, NodeStmtIf *, NodeStmtAssign *, NodeStmtWhile *,
        NodeStmtBreak *, NodeStmtContinue *, NodeStmtPrint *> var;
};

struct NodeStart {
    vector<NodeStatement *> statements;
};


class Parser {
public:
    explicit Parser(const vector<Token>&tokens) : tokens(tokens), allocator(1024 * 1024 * 4) {
    }


    static int digit_count(const int num) {
        if (num == 0) return 1;
        return static_cast<int>(log10(static_cast<double>(num))) + 1;
    }

    NodeTermIntLit* parse_number() {
        next_token(int_tokens, true);

        int result = 0;

        int multiplier = 1;
        int prev_result = INT_MAX;

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

            if (digit_count(prev_result) <= digit_count(curr_result)) {
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
            if (!next_token(arithmetic_tokens, false) && !next_token(boolean_tokens, false)) {
                break;
            }

            const TokenType opr = it->type;
            const int prec = get_prec(opr);

            if (prec < min_prec) {
                --it;
                break;
            }

            auto* const expr_rhs = parse_expr(prec + 1);

            auto* node_arith_expr = allocator.alloc<NodeBinExpr>();
            auto* const expr_lhs = allocator.alloc<NodeExpr>();

            expr_lhs->var = node_expr->var;

            node_arith_expr->left = expr_lhs;
            node_arith_expr->right = expr_rhs;
            node_arith_expr->type = opr;


            node_expr->var = node_arith_expr;
        }


        return node_expr;
    }

    [[nodiscard]] NodeTerm* parse_term() {
        auto* const term = allocator.alloc<NodeTerm>();

        switch (it->type) {
            case TokenType::sq_brkt_open: {
                NodeTermIntLit* int_lit = parse_number();
                term->var = int_lit;

                next_token({TokenType::sq_brkt_close}, true);
                break;
            }
            case TokenType::backtick: {
                next_token({TokenType::var_ident}, true);

                auto* term_ident = allocator.alloc<NodeTermIdent>();
                term_ident->ident = *it;
                term->var = term_ident;

                next_token({TokenType::backtick}, true);
                break;
            }
            case TokenType::paren_open: {
                auto* const expr = parse_expr();

                next_token({TokenType::paren_close}, true);
                auto* term_paren = allocator.alloc<NodeTermParen>();

                term_paren->expr = expr;
                term->var = term_paren;
                break;
            }
            case TokenType::bool_true:
            case TokenType::bool_false: {
                auto* bool_lit = allocator.alloc<NodeTermBoolLit>();
                bool_lit->value = it->type == TokenType::bool_true;

                term->var = bool_lit;
                break;
            }
            default: assert(false); //Unreachable
        }


        return term;
    }

    NodeStmtScope* parse_scope() {
        auto* scope = allocator.alloc<NodeStmtScope>();

        vector<NodeStatement *> statements;

        while (next_token(stmt_tokens, false)) {
            statements.push_back(parse_statement());
        }

        scope->statements = statements;
        return scope;
    }

    NodeIfPred* parse_if() {
        next_token({TokenType::paren_open}, true);

        NodeExpr* expr = parse_expr();

        next_token({TokenType::paren_close}, true);
        next_token({TokenType::colon}, true);

        next_token(stmt_tokens, true);
        NodeStatement* stmt = parse_statement();

        auto* if_pred = allocator.alloc<NodeIfPred>();
        if_pred->expr = expr;
        if_pred->stmt = stmt;

        return if_pred;
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
            next_token({TokenType::var_type_int, TokenType::var_type_boolean}, true);
            const Token var_type = *it;

            next_token({TokenType::backtick}, true);
            next_token({TokenType::var_ident}, true);
            const Token ident = *it;
            next_token({TokenType::backtick}, true);

            next_token({TokenType::var_assign}, true);

            NodeExpr* expr = parse_expr();

            auto* node_stmt_variable = allocator.alloc<NodeStmtVariable>();
            node_stmt_variable->ident = ident;
            node_stmt_variable->type = var_type.type;
            node_stmt_variable->expr = expr;

            node_statement->var = node_stmt_variable;
        }
        else if (it->type == TokenType::cur_brkt_open) {
            NodeStmtScope* scope = parse_scope();

            next_token({TokenType::cur_brkt_close}, true);

            node_statement->var = scope;
        }
        else if (it->type == TokenType::cond_if) {
            NodeIfPred* if_pred = parse_if();

            auto* stmt_if = allocator.alloc<NodeStmtIf>();
            stmt_if->pred = if_pred;

            while (next_token({TokenType::cond_else}, false)) {
                if (next_token({TokenType::cond_if}, false)) {
                    NodeIfPred* elif_pred = parse_if();
                    stmt_if->pred_elif.push_back(elif_pred);
                }
                else {
                    next_token({TokenType::colon}, true);

                    next_token(stmt_tokens, true);
                    NodeStatement* else_stmt = parse_statement();

                    auto* pred_else = allocator.alloc<NodeIfPredElse>();
                    pred_else->stmt = else_stmt;

                    stmt_if->pred_else = pred_else;
                }
            }

            node_statement->var = stmt_if;
        }
        else if (it->type == TokenType::backtick) {
            next_token({TokenType::var_ident}, true);
            const Token ident = *it;

            next_token({TokenType::backtick}, true);
            next_token({TokenType::var_assign}, true);

            NodeExpr* expr = parse_expr();

            auto* stmt_assign = allocator.alloc<NodeStmtAssign>();
            stmt_assign->ident = ident;
            stmt_assign->expr = expr;

            node_statement->var = stmt_assign;
        }
        else if (it->type == TokenType::loop) {
            auto* stmt_while = allocator.alloc<NodeStmtWhile>();

            if (next_token({TokenType::cond_if}, false)) {
                next_token({TokenType::paren_open}, true);

                NodeExpr* expr = parse_expr();
                stmt_while->expr = expr;

                next_token({TokenType::paren_close}, true);
            }
            next_token({TokenType::colon}, true);

            next_token(stmt_tokens, true);
            NodeStatement* stmt = parse_statement();
            stmt_while->stmt = stmt;

            node_statement->var = stmt_while;
        }
        else if (it->type == TokenType::loop_break) {
            node_statement->var = allocator.alloc<NodeStmtBreak>();
        }
        else if (it->type == TokenType::loop_continue) {
            node_statement->var = allocator.alloc<NodeStmtContinue>();
        }
        else if (it->type == TokenType::print) {
            next_token({TokenType::paren_open}, true);

            NodeExpr* expr = parse_expr();
            auto* stmt_print = allocator.alloc<NodeStmtPrint>();
            stmt_print->expr = expr;

            next_token({TokenType::paren_close}, true);

            node_statement->var = stmt_print;
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
        const auto next = it + 1;

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
