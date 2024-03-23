#pragma once

#include <map>
#include <sstream>
#include <stack>

#include "parser.hpp"

using namespace std;


const string RAX = "rax";
const string RBX = "rbx";
const string RCX = "rcx";
const string RDX = "rdx";
const string RDI = "rdi";
const string RSI = "rsi";

class Generator {
public:
    explicit Generator(NodeStart root) : root(std::move(root)) {
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

    struct Var {
        size_t location;
        TokenType type;
    };

    void generate_bin_expr(const NodeBinExpr* expr, const TokenType expected_type) {
        check_operator(expr->opr.type, expected_type, expr->opr.line);

        const TokenType expected_param_type = get_param_type(expr->opr.type);
        generate_expr(expr->right, expected_param_type);
        generate_expr(expr->left, expected_param_type);

        pop_stack(RAX);
        pop_stack(RBX);

        switch (expr->opr.type) {
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
        check_operator(un_expr->opr.type, expected_type, un_expr->opr.line);

        const TokenType expected_param_type = get_param_type(un_expr->opr.type);
        generate_term(un_expr->term, expected_param_type);

        pop_stack(RAX);

        switch (un_expr->opr.type) {
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

    void generate_term(NodeTerm* term, const TokenType expected_type, Var* var = nullptr) {
        struct TermVisitor {
            Generator&gen;
            TokenType expected_type;

            void operator()(const NodeTermIntLit* term_int_lit) const {
                check_token(TokenType::var_type_int, expected_type, term_int_lit->int_lit.line);
                gen.mov_reg(RAX, term_int_lit->int_lit.value.value());
                gen.push_stack(RAX);
            }

            void operator()(const NodeTermBoolLit* term_bool_lit) const {
                check_token(TokenType::var_type_boolean, expected_type, term_bool_lit->bool_lit.line);
                gen.mov_reg(RAX, term_bool_lit->bool_lit.value.value());
                gen.push_stack(RAX);
            }

            void operator()(const NodeTermCharLit* term_char_lit) const {
                check_token(TokenType::var_type_char, expected_type, term_char_lit->char_lit.line);

                const int c = static_cast<unsigned char>(term_char_lit->char_lit.value.value()[0]);
                gen.mov_reg(RAX, to_string(c));
                gen.push_stack(RAX);
            }

            void operator()(const NodeTermStringLit* term_string_lit) const {
                check_token(TokenType::var_type_string, expected_type, term_string_lit->array_expr->token.line);
                //TODO
                //gen.generate_array_expr(term_string_lit->array_expr, );
            }

            void operator()(const NodeTermIdent* term_ident) const {
                const string* ident = &term_ident->ident.value.value();
                gen.check_ident(term_ident->ident, true);

                const Var var = gen.stack_vars[*ident];
                check_token(var.type, expected_type, term_ident->ident.line);

                const string qword_offset = "QWORD [rsp + " + gen.get_variable_offset(*ident) + "]";
                gen.push_stack(qword_offset);
            }

            void operator()(const NodeTermParen* term_paren) const {
                gen.generate_expr(term_paren->expr, expected_type);
            }

            void operator()(const NodeTermArrIdent* arr_ident) const {
                const string* ident = &arr_ident->ident.value.value();
                gen.check_ident(arr_ident->ident, true);

                const Var var = gen.stack_vars[*ident];
                check_token(var.type, expected_type, arr_ident->ident.line);

                gen.generate_expr(arr_ident->index, TokenType::var_type_int);

                const string qword_offset = "QWORD [rsp + " + gen.get_variable_offset(*ident) + "]";
                gen.mov_reg(RBX, qword_offset);

                gen.pop_stack(RAX);
                gen.mov_reg(RCX, "8");
                gen.multiply(RCX);
                gen.add(RAX, RBX);

                gen.mov_reg(RBX, "qword [rax]");
                gen.push_stack(RBX);
            }


            void operator()(const NodeTermArray* array_expr) const {
                check_token(TokenType::array, expected_type, array_expr->token.line);
                //TODO
                //gen.generate_array_expr(array_expr, *var);
            }

            void operator()(const NodeTermReadChar* read_char) const {
                check_token(TokenType::var_type_char, expected_type, read_char->token.line);

                gen.read_char();
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

    void check_ident(const Token&ident, const bool should_exist) const {
        const string&ident_str = ident.value.value();

        if (stack_vars.contains(ident_str)) {
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

    void generate_array_expr(const NodeTermArray* arr_expr, const Var&var) {
        for (int index = 0; index < arr_expr->exprs.size(); index++) {
            generate_expr(arr_expr->exprs.at(index), var.type);

            const string pointer_offset = "QWORD [rsp + " + get_location_offset(var.location) + "]";
            mov_reg(RAX, pointer_offset);
            mov_reg(RBX, to_string(8 * index));
            add(RAX, RBX);
            pop_stack(RBX);
            mov_reg("qword [rax]", RBX);
        }
    }

    void generate_statement(NodeStatement* stmt) {
        struct StatementVisitor {
            Generator&gen;

            void operator()(const NodeStmtVariable* stmt_var) const {
                const string* ident = &stmt_var->ident.value.value();
                gen.check_ident(stmt_var->ident, false);

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
                const string* ident = &stmt_assign->ident.value.value();
                gen.check_ident(stmt_assign->ident, true);

                Var var = gen.stack_vars[*ident];

                gen.generate_expr(stmt_assign->expr, var.type);
                gen.pop_stack(RAX);
                gen.mov_reg("[rsp + " + gen.get_variable_offset(*ident) + "]", RAX);
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
                    cerr << "[BŁĄD] [Analiza semantyczna] Instrukcja 'przerwij' poza zakresem pętli \n\t w linijce " <<
                            stmt_break->token.line << endl;
                    exit(EXIT_FAILURE);
                }

                const string&label = gen.loop_labels.top().second;
                gen.jump(label);
                gen.loop_labels.pop();
            }

            void operator()(const NodeStmtContinue* stmt_continue) const {
                if (gen.loop_labels.empty()) {
                    cerr << "[BŁĄD] [Analiza semantyczna] Instrukcja 'kontynuuj' poza zakresem pętli \n\t w linijce " <<
                            stmt_continue->token.line << endl;
                    exit(EXIT_FAILURE);
                }

                const string&label = gen.loop_labels.top().first;
                gen.jump(label);
            }

            void operator()(const NodeStmtPrintInt* stmt_print) const {
                gen.generate_expr(stmt_print->expr, TokenType::var_type_int);
                gen.pop_stack(RAX);
                gen.print_int();
            }

            void operator()(const NodeStmtPrintChar* stmt_print) const {
                gen.generate_expr(stmt_print->expr, TokenType::var_type_char);
                gen.print_char();
                gen.pop_stack(RAX);
            }

            void operator()(const NodeStmtArray* stmt_array) const {
                const string* ident = &stmt_array->ident.value.value();
                gen.check_ident(stmt_array->ident, false);

                gen.generate_expr(stmt_array->size, TokenType::var_type_int);

                const string qword_offset = "QWORD [rsp + " + gen.get_location_offset(gen.heap_pointers.top()) + "]";
                gen.mov_reg(RDI, qword_offset);

                gen.pop_stack(RAX);
                gen.mov_reg(RBX, "8");
                gen.multiply(RBX);
                gen.add(RDI, RBX);
                gen.alloc_mem();

                gen.heap_pointers.push(gen.stack_size);
                auto var = Var{gen.stack_size, stmt_array->type};
                gen.stack_vars.insert({*ident, var});

                gen.push_stack(RAX);


                if (stmt_array->contents.has_value()) {
                    const NodeTermArray* array_expr = stmt_array->contents.value();

                    gen.generate_array_expr(array_expr, var);
                }
            }

            void operator()(const NodeStmtArrAssign* stmt_arr_assign) const {
                const string* ident = &stmt_arr_assign->ident.value.value();
                gen.check_ident(stmt_arr_assign->ident, true);

                Var var = gen.stack_vars[*ident];

                gen.generate_expr(stmt_arr_assign->expr, var.type);
                gen.generate_expr(stmt_arr_assign->index, TokenType::var_type_int);

                const string pointer_offset = "QWORD [rsp + " + gen.get_variable_offset(*ident) + "]";
                gen.mov_reg(RBX, pointer_offset);

                gen.pop_stack(RAX);
                gen.mov_reg(RCX, "8");
                gen.multiply(RCX);
                gen.add(RAX, RBX);

                gen.pop_stack(RBX);

                gen.mov_reg("qword [rax]", RBX);
            }
        };
        StatementVisitor visitor{*this};
        visit(visitor, stmt->var);
    }

    [[nodiscard]] string generate_program() {
        asm_out << "%include \"printer.asm\"" << endl;
        asm_out << "global _start" << endl;
        asm_out << "_start:" << endl;

        init_mem();
        heap_pointers.push(stack_size);
        push_stack(RAX);

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

private:
    NodeStart root;
    stringstream asm_out;

    size_t stack_size = 0;
    map<string, Var> stack_vars;

    stack<size_t> heap_pointers;

    vector<size_t> scopes;
    int label_count = 0;
    stack<pair<string, string>> loop_labels;

    string get_variable_offset(const string&ident) const {
        const size_t stack_loc = stack_vars.at(ident).location;
        return get_location_offset(stack_loc);
    }

    string get_location_offset(const size_t stack_loc) const {
        return to_string((stack_size - stack_loc - 1) * 8);
    }

    string get_new_label() {
        return "label_" + to_string(label_count++);
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
        asm_out << "    xor rdx, rdx\n    div " << reg << endl;
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

    void print_char() {
        asm_out << "    mov rax, 1\n    mov rdi, 1\n    mov rdx, 1\n    lea rsi, [rsp]\n    syscall" << endl;
    }

    void init_mem() {
        asm_out << "    mov rax, 12\n    mov rdi, 0\n    syscall\n";
    }

    void alloc_mem() {
        asm_out << "    mov rax, 12\n    syscall\n";
    }

    void read_char() {
        asm_out<< "    inc rsp\n    mov rax, 0\n    mov rdi, 0\n    mov rdx, 1\n    lea rsi, [rsp]\n    syscall\n";
        stack_size++;
    }
};
