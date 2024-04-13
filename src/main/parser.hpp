#pragma once

#include <set>
#include <variant>
#include <cmath>

#include "arena_allocator.hpp"
#include "tokenizer.hpp"

struct NodeExpr;
struct NodeStatement;


struct NodeTermArray {
    Token token;
    std::vector<NodeExpr *> exprs;
};

struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermBoolLit {
    Token bool_lit;
};

struct NodeTermCharLit {
    Token char_lit;
};

struct NodeTermStringLit {
    NodeTermArray *array_expr{};
};

struct NodeTermIdent {
    Token ident;
};

struct NodeTermParen {
    NodeExpr *expr;
};

struct NodeTermArrIdent {
    Token ident;
    NodeExpr *index{};
};

struct NodeTermReadChar {
    Token token;
};

struct NodeTerm {
    std::variant<NodeTermCharLit *, NodeTermBoolLit *, NodeTermIntLit *, NodeTermIdent *, NodeTermParen *, NodeTermArrIdent
    *, NodeTermStringLit *, NodeTermArray *, NodeTermReadChar *> var;
};

struct NodeBinExpr {
    Token opr;
    NodeExpr *left{};
    NodeExpr *right{};
};

struct NodeUnExpr {
    Token opr;
    NodeTerm *term{};
};

struct NodeExpr {
    std::variant<NodeTerm *, NodeBinExpr *, NodeUnExpr *> var;
};

struct NodeStmtExit {
    NodeExpr *expr;
};

struct NodeStmtVariable {
    Token ident;
    TokenType type{};
    NodeExpr *expr{};
};

struct NodeStmtArray {
    Token ident;
    TokenType type{};
    NodeExpr *size{};
    std::optional<NodeTermArray *> contents;
};

struct NodeStmtScope {
    std::vector<NodeStatement *> statements;
};

struct NodeIfPred {
    NodeExpr *expr;
    NodeStatement *stmt;
};

struct NodeIfPredElse {
    NodeStatement *stmt;
};

struct NodeStmtIf {
    NodeIfPred *pred;
    std::optional<NodeIfPredElse *> pred_else;
    std::vector<NodeIfPred *> pred_elif;
};

struct NodeStmtWhile {
    std::optional<NodeExpr *> expr;
    NodeStatement *stmt{};
};

struct NodeStmtBreak {
    Token token;
};

struct NodeStmtContinue {
    Token token;
};

struct NodeStmtAssign {
    Token ident;
    NodeExpr *expr{};
};

struct NodeStmtArrAssign {
    Token ident;
    NodeExpr *index{};
    NodeExpr *expr{};
};

struct NodeStmtPrintInt {
    NodeExpr *expr;
};

struct NodeStmtPrintChar {
    NodeExpr *expr;
};

struct NodeStatement {
    std::variant<NodeStmtExit *, NodeStmtVariable *, NodeStmtScope *, NodeStmtIf *, NodeStmtAssign *, NodeStmtWhile *,
            NodeStmtBreak *, NodeStmtContinue *, NodeStmtPrintInt *, NodeStmtPrintChar *, NodeStmtArray *, NodeStmtArrAssign
            *> var;
};

struct NodeStart {
    std::vector<NodeStatement *> statements;
};


class Parser {
public:
    explicit Parser(const std::vector<Token> &tokens) : tokens(tokens), allocator((1024 * 1024 * 4)) {
    }


    static int digit_count(const int num) {
        if (num == 0) return 1;
        return static_cast<int>(log10(static_cast<double>(num))) + 1;
    }

    NodeTermIntLit *parse_number() {
        const bool minus = next_token({TokenType::minus}, false);
        next_token(int_tokens, true);

        std::stack<Token *> num_tokens;
        num_tokens.push(to_address(it));
        while (next_token(int_tokens, false)) {
            num_tokens.push(to_address(it));
        }

        int result = 0;

        int multiplier = 1;
        int prev_result = -1;

        for (; !num_tokens.empty(); num_tokens.pop()) {
            const Token *curr = num_tokens.top();

            if (curr->type == TokenType::int_lit_mul) {
                const int new_multiplier = multipliers.at(curr->value.value());

                if (new_multiplier <= multiplier) {
                    number_err();
                }

                multiplier = new_multiplier;
                continue;
            }

            const int value = num_values.at(curr->value.value());
            const int curr_result = multiplier * value;

            if (digit_count(prev_result) >= digit_count(curr_result)) {
                number_err();
            }

            prev_result = curr_result;
            result += prev_result;
        }

        if (minus) {
            result = -result;
        }

        auto *term_int_lit = allocator.alloc<NodeTermIntLit>();
        term_int_lit->int_lit = Token{TokenType::int_lit_num, std::to_string(result), it->line};

        return term_int_lit;
    }

    void number_err() const {
        std::cerr << "[BŁĄD] [Analiza składniowa] Nieprawidłowa liczba \n\t w linijce " << it->line << std::endl;
        exit(EXIT_FAILURE);
    }

    [[nodiscard]] NodeExpr *parse_expr(const int min_prec = 0) {
        auto *node_expr = allocator.alloc<NodeExpr>();

        if (next_token({TokenType::logical_not}, false)) {
            auto *node_un_expr = allocator.alloc<NodeUnExpr>();
            node_un_expr->opr = *it;

            next_token(term_tokens, true);
            NodeTerm *node_term = parse_term();
            node_un_expr->term = node_term;

            node_expr->var = node_un_expr;
        } else {
            next_token(term_tokens, true);

            NodeTerm *node_term = parse_term();
            node_expr->var = node_term;
        }

        while (true) {
            if (!next_token(arithmetic_tokens, false) && !next_token(boolean_tokens, false)) {
                break;
            }

            const Token opr = *it;
            const int prec = get_prec(opr.type);

            if (prec < min_prec) {
                --it;
                break;
            }

            auto *const expr_rhs = parse_expr(prec + 1);

            auto *node_arith_expr = allocator.alloc<NodeBinExpr>();
            auto *const expr_lhs = allocator.alloc<NodeExpr>();

            expr_lhs->var = node_expr->var;

            node_arith_expr->left = expr_lhs;
            node_arith_expr->right = expr_rhs;
            node_arith_expr->opr = opr;


            node_expr->var = node_arith_expr;
        }


        return node_expr;
    }

    [[nodiscard]] NodeTerm *parse_term() {
        auto *const term = allocator.alloc<NodeTerm>();

        switch (it->type) {
            case TokenType::sq_brkt_open: {
                NodeTermIntLit *int_lit = parse_number();
                term->var = int_lit;

                next_token({TokenType::sq_brkt_close}, true);
                break;
            }
            case TokenType::backtick: {
                next_token({TokenType::var_ident}, true);
                const Token ident = *it;
                next_token({TokenType::backtick}, true);

                if (next_token({TokenType::element}, false)) {
                    NodeExpr *index = parse_expr();
                    auto *term_arr_ident = allocator.alloc<NodeTermArrIdent>();
                    term_arr_ident->ident = ident;
                    term_arr_ident->index = index;
                    term->var = term_arr_ident;
                } else {
                    auto *term_ident = allocator.alloc<NodeTermIdent>();
                    term_ident->ident = ident;
                    term->var = term_ident;
                }

                break;
            }
            case TokenType::paren_open: {
                auto *const expr = parse_expr();

                next_token({TokenType::paren_close}, true);
                auto *term_paren = allocator.alloc<NodeTermParen>();

                term_paren->expr = expr;
                term->var = term_paren;
                break;
            }
            case TokenType::bool_lit: {
                auto *term_bool_lit = allocator.alloc<NodeTermBoolLit>();
                term_bool_lit->bool_lit = *it;

                term->var = term_bool_lit;
                break;
            }
            case TokenType::single_quote: {
                next_token({TokenType::character}, true);

                auto *term_char_lit = allocator.alloc<NodeTermCharLit>();
                term_char_lit->char_lit = *it;

                next_token({TokenType::single_quote}, true);

                term->var = term_char_lit;
                break;
            }
            case TokenType::double_quote: {
                next_token({TokenType::string_lit}, true);

                std::string string_lit = it->value.value();

                auto *term_string_lit = allocator.alloc<NodeTermStringLit>();
                auto *array_expr = allocator.alloc<NodeTermArray>();

                array_expr->token = *it;
                for (const char c: string_lit) {
                    auto *expr_char = allocator.alloc<NodeExpr>();
                    auto *term_char = allocator.alloc<NodeTerm>();
                    auto *term_char_lit = allocator.alloc<NodeTermCharLit>();

                    term_char_lit->char_lit = Token{TokenType::character, std::to_string(c), it->line};
                    term_char->var = term_char_lit;
                    expr_char->var = term_char;
                    array_expr->exprs.push_back(expr_char);
                }

                term_string_lit->array_expr = array_expr;

                next_token({TokenType::double_quote}, true);

                term->var = term_string_lit;
                break;
            }
            case TokenType::read_char: {
                next_token({TokenType::paren_open}, true);
                next_token({TokenType::paren_close}, true);

                auto *term_read_char = allocator.alloc<NodeTermReadChar>();
                term_read_char->token = *it;

                term->var = term_read_char;
                break;
            }
            default:
                assert(false); //Unreachable
        }


        return term;
    }

    NodeTermArray *parse_array_expr() {
        next_token({TokenType::cur_brkt_open}, true);

        auto *array_expr = allocator.alloc<NodeTermArray>();
        array_expr->token = *it;

        do {
            NodeExpr *expr = parse_expr();
            array_expr->exprs.push_back(expr);
        } while (next_token({TokenType::comma}, false));


        next_token({TokenType::cur_brkt_close}, false);

        return array_expr;
    }

    NodeStmtScope *parse_scope() {
        auto *scope = allocator.alloc<NodeStmtScope>();

        std::vector<NodeStatement *> statements;

        while (next_token(stmt_tokens, false)) {
            statements.push_back(parse_statement());
        }

        scope->statements = statements;
        return scope;
    }

    NodeIfPred *parse_if() {
        next_token({TokenType::paren_open}, true);

        NodeExpr *expr = parse_expr();

        next_token({TokenType::paren_close}, true);
        next_token({TokenType::colon}, true);

        next_token(stmt_tokens, true);
        NodeStatement *stmt = parse_statement();

        auto *if_pred = allocator.alloc<NodeIfPred>();
        if_pred->expr = expr;
        if_pred->stmt = stmt;

        return if_pred;
    }

    NodeStatement *parse_statement() {
        auto *node_statement = allocator.alloc<NodeStatement>();

        switch (it->type) {
            case TokenType::exit: {
                next_token({TokenType::paren_open}, true);

                NodeExpr *expr = parse_expr();

                next_token({TokenType::paren_close}, true);

                auto *node_stmt_exit = allocator.alloc<NodeStmtExit>();
                node_stmt_exit->expr = expr;

                node_statement->var = node_stmt_exit;
                break;
            }
            case TokenType::var_decl: {
                next_token(var_types, true);
                const Token var_type = *it;

                next_token({TokenType::backtick}, true);
                next_token({TokenType::var_ident}, true);
                const Token ident = *it;
                next_token({TokenType::backtick}, true);

                next_token({TokenType::var_assign}, true);

                NodeExpr *expr = parse_expr();

                auto *node_stmt_variable = allocator.alloc<NodeStmtVariable>();
                node_stmt_variable->ident = ident;
                node_stmt_variable->type = var_type.type;
                node_stmt_variable->expr = expr;

                node_statement->var = node_stmt_variable;
                break;
            }
            case TokenType::cur_brkt_open: {
                NodeStmtScope *scope = parse_scope();

                next_token({TokenType::cur_brkt_close}, true);

                node_statement->var = scope;
                break;
            }
            case TokenType::cond_if: {
                NodeIfPred *if_pred = parse_if();

                auto *stmt_if = allocator.alloc<NodeStmtIf>();
                stmt_if->pred = if_pred;

                while (next_token({TokenType::cond_else}, false)) {
                    if (next_token({TokenType::cond_if}, false)) {
                        NodeIfPred *elif_pred = parse_if();
                        stmt_if->pred_elif.push_back(elif_pred);
                    } else {
                        next_token({TokenType::colon}, true);

                        next_token(stmt_tokens, true);
                        NodeStatement *else_stmt = parse_statement();

                        auto *pred_else = allocator.alloc<NodeIfPredElse>();
                        pred_else->stmt = else_stmt;

                        stmt_if->pred_else = pred_else;
                    }
                }

                node_statement->var = stmt_if;
                break;
            }
            case TokenType::backtick: {
                next_token({TokenType::var_ident}, true);
                const Token ident = *it;

                next_token({TokenType::backtick}, true);
                next_token({TokenType::var_assign, TokenType::element}, true);

                if (it->type == TokenType::var_assign) {
                    NodeExpr *expr = parse_expr();

                    auto *stmt_assign = allocator.alloc<NodeStmtAssign>();
                    stmt_assign->ident = ident;
                    stmt_assign->expr = expr;

                    node_statement->var = stmt_assign;
                } else {
                    NodeExpr *index = parse_expr();
                    next_token({TokenType::var_assign}, true);

                    NodeExpr *value = parse_expr();

                    auto *stmt_arr_assign = allocator.alloc<NodeStmtArrAssign>();
                    stmt_arr_assign->ident = ident;
                    stmt_arr_assign->index = index;
                    stmt_arr_assign->expr = value;

                    node_statement->var = stmt_arr_assign;
                }
                break;
            }
            case TokenType::loop: {
                auto *stmt_while = allocator.alloc<NodeStmtWhile>();

                if (next_token({TokenType::cond_if}, false)) {
                    next_token({TokenType::paren_open}, true);

                    NodeExpr *expr = parse_expr();
                    stmt_while->expr = expr;

                    next_token({TokenType::paren_close}, true);
                }
                next_token({TokenType::colon}, true);

                next_token(stmt_tokens, true);
                NodeStatement *stmt = parse_statement();
                stmt_while->stmt = stmt;

                node_statement->var = stmt_while;
                break;
            }
            case TokenType::loop_break: {
                auto *stmt_break = allocator.alloc<NodeStmtBreak>();
                stmt_break->token = *it;
                node_statement->var = stmt_break;
                break;
            }
            case TokenType::loop_continue: {
                auto *stmt_continue = allocator.alloc<NodeStmtContinue>();
                stmt_continue->token = *it;
                node_statement->var = stmt_continue;
                break;
            }
            case TokenType::print_int: {
                next_token({TokenType::paren_open}, true);

                NodeExpr *expr = parse_expr();
                auto *stmt_print = allocator.alloc<NodeStmtPrintInt>();
                stmt_print->expr = expr;

                next_token({TokenType::paren_close}, true);

                node_statement->var = stmt_print;
                break;
            }
            case TokenType::print_char: {
                next_token({TokenType::paren_open}, true);

                NodeExpr *expr = parse_expr();
                auto *stmt_print = allocator.alloc<NodeStmtPrintChar>();
                stmt_print->expr = expr;

                next_token({TokenType::paren_close}, true);

                node_statement->var = stmt_print;
                break;
            }
            case TokenType::array: {
                next_token(var_types, true);
                const Token var_type = *it;

                next_token({TokenType::backtick}, true);
                next_token({TokenType::var_ident}, true);
                const Token ident = *it;
                next_token({TokenType::backtick}, true);

                next_token({TokenType::var_assign, TokenType::of_size}, true);

                auto *node_stmt_arr = allocator.alloc<NodeStmtArray>();
                node_stmt_arr->ident = ident;
                node_stmt_arr->type = var_type.type;

                if (it->type == TokenType::of_size) {
                    NodeExpr *expr = parse_expr();
                    node_stmt_arr->size = expr;
                } else {
                    auto *array_expr = parse_array_expr();

                    auto *size_expr = allocator.alloc<NodeExpr>();
                    auto *size_term = allocator.alloc<NodeTerm>();
                    auto *size_int_lit = allocator.alloc<NodeTermIntLit>();
                    size_int_lit->int_lit = Token{TokenType::int_lit_num, std::to_string(array_expr->exprs.size())};
                    size_term->var = size_int_lit;
                    size_expr->var = size_term;

                    node_stmt_arr->size = size_expr;
                    node_stmt_arr->contents = array_expr;
                }


                node_statement->var = node_stmt_arr;
                break;
            }
            default: {
                std::cerr << "[BŁĄD] [Analiza składniowa] Oczekiwano <instrukcja>, znaleziono '" <<
                          token_names.at(it->type) << "' \n\t w linijce: " << it->line << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        return node_statement;
    }

    std::optional<NodeStart> parse_program() {
        NodeStart start_node;

        for (; it != tokens.end(); ++it) {
            start_node.statements.push_back(parse_statement());
        }

        return start_node;
    }

private:
    std::vector<Token> tokens;
    std::vector<Token>::iterator it = tokens.begin();
    ArenaAllocator allocator;

    bool next_token(const std::set<TokenType> &expected, const bool required) {
        const auto next = it + 1;

        if (next == tokens.end()) {
            if (!required) return false;

            std::cerr << "[BŁĄD] [Analiza składniowa] Oczekiwano '" << get_token_names(expected) <<
                      "', nie znaleziono kolejnych tokenów \n\t w linijce: " << it->line << std::endl;
            exit(EXIT_FAILURE);
        }

        if (!expected.contains(next->type)) {
            if (!required) return false;

            std::cerr << "[BŁĄD] [Analiza składniowa] Oczekiwano '" << get_token_names(expected) << "', znaleziono '" <<
                      token_names.at(next->type) << "' \n\t w linijce: " << it->line << std::endl;
            exit(EXIT_FAILURE);
        }

        ++it;
        return true;
    }
};
