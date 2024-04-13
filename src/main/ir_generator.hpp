#pragma once

#include <map>
#include <stack>

#include "parser.hpp"


enum class OperationType {
    add, subtract, multiply, divide, modulo,
    is_equal, not_equal, is_greater, is_greater_equal, is_less, is_less_equal,
    log_and, log_or, log_not,
    assign, jump_false, jump, label,
    prog_exit, print_int, print_char, read_char,
    bgn_scope, end_scope, array_get, array_assign, array_allocate, array_free
};

struct TACInstruction {
    OperationType op;
    std::optional<std::string> result;
    std::optional<std::string> arg1;
    std::optional<std::string> arg2;
};

class IRGenerator {
public:
    explicit IRGenerator(NodeStart root) : root(std::move(root)) {
    }

    static TokenType get_result_type(const TokenType opr) {
        if (arithmetic_tokens.contains(opr)) return TokenType::var_type_int;
        if (boolean_tokens.contains(opr)) return TokenType::var_type_boolean;
        assert(false);
    }

    static TokenType get_param_type(const TokenType opr) {
        if (opr == TokenType::equal || opr == TokenType::not_equal) return TokenType::null;
        if (logical_tokens.contains(opr)) return TokenType::var_type_boolean;
        return TokenType::var_type_int;
    }

    static void check_token(const TokenType result_type, const TokenType expected_type, const int line) {
        if (expected_type == TokenType::null) return;
        if (result_type == expected_type)return;

        //TODO Only temporarily
        if (expected_type == TokenType::var_type_int && result_type == TokenType::var_type_char) return;
        if (expected_type == TokenType::var_type_char && result_type == TokenType::var_type_int) return;

        std::cerr << "[BŁĄD] [Analisa semantyczna] Niezgodność typu danych, oczekiwano `" +
                     get_token_names({expected_type})
                  << "' \n\t w linijce " << line << std::endl;
        exit(EXIT_FAILURE);
    }

    static void check_operator(const TokenType opr, const TokenType expected_type, const int line) {
        const TokenType result_type = get_result_type(opr);
        check_token(result_type, expected_type, line);
    }

    void check_ident(const Token &ident, const bool should_exist) const {
        const std::string &ident_str = ident.value.value();

        if (var_types.contains(ident_str)) {
            if (!should_exist) {
                std::cerr << "[BŁĄD] [Analiza semantyczna] Redeklaracja identyfikatora '" << ident_str <<
                          "' \n\t w linijce " << ident.line << std::endl;
                exit(EXIT_FAILURE);
            }
        } else {
            if (should_exist) {
                std::cerr << "[BŁĄD] [Analiza semantyczna] Nieznany identyfikator '" << ident_str << "' \n\t w linijce "
                          <<
                          ident.line << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    std::string generate_bin_expr(const NodeBinExpr *expr, const TokenType expected_type) {
        check_operator(expr->opr.type, expected_type, expr->opr.line);

        const TokenType expected_param_type = get_param_type(expr->opr.type);
        const std::string rhs = generate_expr(expr->right, expected_param_type);
        const std::string lhs = generate_expr(expr->left, expected_param_type);

        const std::string result = new_temp_var();

        instructions.push_back({
                                       token_operation_map[expr->opr.type],
                                       result,
                                       lhs,
                                       rhs
                               }
        );

        return result;
    }

    std::string generate_un_expr(const NodeUnExpr *un_expr, const TokenType expected_type) {
        check_operator(un_expr->opr.type, expected_type, un_expr->opr.line);

        const TokenType expected_param_type = get_param_type(un_expr->opr.type);
        const std::string term = generate_term(un_expr->term, expected_param_type);

        std::string result = new_temp_var();
        instructions.push_back({
                                       token_operation_map[un_expr->opr.type],
                                       result,
                                       term});

        return result;
    }

    std::string generate_term(NodeTerm *term, const TokenType expected_type) {
        struct TermVisitor {
            IRGenerator &gen;
            TokenType expected_type;

            std::string operator()(const NodeTermIntLit *term_int_lit) const {
                check_token(TokenType::var_type_int, expected_type, term_int_lit->int_lit.line);

                return term_int_lit->int_lit.value.value();
            }

            std::string operator()(const NodeTermBoolLit *term_bool_lit) const {
                check_token(TokenType::var_type_boolean, expected_type, term_bool_lit->bool_lit.line);

                return term_bool_lit->bool_lit.value.value();
            }

            std::string operator()(const NodeTermCharLit *term_char_lit) const {
                check_token(TokenType::var_type_char, expected_type, term_char_lit->char_lit.line);

                const int c = static_cast<unsigned char>(term_char_lit->char_lit.value.value()[0]);
                return std::to_string(c);
            }

            std::string operator()(const NodeTermStringLit *term_string_lit) const {
                check_token(TokenType::var_type_string, expected_type, term_string_lit->array_expr->token.line);
                assert(false);
                //TODO
                //gen.generate_array_expr(term_string_lit->array_expr, );
            }

            std::string operator()(const NodeTermIdent *term_ident) const {
                const std::string &ident = term_ident->ident.value.value();
                gen.check_ident(term_ident->ident, true);

                const TokenType type = gen.var_types[ident];
                check_token(type, expected_type, term_ident->ident.line);

                return term_ident->ident.value.value();
            }

            std::string operator()(const NodeTermParen *term_paren) const {
                return gen.generate_expr(term_paren->expr, expected_type);
            }

            std::string operator()(const NodeTermArrIdent *arr_ident) const {
                const std::string &ident = arr_ident->ident.value.value();
                gen.check_ident(arr_ident->ident, true);

                const TokenType type = gen.var_types[ident];
                check_token(type, expected_type, arr_ident->ident.line);

                std::string index = gen.generate_expr(arr_ident->index, TokenType::var_type_int);
                std::string result = gen.new_temp_var();

                gen.instructions.push_back({
                                                   OperationType::array_get,
                                                   result,
                                                   ident,
                                                   index}
                );

                return result;
            }


            std::string operator()(const NodeTermArray *array_expr) const {
                check_token(TokenType::array, expected_type, array_expr->token.line);
                assert(false);
                //TODO
                //gen.generate_array_expr(array_expr, *var);
            }

            std::string operator()(const NodeTermReadChar *read_char) const {
                check_token(TokenType::var_type_char, expected_type, read_char->token.line);

                std::string result = gen.new_temp_var();
                gen.instructions.push_back({
                                                   OperationType::read_char,
                                                   result}
                );

                return result;
            }
        };
        TermVisitor visitor{*this, expected_type};
        return visit(visitor, term->var);
    }


    std::string generate_expr(NodeExpr *expr, const TokenType expected_type) {
        struct ExprVisitor {
            IRGenerator &gen;
            TokenType expected_type;

            std::string operator()(const NodeBinExpr *arith_expr) const {
                return gen.generate_bin_expr(arith_expr, expected_type);
            }

            std::string operator()(NodeTerm *term) const {
                return gen.generate_term(term, expected_type);
            }

            std::string operator()(const NodeUnExpr *un_expr) const {
                return gen.generate_un_expr(un_expr, expected_type);
            }
        };

        ExprVisitor visitor = {*this, expected_type};
        return visit(visitor, expr->var);
    }

    void generate_if_pred(const NodeIfPred *pred_if, const std::string &false_label, const std::string &end_label) {
        std::string expr = generate_expr(pred_if->expr, TokenType::var_type_boolean);
        instructions.push_back({
                                       OperationType::jump_false,
                                       {},
                                       expr,
                                       false_label
                               });

        generate_statement(pred_if->stmt);

        generate_jump(end_label);
    }

    void generate_array_expr(const NodeTermArray *arr_expr, const std::string &ident, TokenType type) {
        for (int index = 0; index < arr_expr->exprs.size(); index++) {
            std::string expr = generate_expr(arr_expr->exprs.at(index), type);

            instructions.push_back({
                                           OperationType::array_assign,
                                           ident,
                                           std::to_string(index),
                                           expr}
            );
        }
    }

    void generate_exit(const std::optional<NodeStmtExit *> stmt_exit) {
        std::string expr = "0";
        if (stmt_exit.has_value()) {
            expr = generate_expr(stmt_exit.value()->expr, TokenType::var_type_int);
        }

        instructions.push_back({
                                       OperationType::prog_exit, {}, expr
                               });
    }

    void generate_label(const std::string &label_str) {
        instructions.push_back({
                                       OperationType::label,
                                       {},
                                       label_str
                               });
    }

    void generate_jump(const std::string &label) {
        instructions.push_back({
                                       OperationType::jump,
                                       {},
                                       label
                               });
    }

    std::string generate_assign(const std::string &ident, const std::string &value) {
        std::string result = new_temp_var();
        instructions.push_back({
                                       OperationType::assign,
                                       ident,
                                       value}
        );
        return result;
    }

    void generate_statement(NodeStatement *stmt) {
        struct StatementVisitor {
            IRGenerator &gen;

            void operator()(const NodeStmtVariable *stmt_var) const {
                const std::string *ident = &stmt_var->ident.value.value();
                gen.check_ident(stmt_var->ident, false);

                gen.var_types.try_emplace(*ident, stmt_var->type);
                gen.scopes.top().push_back(*ident);

                const std::string expr = gen.generate_expr(stmt_var->expr, stmt_var->type);
                gen.generate_assign(*ident, expr);
            }

            void operator()(NodeStmtExit *stmt_exit) const {
                gen.generate_exit(stmt_exit);
            }

            void operator()(const NodeStmtScope *stmt_scope) const {
                gen.gen_begin_scope();

                for (NodeStatement *stmt: stmt_scope->statements) {
                    gen.generate_statement(stmt);
                }

                gen.gen_end_scope();
            }

            void operator()(const NodeStmtIf *stmt_if) const {
                const std::string &end_label = gen.get_new_label();
                std::string false_label = gen.get_new_label();
                gen.generate_if_pred(stmt_if->pred, false_label, end_label);

                for (const NodeIfPred *pred_if: stmt_if->pred_elif) {
                    gen.generate_label(false_label);
                    false_label = gen.get_new_label();
                    gen.generate_if_pred(pred_if, false_label, end_label);
                }

                gen.generate_label(false_label);

                if (stmt_if->pred_else.has_value()) {
                    gen.generate_statement(stmt_if->pred_else.value()->stmt);
                }

                gen.generate_label(end_label);
            }

            void operator()(const NodeStmtAssign *stmt_assign) const {
                const std::string *ident = &stmt_assign->ident.value.value();
                gen.check_ident(stmt_assign->ident, true);

                const TokenType type = gen.var_types[*ident];
                const std::string expr = gen.generate_expr(stmt_assign->expr, type);

                gen.generate_assign(*ident, expr);
            }

            void operator()(const NodeStmtWhile *stmt_while) const {
                const std::string start_label = gen.get_new_label();
                const std::string end_label = gen.get_new_label();
                auto label_pair = std::pair{start_label, end_label};
                gen.loop_labels.emplace(label_pair);

                gen.generate_label(start_label);

                if (stmt_while->expr.has_value()) {
                    std::string expr = gen.generate_expr(stmt_while->expr.value(), TokenType::var_type_boolean);
                    gen.instructions.push_back({
                                                       OperationType::jump_false,
                                                       {},
                                                       expr,
                                                       end_label
                                               });
                }

                gen.generate_statement(stmt_while->stmt);
                gen.generate_jump(start_label);

                gen.generate_label(end_label);

                if (!gen.loop_labels.empty() && gen.loop_labels.top() == label_pair) {
                    gen.loop_labels.pop();
                }
            }

            void operator()(const NodeStmtBreak *stmt_break) const {
                if (gen.loop_labels.empty()) {
                    std::cerr
                            << "[BŁĄD] [Analiza semantyczna] Instrukcja 'przerwij' poza zakresem pętli \n\t w linijce "
                            <<
                            stmt_break->token.line << std::endl;
                    exit(EXIT_FAILURE);
                }

                const std::string &label = gen.loop_labels.top().second;
                gen.generate_jump(label);
                gen.loop_labels.pop();
            }

            void operator()(const NodeStmtContinue *stmt_continue) const {
                if (gen.loop_labels.empty()) {
                    std::cerr
                            << "[BŁĄD] [Analiza semantyczna] Instrukcja 'kontynuuj' poza zakresem pętli \n\t w linijce "
                            <<
                            stmt_continue->token.line << std::endl;
                    exit(EXIT_FAILURE);
                }

                const std::string &label = gen.loop_labels.top().first;
                gen.generate_jump(label);
            }

            void operator()(const NodeStmtPrintInt *stmt_print) const {
                std::string expr = gen.generate_expr(stmt_print->expr, TokenType::var_type_int);

                gen.instructions.push_back({
                                                   OperationType::print_int,
                                                   {},
                                                   expr
                                           });
            }

            void operator()(const NodeStmtPrintChar *stmt_print) const {
                std::string expr = gen.generate_expr(stmt_print->expr, TokenType::var_type_char);

                gen.instructions.push_back({
                                                   OperationType::print_char,
                                                   {},
                                                   expr
                                           });
            }

            void operator()(const NodeStmtArray *stmt_array) const {
                const std::string &ident = stmt_array->ident.value.value();
                gen.check_ident(stmt_array->ident, false);

                std::string size = gen.generate_expr(stmt_array->size, TokenType::var_type_int);

                gen.instructions.push_back({
                                                   OperationType::array_allocate,
                                                   ident,
                                                   size}
                );

                gen.var_types.try_emplace(ident, stmt_array->type);
                gen.scopes.top().push_back(ident);

                if (stmt_array->contents.has_value()) {
                    const NodeTermArray *array_expr = stmt_array->contents.value();

                    gen.generate_array_expr(array_expr, ident, stmt_array->type);
                }
            }

            void operator()(const NodeStmtArrAssign *stmt_arr_assign) const {
                const std::string &ident = stmt_arr_assign->ident.value.value();
                gen.check_ident(stmt_arr_assign->ident, true);

                const TokenType type = gen.var_types[ident];

                std::string expr = gen.generate_expr(stmt_arr_assign->expr, type);
                std::string index = gen.generate_expr(stmt_arr_assign->index, TokenType::var_type_int);
                gen.instructions.push_back({
                                                   OperationType::array_assign,
                                                   ident,
                                                   index,
                                                   expr}
                );
            }
        };
        StatementVisitor visitor{*this};
        visit(visitor, stmt->var);
    }

    [[nodiscard]] std::vector<TACInstruction> generate_program() {
        scopes.emplace();

        bool contains_exit = false;
        for (NodeStatement *stmt: root.statements) {
            if (holds_alternative<NodeStmtExit *>(stmt->var)) {
                contains_exit = true;
            }

            generate_statement(stmt);
        }

        if (!contains_exit) {
            generate_exit({});
        }

        return instructions;
    }

    std::string ir_to_string() {
        std::stringstream out;

        out << "_start" << std::endl;

        for (TACInstruction &instr: instructions) {
            if (instr.op == OperationType::label) {
                out << instr.arg1.value() << ":";
            } else {
                out << "    ";
                if (instr.result.has_value()) {
                    out << instr.result.value() << " = ";
                }
                if (operation_strings.contains(instr.op)) {
                    out << operation_strings[instr.op] << " ";
                }
                if (instr.arg1.has_value()) {
                    out << instr.arg1.value();
                }
                if (instr.arg2.has_value()) {
                    out << ", " << instr.arg2.value();
                }
            }

            out << std::endl;
        }

        return out.str();
    }

private:
    NodeStart root;
    std::vector<TACInstruction> instructions;

    std::map<std::string, TokenType> var_types;
    std::stack<std::vector<std::string>> scopes;

    int label_counter = 0;
    std::stack<std::pair<std::string, std::string>> loop_labels;

    int temp_var_counter = 0;

    std::map<TokenType, OperationType> token_operation_map = {
            {TokenType::add,           OperationType::add},
            {TokenType::subtract,      OperationType::subtract},
            {TokenType::multiply,      OperationType::multiply},
            {TokenType::divide,        OperationType::divide},
            {TokenType::modulo,        OperationType::modulo},
            {TokenType::equal,         OperationType::is_equal},
            {TokenType::not_equal,     OperationType::not_equal},
            {TokenType::greater,       OperationType::is_greater},
            {TokenType::less,          OperationType::is_less},
            {TokenType::greater_equal, OperationType::is_greater_equal},
            {TokenType::less_equal,    OperationType::is_less_equal},
            {TokenType::logical_and,   OperationType::log_and},
            {TokenType::logical_or,    OperationType::log_or},
            {TokenType::logical_not,   OperationType::log_not},
    };

    std::map<OperationType, std::string> operation_strings = {
            {OperationType::add,              "add"},
            {OperationType::subtract,         "sub"},
            {OperationType::multiply,         "mul"},
            {OperationType::divide,           "div"},
            {OperationType::modulo,           "mod"},
            {OperationType::is_equal,         "eq"},
            {OperationType::not_equal,        "ne"},
            {OperationType::is_greater,       "gt"},
            {OperationType::is_greater_equal, "ge"},
            {OperationType::is_less,          "lt"},
            {OperationType::is_less_equal,    "le"},
            {OperationType::log_and,          "and"},
            {OperationType::log_or,           "or"},
            {OperationType::log_not,          "not"},
            {OperationType::jump_false,       "jmp_false"},
            {OperationType::jump,             "jmp"},
            {OperationType::prog_exit,        "exit"},
            {OperationType::print_int,        "print_int"},
            {OperationType::print_char,       "print_char"},
            {OperationType::read_char,        "read_char"},
            {OperationType::bgn_scope,        "begin_scope"},
            {OperationType::end_scope,        "end_scope"},
            {OperationType::array_allocate,   "alloc"},
            {OperationType::array_free,       "free"},
            {OperationType::array_assign,     "offset_set"},
            {OperationType::array_get,        "offset_get"},
    };

    std::string new_temp_var() {
        return "#" + std::to_string(temp_var_counter++);
    }

    std::string get_new_label() {
        return "label_" + std::to_string(label_counter++);
    }

    void gen_begin_scope() {
        scopes.emplace();
        instructions.push_back({OperationType::bgn_scope});
    }

    void gen_end_scope() {
        for (std::string const &var: scopes.top()) {
            if (var_types[var] == TokenType::array) {

            }
            var_types.erase(var);

        }
        scopes.pop();
        instructions.push_back({OperationType::end_scope});
    }
};
