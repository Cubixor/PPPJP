#pragma once

#include <map>
#include <sstream>

#include "parser.hpp"

using namespace std;


const string RAX = "rax";
const string RBX = "rbx";
const string RDI = "rdi";

class Generator {
public:
    explicit Generator(NodeStart root) : root(std::move(root)) {
    }

    void generate_arithm_expr(const NodeArithExpr* expr) {
        generate_expr(expr->right);
        generate_expr(expr->left);

        pop_stack(RAX);
        pop_stack(RBX);

        switch (expr->type) {
            case TokenType::add:
                add(RAX, RBX);
                break;
            case TokenType::multiply:
                multiply(RBX);
                break;
            case TokenType::substract:
                subtract(RAX, RBX);
                break;
            case TokenType::divide:
                divide(RBX);
                break;
            default:
                assert(false); //Unreachable
        }

        push_stack(RAX);
    }

    void generate_expr(NodeExpr* expr) {
        struct ExprVisitor {
            Generator&gen;

            void operator()(const NodeArithExpr* arith_expr) const {
                gen.generate_arithm_expr(arith_expr);
            }

            void operator()(NodeTerm* term) const {
                gen.generate_term(term);
            }
        };

        ExprVisitor visitor = {*this};
        visit(visitor, expr->var);
    }

    void generate_term(NodeTerm* term) {
        struct TermVisitor {
            Generator&gen;

            void operator()(const NodeTermIntLit* term_int_lit) const {
                gen.mov_reg(RAX, term_int_lit->value);
                gen.push_stack(RAX);
            }

            void operator()(const NodeTermIdent* term_ident) const {
                const string* ident = &term_ident->ident.value.value();
                if (!gen.stack_vars.contains(*ident)) {
                    cerr << "Undeclared identifier '" << *ident << "'!" << endl;
                    exit(EXIT_FAILURE);
                }

                const size_t stack_loc = gen.stack_vars.at(*ident);
                const size_t offset = (gen.stack_size - stack_loc - 1) * 8;
                const string qword_offset = "QWORD [rsp + " + to_string(offset) + "]";
                gen.push_stack(qword_offset);
            }

            void operator()(const NodeTermParen* term_paren) const {
                gen.generate_expr(term_paren->expr);
            }
        };
        TermVisitor visitor{*this};
        visit(visitor, term->var);
    }

    void generate_statement(NodeStatement* stmt) {
        struct StatementVisitor {
            Generator&gen;

            void operator()(const NodeStmtVariable* stmt_var) const {
                const string* ident = &stmt_var->ident.value.value();
                if (gen.stack_vars.contains(*ident)) {
                    cerr << "Identifier already declared '" << *ident << "'!" << endl;
                    exit(EXIT_FAILURE);
                }

                gen.stack_vars.insert({*ident, gen.stack_size});
                gen.generate_expr(stmt_var->expr);
            }

            void operator()(const NodeStmtExit* stmt_exit) const {
                gen.generate_expr(stmt_exit->expr);
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
                gen.generate_expr(stmt_if->expr);
                gen.pop_stack(RAX);

                const string false_label = gen.create_label();
                gen.test_condition(false_label);

                gen.generate_statement(stmt_if->stmt);

                if (stmt_if->pred_else.has_value()) {
                    const string true_label = gen.create_label();
                    gen.jump(true_label);

                    gen.label(false_label);
                    gen.generate_statement(stmt_if->pred_else.value()->stmt);
                    gen.label(true_label);
                }
                else {
                    gen.label(false_label);
                }
            }
        };
        StatementVisitor visitor{*this};
        visit(visitor, stmt->var);
    }

    [[nodiscard]] string generate_program() {
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

private:
    NodeStart root;
    stringstream asm_out;
    size_t stack_size = 0;
    map<string, size_t> stack_vars;
    vector<size_t> scopes;
    int if_label_count = 0;


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

    void syscall() {
        asm_out << "    syscall" << endl;
    }

    void test_condition(const string&label) {
        asm_out << "    test rax, rax" << endl;
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

    string create_label() {
        return "if_" + to_string(if_label_count++);
    }
};
