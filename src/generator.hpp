#pragma once

#include <map>
#include <sstream>
#include <stack>

#include "parser.hpp"

using namespace std;


const string RAX = "rax";
const string RBX = "rbx";
const string RDX = "rdx";
const string RDI = "rdi";

class Generator {
public:
    explicit Generator(NodeStart root) : root(std::move(root)) {
    }

    static void check_token(const TokenType result_type, const TokenType expected_type) {
        if (result_type == expected_type) return;

        bool ok;
        switch (expected_type) {
            case TokenType::var_type_int:
                ok = arithmetic_tokens.contains(result_type);
                break;
            case TokenType::var_type_boolean:
                ok = boolean_tokens.contains(result_type);
                break;
            default: ok = false;
        }

        if (!ok) {
            //TODO Add line numbers
            cerr << "[BŁĄD] [Analisa semantyczna] Niezgodność typu danych, oczekiwano `" + get_token_names({expected_type}) << "' \n\t w linijce X"<<endl;
            exit(EXIT_FAILURE);
        }
    }

    static TokenType get_expr_type(const TokenType opr) {
        if (logical_tokens.contains(opr)) return TokenType::var_type_boolean;
        return TokenType::var_type_int;
    }

    void generate_bin_expr(const NodeBinExpr* expr, const TokenType expected_type) {
        check_token(expr->type, expected_type);

        const TokenType expected_param_type = get_expr_type(expr->type);
        generate_expr(expr->right, expected_param_type);
        generate_expr(expr->left, expected_param_type);

        pop_stack(RAX);
        pop_stack(RBX);

        switch (expr->type) {
            case TokenType::add:
                add(RAX, RBX);
                push_stack(RAX);
                break;
            case TokenType::multiply:
                multiply(RBX);
                push_stack(RAX);
                break;
            case TokenType::substract:
                subtract(RAX, RBX);
                push_stack(RAX);
                break;
            case TokenType::divide:
                divide(RBX);
                push_stack(RAX);
                break;
            case TokenType::modulo:
                divide(RBX);
                push_stack(RDX);
                break;
            case TokenType::equal:
                cmp(RAX, RBX);
                set_equal();
                push_stack(RAX);
                break;
            case TokenType::not_equal:
                cmp(RAX, RBX);
                set_not_equal();
                push_stack(RAX);
                break;
            case TokenType::greater:
                cmp(RAX, RBX);
                set_greater();
                push_stack(RAX);
                break;
            case TokenType::less:
                cmp(RAX, RBX);
                set_less();
                push_stack(RAX);
                break;
            case TokenType::greater_equal:
                cmp(RAX, RBX);
                set_greater_equal();
                push_stack(RAX);
                break;
            case TokenType::less_equal:
                cmp(RAX, RBX);
                set_less_equal();
                push_stack(RAX);
                break;
            case TokenType::logical_and:
                logical_and(RAX, RBX);
                push_stack(RAX);
                break;
            case TokenType::logical_or:
                logical_or(RAX, RBX);
                push_stack(RAX);
                break;
            default:
                assert(false); //Unreachable
        }
    }

    void generate_un_expr(const NodeUnExpr* un_expr, const TokenType expected_type) {
        check_token(un_expr->type, expected_type);

        const TokenType expected_param_type = get_expr_type(un_expr->type);
        generate_term(un_expr->term, expected_param_type);

        pop_stack(RAX);

        switch (un_expr->type) {
            case TokenType::logical_not:
                logical_not(RAX);
                push_stack(RAX);
                break;
            default: assert(false);
        }
    }

    void generate_expr(NodeExpr* expr, const TokenType expected_type) {
        struct ExprVisitor {
            Generator&gen;
            TokenType expected_type;

            void operator()(const NodeBinExpr* arith_expr) const {
                gen.generate_bin_expr(arith_expr, expected_type);
            }

            void operator()(NodeTerm* term) const {
                gen.generate_term(term, expected_type);
            }

            void operator()(const NodeUnExpr* un_expr) const {
                gen.generate_un_expr(un_expr, expected_type);
            }
        };

        ExprVisitor visitor = {*this, expected_type};
        visit(visitor, expr->var);
    }

    void generate_term(NodeTerm* term, const TokenType expected_type) {
        struct TermVisitor {
            Generator&gen;
            TokenType expected_type;

            void operator()(const NodeTermIntLit* term_int_lit) const {
                check_token(TokenType::var_type_int, expected_type);
                gen.mov_reg(RAX, term_int_lit->value);
                gen.push_stack(RAX);
            }

            void operator()(const NodeTermBoolLit* bool_lit) const {
                check_token(TokenType::var_type_boolean, expected_type);
                gen.mov_reg(RAX, bool_lit->value ? "1" : "0");
                gen.push_stack(RAX);
            }

            void operator()(const NodeTermIdent* term_ident) const {
                const string* ident = &term_ident->ident.value.value();
                if (!gen.stack_vars.contains(*ident)) {
                    cerr << "[BŁĄD] [Analiza semantyczna] Nieznany identyfikator '" << *ident << "' \n\t w linijce " << term_ident->ident.line << endl;
                    exit(EXIT_FAILURE);
                }
                auto&[location, type] = gen.stack_vars[*ident];
                check_token(type, expected_type);

                const string qword_offset = "QWORD [rsp + " + gen.get_variable_offset(*ident) + "]";
                gen.push_stack(qword_offset);
            }

            void operator()(const NodeTermParen* term_paren) const {
                gen.generate_expr(term_paren->expr, expected_type);
            }
        };
        TermVisitor visitor{*this, expected_type};
        visit(visitor, term->var);
    }

    void generate_if_pred(const NodeIfPred* pred_if, const string&false_label, const string&end_label) {
        generate_expr(pred_if->expr, TokenType::var_type_boolean);
        pop_stack(RAX);
        test(RAX);
        jump_zero(false_label);
        generate_statement(pred_if->stmt);
        jump(end_label);
    }

    void generate_statement(NodeStatement* stmt) {
        struct StatementVisitor {
            Generator&gen;

            void operator()(const NodeStmtVariable* stmt_var) const {
                const string* ident = &stmt_var->ident.value.value();
                if (gen.stack_vars.contains(*ident)) {
                    cerr << "[BŁĄD] [Analiza semantyczna] Redeklaracja identyfikatora '" << *ident << "' \n\t w linijce " << stmt_var->ident.line << endl;
                    exit(EXIT_FAILURE);
                }

                gen.stack_vars.insert({*ident, Var{gen.stack_size, stmt_var->type}});
                gen.generate_expr(stmt_var->expr, stmt_var->type);
            }

            void operator()(const NodeStmtExit* stmt_exit) const {
                gen.generate_expr(stmt_exit->expr, TokenType::var_type_int);
                gen.mov_reg(RAX, "60");
                gen.pop_stack(RDI);
                gen.syscall();
            }

            void operator()(const NodeStmtScope* stmt_scope) const {
                gen.begin_scope();

                for (NodeStatement* stmt: stmt_scope->statements) {
                    gen.generate_statement(stmt);
                }

                gen.end_scope();
            }

            void operator()(const NodeStmtIf* stmt_if) const {
                const string&end_label = gen.get_new_label();
                string false_label = gen.get_new_label();
                gen.generate_if_pred(stmt_if->pred, false_label, end_label);

                for (const NodeIfPred* pred_if: stmt_if->pred_elif) {
                    gen.label(false_label);
                    false_label = gen.get_new_label();
                    gen.generate_if_pred(pred_if, false_label, end_label);
                }

                gen.label(false_label);

                if (stmt_if->pred_else.has_value()) {
                    gen.generate_statement(stmt_if->pred_else.value()->stmt);
                }

                gen.label(end_label);
            }

            void operator()(const NodeStmtAssign* stmt_assign) const {
                const string&ident = stmt_assign->ident.value.value();

                if (!gen.stack_vars.contains(ident)) {
                    cerr << "[BŁĄD] [Analiza semantyczna] Nieznany identyfikator '" << ident << "' \n\t w linijce " << stmt_assign->ident.line << endl;
                    exit(EXIT_FAILURE);
                }
                auto&[location, type] = gen.stack_vars[ident];

                gen.generate_expr(stmt_assign->expr, type);
                gen.pop_stack(RAX);
                gen.mov_reg("[rsp + " + gen.get_variable_offset(ident) + "]", RAX);
            }

            void operator()(const NodeStmtWhile* stmt_while) const {
                const string start_label = gen.get_new_label();
                const string end_label = gen.get_new_label();
                auto label_pair = pair{start_label, end_label};
                gen.loop_labels.emplace(label_pair);

                gen.label(start_label);

                if (stmt_while->expr.has_value()) {
                    gen.generate_expr(stmt_while->expr.value(), TokenType::var_type_boolean);
                    gen.pop_stack(RAX);
                    gen.test(RAX);
                    gen.jump_zero(end_label);
                }

                gen.generate_statement(stmt_while->stmt);
                gen.jump(start_label);

                gen.label(end_label);

                if (!gen.loop_labels.empty() && gen.loop_labels.top() == label_pair) {
                    gen.loop_labels.pop();
                }
            }

            void operator()(const NodeStmtBreak* stmt_break) const {
                if (gen.loop_labels.empty()) {
                    cerr << "[BŁĄD] [Analiza semantyczna] Instrukcja 'przerwij' poza zakresem pętli \n\t w linijce " << stmt_break->token.line << endl;
                    exit(EXIT_FAILURE);
                }

                const string&label = gen.loop_labels.top().second;
                gen.jump(label);
                gen.loop_labels.pop();
            }

            void operator()(const NodeStmtContinue* stmt_continue) const {
                if (gen.loop_labels.empty()) {
                    cerr << "[BŁĄD] [Analiza semantyczna] Instrukcja 'kontynuuj' poza zakresem pętli \n\t w linijce " << stmt_continue->token.line << endl;
                    exit(EXIT_FAILURE);
                }

                const string&label = gen.loop_labels.top().first;
                gen.jump(label);
            }

            void operator()(const NodeStmtPrint* stmt_print) const {
                gen.generate_expr(stmt_print->expr, TokenType::var_type_int);
                gen.pop_stack(RAX);
                gen.print_int();
            }
        };
        StatementVisitor visitor{*this};
        visit(visitor, stmt->var);
    }

    [[nodiscard]] string generate_program() {
        asm_out << "%include \"printer.asm\"" << endl;
        asm_out << "global _start" << endl;
        asm_out << "_start:" << endl;

        bool contains_exit = false;
        for (NodeStatement* stmt: root.statements) {
            if (holds_alternative<NodeStmtExit *>(stmt->var)) {
                contains_exit = true;
            }

            generate_statement(stmt);
        }

        if (!contains_exit) {
            mov_reg(RAX, "60");
            mov_reg(RDI, "0");
            syscall();
        }

        return asm_out.str();
    }

    struct Var {
        size_t location;
        TokenType type;
    };

private:
    NodeStart root;
    stringstream asm_out;

    size_t stack_size = 0;
    map<string, Var> stack_vars;

    vector<size_t> scopes;
    int label_count = 0;
    stack<pair<string, string>> loop_labels;

    string get_variable_offset(const string&ident) const {
        const size_t stack_loc = stack_vars.at(ident).location;
        return to_string((stack_size - stack_loc - 1) * 8);
    }

    void push_stack(const string&reg) {
        asm_out << "    push " << reg << endl;
        stack_size++;
    }

    void pop_stack(const string&reg) {
        asm_out << "    pop " << reg << endl;
        stack_size--;
    }

    void mov_reg(const string&reg, const string&val) {
        asm_out << "    mov " << reg << ", " << val << endl;
    }

    void add(const string&reg1, const string&reg2) {
        asm_out << "    add " << reg1 << ", " << reg2 << endl;
    }

    void subtract(const string&reg1, const string&reg2) {
        asm_out << "    sub " << reg1 << ", " << reg2 << endl;
    }

    void multiply(const string&reg) {
        asm_out << "    mul " << reg << endl;
    }

    void divide(const string&reg) {
        asm_out << "    div " << reg << endl;
    }

    void logical_and(const string&reg1, const string&reg2) {
        asm_out << "    and " << reg1 << ", " << reg2 << endl;
    }

    void logical_or(const string&reg1, const string&reg2) {
        asm_out << "    or " << reg1 << ", " << reg2 << endl;
    }

    void logical_not(const string&reg) {
        asm_out << "    not " << reg << endl;
    }

    void syscall() {
        asm_out << "    syscall" << endl;
    }

    void test(const string&reg) {
        asm_out << "    test " << reg << ", " << reg << endl;
    }

    void cmp(const string&reg1, const string&reg2) {
        asm_out << "    cmp " << reg1 << ", " << reg2 << endl;
    }

    void set_equal() {
        asm_out << "    setz al" << endl;
    }

    void set_not_equal() {
        asm_out << "    setnz al" << endl;
    }

    void set_greater() {
        asm_out << "    setg al" << endl;
    }

    void set_less() {
        asm_out << "    setl al" << endl;
    }

    void set_greater_equal() {
        asm_out << "    setge al" << endl;
    }

    void set_less_equal() {
        asm_out << "    setle al" << endl;
    }

    void jump_zero(const string&label) {
        asm_out << "    jz " << label << endl;
    }

    void jump(const string&label) {
        asm_out << "    jmp " << label << endl;
    }

    void label(const string&label) {
        asm_out << label << ":" << endl;
    }

    void begin_scope() {
        scopes.push_back(stack_vars.size());
    }

    void end_scope() {
        const size_t var_count = stack_vars.size() - scopes.back();
        asm_out << "    add rsp, " << var_count * 8 << endl;
        stack_size -= var_count;

        auto it = stack_vars.rbegin();
        for (int i = 0; i < var_count; i++) {
            stack_vars.erase(it->first);
            ++it;
        }

        scopes.pop_back();
    }

    void print_int() {
        asm_out << "    call _print_int" << endl;
    }

    string get_new_label() {
        return "label_" + to_string(label_count++);
    }
};
