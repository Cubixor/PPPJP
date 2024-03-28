#pragma once

#include <map>
#include <stack>

#include "parser.hpp"

using namespace std;


const string RAX = "rax";
const string RBX = "rbx";
const string RCX = "rcx";
const string RDX = "rdx";
const string RDI = "rdi";
const string RSI = "rsi";

enum OperationType {
    add, substract, multiply, divide, modulo,
    is_equal, not_equal, is_greater, is_greater_equal, is_less, is_less_equal,
    log_and, log_or, log_not,
    assign, jump_false, jump, label,
    prog_exit, print_int, print_char, read_char,
    bgn_scope, end_scope, array_get, array_assign, array_allocate
};

struct TACInstruction {
    OperationType op{};
    optional<string> result;
    optional<string> arg1;
    optional<string> arg2;
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

        cerr << "[BŁĄD] [Analisa semantyczna] Niezgodność typu danych, oczekiwano `" + get_token_names({
            expected_type
        }) << "' \n\t w linijce " << line << endl;
        exit(EXIT_FAILURE);
    }

    static void check_operator(const TokenType opr, const TokenType expected_type, const int line) {
        const TokenType result_type = get_result_type(opr);
        check_token(result_type, expected_type, line);
    }

    void check_ident(const Token&ident, const bool should_exist) const {
        const string&ident_str = ident.value.value();

        if (var_types.contains(ident_str)) {
            if (!should_exist) {
                cerr << "[BŁĄD] [Analiza semantyczna] Redeklaracja identyfikatora '" << ident_str <<
                        "' \n\t w linijce " << ident.line << endl;
                exit(EXIT_FAILURE);
            }
        }
        else {
            if (should_exist) {
                cerr << "[BŁĄD] [Analiza semantyczna] Nieznany identyfikator '" << ident_str << "' \n\t w linijce " <<
                        ident.line << endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    string generate_bin_expr(const NodeBinExpr* expr, const TokenType expected_type) {
        check_operator(expr->opr.type, expected_type, expr->opr.line);

        const TokenType expected_param_type = get_param_type(expr->opr.type);
        const string rhs = generate_expr(expr->right, expected_param_type);
        const string lhs = generate_expr(expr->left, expected_param_type);

        const string result = new_temp_var();

        instructions.push_back({
            token_operation_map[expr->opr.type],
            result,
            lhs,
            rhs,
        });

        return result;
    }

    string generate_un_expr(const NodeUnExpr* un_expr, const TokenType expected_type) {
        check_operator(un_expr->opr.type, expected_type, un_expr->opr.line);

        const TokenType expected_param_type = get_param_type(un_expr->opr.type);
        const string term = generate_term(un_expr->term, expected_param_type);

        string result = new_temp_var();
        instructions.push_back({
            token_operation_map[un_expr->opr.type],
            result,
            term
        });

        return result;
    }

    string generate_term(NodeTerm* term, const TokenType expected_type) {
        struct TermVisitor {
            IRGenerator&gen;
            TokenType expected_type;

            string operator()(const NodeTermIntLit* term_int_lit) const {
                check_token(TokenType::var_type_int, expected_type, term_int_lit->int_lit.line);

                return term_int_lit->int_lit.value.value();
            }

            string operator()(const NodeTermBoolLit* term_bool_lit) const {
                check_token(TokenType::var_type_boolean, expected_type, term_bool_lit->bool_lit.line);

                return term_bool_lit->bool_lit.value.value();
            }

            string operator()(const NodeTermCharLit* term_char_lit) const {
                check_token(TokenType::var_type_char, expected_type, term_char_lit->char_lit.line);

                const int c = static_cast<unsigned char>(term_char_lit->char_lit.value.value()[0]);
                return to_string(c);
            }

            string operator()(const NodeTermStringLit* term_string_lit) const {
                check_token(TokenType::var_type_string, expected_type, term_string_lit->array_expr->token.line);
                assert(false);
                //TODO
                //gen.generate_array_expr(term_string_lit->array_expr, );
            }

            string operator()(const NodeTermIdent* term_ident) const {
                const string&ident = term_ident->ident.value.value();
                gen.check_ident(term_ident->ident, true);

                const TokenType type = gen.var_types[ident];
                check_token(type, expected_type, term_ident->ident.line);

                return term_ident->ident.value.value();
            }

            string operator()(const NodeTermParen* term_paren) const {
                return gen.generate_expr(term_paren->expr, expected_type);
            }

            string operator()(const NodeTermArrIdent* arr_ident) const {
                const string&ident = arr_ident->ident.value.value();
                gen.check_ident(arr_ident->ident, true);

                const TokenType type = gen.var_types[ident];
                check_token(type, expected_type, arr_ident->ident.line);

                string index = gen.generate_expr(arr_ident->index, TokenType::var_type_int);
                string result = gen.new_temp_var();

                gen.instructions.push_back({
                    array_get,
                    result,
                    ident,
                    index,
                });

                return result;
            }


            string operator()(const NodeTermArray* array_expr) const {
                check_token(TokenType::array, expected_type, array_expr->token.line);
                assert(false);
                //TODO
                //gen.generate_array_expr(array_expr, *var);
            }

            string operator()(const NodeTermReadChar* read_char) const {
                check_token(TokenType::var_type_char, expected_type, read_char->token.line);

                string result = gen.new_temp_var();
                gen.instructions.push_back({
                    OperationType::read_char,
                    result
                });

                return result;
            }
        };
        TermVisitor visitor{*this, expected_type};
        return visit(visitor, term->var);
    }


    string generate_expr(NodeExpr* expr, const TokenType expected_type) {
        struct ExprVisitor {
            IRGenerator&gen;
            TokenType expected_type;

            string operator()(const NodeBinExpr* arith_expr) const {
                return gen.generate_bin_expr(arith_expr, expected_type);
            }

            string operator()(NodeTerm* term) const {
                return gen.generate_term(term, expected_type);
            }

            string operator()(const NodeUnExpr* un_expr) const {
                return gen.generate_un_expr(un_expr, expected_type);
            }
        };

        ExprVisitor visitor = {*this, expected_type};
        return visit(visitor, expr->var);
    }

    void generate_if_pred(const NodeIfPred* pred_if, const string&false_label, const string&end_label) {
        string expr = generate_expr(pred_if->expr, TokenType::var_type_boolean);
        instructions.push_back({
            jump_false,
            {},
            expr,
            false_label
        });

        generate_statement(pred_if->stmt);

        generate_jump(end_label);
    }

    void generate_array_expr(const NodeTermArray* arr_expr, const string&ident, TokenType type) {
        for (int index = 0; index < arr_expr->exprs.size(); index++) {
            string expr = generate_expr(arr_expr->exprs.at(index), type);

            instructions.push_back({
                array_assign,
                ident,
                to_string(index),
                expr
            });
        }
    }

    void generate_exit(const optional<NodeStmtExit *> stmt_exit) {
        string expr = "0";
        if (stmt_exit.has_value()) {
            expr = generate_expr(stmt_exit.value()->expr, TokenType::var_type_int);
        }

        instructions.push_back({
            prog_exit,
            {},
            expr
        });
    }

    void generate_label(const string&label_str) {
        instructions.push_back({
            label,
            {},
            label_str
        });
    }

    void generate_jump(const string&label) {
        instructions.push_back({
            jump,
            {},
            label
        });
    }

    string generate_assign(const string&ident, const string&value) {
        string result = new_temp_var();
        instructions.push_back({
            assign,
            ident,
            value
        });
        return result;
    }

    void generate_statement(NodeStatement* stmt) {
        struct StatementVisitor {
            IRGenerator&gen;

            void operator()(const NodeStmtVariable* stmt_var) const {
                const string* ident = &stmt_var->ident.value.value();
                gen.check_ident(stmt_var->ident, false);

                gen.var_types.insert({*ident, stmt_var->type});
                gen.scopes.top().push_back(*ident);

                const string expr = gen.generate_expr(stmt_var->expr, stmt_var->type);
                gen.generate_assign(*ident, expr);
            }

            void operator()(NodeStmtExit* stmt_exit) const {
                gen.generate_exit(stmt_exit);
            }

            void operator()(const NodeStmtScope* stmt_scope) const {
                gen.gen_begin_scope();

                for (NodeStatement* stmt: stmt_scope->statements) {
                    gen.generate_statement(stmt);
                }

                gen.gen_end_scope();
            }

            void operator()(const NodeStmtIf* stmt_if) const {
                const string&end_label = gen.get_new_label();
                string false_label = gen.get_new_label();
                gen.generate_if_pred(stmt_if->pred, false_label, end_label);

                for (const NodeIfPred* pred_if: stmt_if->pred_elif) {
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

            void operator()(const NodeStmtAssign* stmt_assign) const {
                const string* ident = &stmt_assign->ident.value.value();
                gen.check_ident(stmt_assign->ident, true);

                const TokenType type = gen.var_types[*ident];
                const string expr = gen.generate_expr(stmt_assign->expr, type);

                gen.generate_assign(*ident, expr);
            }

            void operator()(const NodeStmtWhile* stmt_while) const {
                const string start_label = gen.get_new_label();
                const string end_label = gen.get_new_label();
                auto label_pair = pair{start_label, end_label};
                gen.loop_labels.emplace(label_pair);

                gen.generate_label(start_label);

                if (stmt_while->expr.has_value()) {
                    string expr = gen.generate_expr(stmt_while->expr.value(), TokenType::var_type_boolean);
                    gen.instructions.push_back({
                        jump_false,
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

            void operator()(const NodeStmtBreak* stmt_break) const {
                if (gen.loop_labels.empty()) {
                    cerr << "[BŁĄD] [Analiza semantyczna] Instrukcja 'przerwij' poza zakresem pętli \n\t w linijce " <<
                            stmt_break->token.line << endl;
                    exit(EXIT_FAILURE);
                }

                const string&label = gen.loop_labels.top().second;
                gen.generate_jump(label);
                gen.loop_labels.pop();
            }

            void operator()(const NodeStmtContinue* stmt_continue) const {
                if (gen.loop_labels.empty()) {
                    cerr << "[BŁĄD] [Analiza semantyczna] Instrukcja 'kontynuuj' poza zakresem pętli \n\t w linijce " <<
                            stmt_continue->token.line << endl;
                    exit(EXIT_FAILURE);
                }

                const string&label = gen.loop_labels.top().first;
                gen.generate_jump(label);
            }

            void operator()(const NodeStmtPrintInt* stmt_print) const {
                string expr = gen.generate_expr(stmt_print->expr, TokenType::var_type_int);

                gen.instructions.push_back({
                    print_int,
                    {},
                    expr
                });
            }

            void operator()(const NodeStmtPrintChar* stmt_print) const {
                string expr = gen.generate_expr(stmt_print->expr, TokenType::var_type_char);

                gen.instructions.push_back({
                    print_char,
                    {},
                    expr
                });
            }

            void operator()(const NodeStmtArray* stmt_array) const {
                const string&ident = stmt_array->ident.value.value();
                gen.check_ident(stmt_array->ident, false);

                string size = gen.generate_expr(stmt_array->size, TokenType::var_type_int);

                gen.instructions.push_back({
                    array_allocate,
                    ident,
                    size
                });

                gen.var_types.insert({ident, stmt_array->type});
                gen.scopes.top().push_back(ident);

                if (stmt_array->contents.has_value()) {
                    const NodeTermArray* array_expr = stmt_array->contents.value();

                    gen.generate_array_expr(array_expr, ident, stmt_array->type);
                }
            }

            void operator()(const NodeStmtArrAssign* stmt_arr_assign) const {
                const string&ident = stmt_arr_assign->ident.value.value();
                gen.check_ident(stmt_arr_assign->ident, true);

                const TokenType type = gen.var_types[ident];

                string expr = gen.generate_expr(stmt_arr_assign->expr, type);
                string index = gen.generate_expr(stmt_arr_assign->index, TokenType::var_type_int);
                gen.instructions.push_back({
                    array_assign,
                    ident,
                    index,
                    expr
                });
            }
        };
        StatementVisitor visitor{*this};
        visit(visitor, stmt->var);
    }

    [[nodiscard]] vector<TACInstruction> generate_program() {
        scopes.emplace();

        bool contains_exit = false;
        for (NodeStatement* stmt: root.statements) {
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

    string ir_to_string() {
        stringstream out;

        out << "_start" << endl;

        for (TACInstruction&instr: instructions) {
            if (instr.op == label) {
                out << instr.arg1.value() << ":";
            }
            else {
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

            out << endl;
        }

        return out.str();
    }

private:
    NodeStart root;
    vector<TACInstruction> instructions;

    map<string, TokenType> var_types;
    stack<vector<string>> scopes;

    int label_counter = 0;
    stack<pair<string, string>> loop_labels;

    int temp_var_counter = 0;

    map<TokenType, OperationType> token_operation_map = {
        {TokenType::add, add},
        {TokenType::substract, substract},
        {TokenType::multiply, multiply},
        {TokenType::divide, divide},
        {TokenType::modulo, modulo},
        {TokenType::equal, is_equal},
        {TokenType::not_equal, not_equal},
        {TokenType::greater, is_greater},
        {TokenType::less, is_less},
        {TokenType::greater_equal, is_greater_equal},
        {TokenType::less_equal, is_less_equal},
        {TokenType::logical_and, log_and},
        {TokenType::logical_or, log_or},
        {TokenType::logical_not, log_not},
    };

    map<OperationType, string> operation_strings = {
        {add, "add"},
        {substract, "sub"},
        {multiply, "mul"},
        {divide, "div"},
        {modulo, "mod"},
        {is_equal, "eq"},
        {not_equal, "ne"},
        {is_greater, "gt"},
        {is_greater_equal, "ge"},
        {is_less, "lt"},
        {is_less_equal, "le"},
        {log_and, "and"},
        {log_or, "or"},
        {log_not, "not"},
        {jump_false, "jmp_false"},
        {jump, "jmp"},
        {prog_exit, "exit"},
        {print_int, "print_int"},
        {print_char, "print_char"},
        {read_char, "read_char"},
        {bgn_scope, "begin_scope"},
        {end_scope, "end_scope"},
        {array_allocate, "alloc"},
        {array_assign, "offset_set"},
        {array_get, "offset_get"},
    };

    string new_temp_var() {
        return "t" + to_string(temp_var_counter++);
    }

    string get_new_label() {
        return "label_" + to_string(label_counter++);
    }

    void gen_begin_scope() {
        scopes.emplace();
        instructions.push_back({
            bgn_scope,
        });
    }

    void gen_end_scope() {
        for (string&var: scopes.top()) {
            var_types.erase(var);
        }
        scopes.pop();
        instructions.push_back({
            end_scope,
        });
    }
};
