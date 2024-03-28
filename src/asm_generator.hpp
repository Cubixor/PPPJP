#pragma once
#include <cassert>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "ir_generator.hpp"

using namespace std;

class ASMGenerator {
public:
    explicit ASMGenerator(vector<TACInstruction>&instructions) : instructions(move(instructions)) {
    }

    void assign_variable(const string&ident, const string&value) {
        if (ident.starts_with("t")) {
            asm_push_stack(value);
        }
        else {
            const bool reassignment = stack_vars.contains(ident);
            load_stack_var(value, RAX);

            if (reassignment) {
                asm_mov_reg(get_var_pointer(ident), RAX);
            }
            else {
                stack_vars.emplace(ident, stack_size);
                asm_push_stack(RAX);
            }
        }
    }

    void generate_expression(const TACInstruction&instr) {
        load_stack_var(instr.arg1.value(), RAX);
        load_stack_var(instr.arg2.value(), RBX);

        string result_reg = RAX;

        switch (instr.op) {
            case add:
                asm_add(RAX, RBX);
                break;
            case multiply:
                asm_multiply(RBX);
                break;
            case substract:
                asm_substract(RAX, RBX);
                break;
            case divide:
                asm_divide(RBX);
                break;
            case modulo:
                asm_divide(RBX);
                result_reg = RDX;
                break;
            case is_equal:
                asm_cmp(RAX, RBX);
                asm_set_equal();
                break;
            case not_equal:
                asm_cmp(RAX, RBX);
                asm_set_not_equal();
                break;
            case is_greater:
                asm_cmp(RAX, RBX);
                asm_set_greater();
                break;
            case is_less:
                asm_cmp(RAX, RBX);
                asm_set_less();
                break;
            case is_greater_equal:
                asm_cmp(RAX, RBX);
                asm_set_greater_equal();
                break;
            case is_less_equal:
                asm_cmp(RAX, RBX);
                asm_set_less_equal();
                break;
            case log_and:
                asm_logical_and(RAX, RBX);
                break;
            case log_or:
                asm_logical_or(RAX, RBX);
                break;
            case log_not:
                asm_logical_not(RAX);
                break;

            default: assert(false);
        }

        asm_push_stack(result_reg);
    }

    void generate_instruction(const TACInstruction&instr) {
        switch (instr.op) {
            case label: {
                asm_label(instr.arg1.value());
                break;
            }
            case jump: {
                asm_jump(instr.arg1.value());
                break;
            }
            case jump_false: {
                load_stack_var(instr.arg1.value(), RAX);
                asm_test(RAX);
                asm_jump_zero(instr.arg2.value());
                break;
            }
            case assign: {
                assign_variable(instr.result.value(), instr.arg1.value());
                break;
            }
            case prog_exit: {
                load_stack_var(instr.arg1.value(), RDI);
                asm_exit();
                break;
            }
            case print_int: {
                load_stack_var(instr.arg1.value(), RAX);
                asm_print_int();
                break;
            }
            case print_char: {
                load_stack_var(instr.arg1.value(), RAX);
                asm_push_stack(RAX);
                asm_print_char("[rsp]");
                asm_pop_stack(RAX);
                break;
            }
            case read_char: {
                asm_read_char();
                break;
            }
            case bgn_scope: {
                begin_scope();
                break;
            }
            case OperationType::end_scope: {
                end_scope();
                break;
            }
            case array_allocate: {
                const string offset = get_loc_offset(heap_pointers.top());
                asm_mov_reg(RDI, offset);
                load_stack_var(instr.arg1.value(), RAX);
                asm_mov_reg(RBX, "8");
                asm_multiply(RBX);
                asm_add(RDI, RBX);
                asm_alloc_mem();

                heap_pointers.push(stack_size);
                stack_vars.insert({instr.result.value(), stack_size});
                asm_push_stack(RAX);

                break;
            }
            case array_assign: {
                load_stack_var(instr.result.value(), RBX);
                load_stack_var(instr.arg1.value(), RAX);
                asm_mov_reg(RCX, "8");
                asm_multiply(RCX);
                asm_add(RAX, RBX);
                load_stack_var(instr.arg2.value(), RBX);
                asm_mov_reg("QWORD [RAX]", RBX);

                break;
            }
            case array_get: {
                load_stack_var(instr.arg1.value(), RBX);
                load_stack_var(instr.arg2.value(), RAX);
                asm_mov_reg(RCX, "8");
                asm_multiply(RCX);
                asm_add(RAX, RBX);
                asm_mov_reg(RBX, "QWORD [RAX]");
                asm_push_stack(RBX);

                break;
            }

            default: generate_expression(instr);
        }
    }

    [[nodiscard]] string generate_program() {
        asm_header();
        asm_init_mem();
        heap_pointers.push(stack_size);
        asm_push_stack(RAX);


        for (TACInstruction&instruction: instructions) {
            generate_instruction(instruction);
        }

        return asm_out.str();
    }

private:
    const string RAX = "rax";
    const string RBX = "rbx";
    const string RCX = "rcx";
    const string RDX = "rdx";
    const string RDI = "rdi";
    const string RSI = "rsi";

    const string RSP = "rsp";
    const string RBP = "rbp";

    stringstream asm_out;

    size_t stack_size = 0;
    map<string, size_t> stack_vars;

    stack<size_t> heap_pointers;

    vector<size_t> scopes;

    vector<TACInstruction> instructions;

    void begin_scope() {
        scopes.push_back(stack_vars.size());
    }

    void end_scope() {
        const size_t var_count = stack_vars.size() - scopes.back();
        asm_add(RSP, to_string(var_count * 8));
        stack_size -= var_count;

        for (auto it = stack_vars.begin(); it != stack_vars.end();) {
            if (it->second > scopes.back()) {
                it = stack_vars.erase(it);
            }
            else {
                ++it;
            }
        }

        scopes.pop_back();
    }

    static bool is_num(const string&str) {
        char* p;
        strtol(str.c_str(), &p, 10);
        return *p == 0;
    }

    void load_stack_var(const string&ident, const string&reg) {
        if (is_num(ident)) {
            asm_mov_reg(reg, ident);
        }
        else if (ident[0] == 't') {
            asm_pop_stack(reg);
        }
        else {
            const string pointer = get_var_pointer(ident);
            asm_mov_reg(reg, pointer);
        }
    }

    string get_var_pointer(const string&ident) const {
        const size_t stack_loc = stack_vars.at(ident);
        return get_loc_offset(stack_loc);
    }

    string get_loc_offset(const size_t stack_loc) const {
        const string offset = to_string((stack_size - stack_loc - 1) * 8);

        return "QWORD [rsp + " + offset + "]";
    }

    void asm_push_stack(const string&reg) {
        asm_out << "    push " << reg << endl;
        stack_size++;
    }

    void asm_pop_stack(const string&reg) {
        asm_out << "    pop " << reg << endl;
        stack_size--;
    }

    void asm_mov_reg(const string&reg, const string&val) {
        asm_out << "    mov " << reg << ", " << val << endl;
    }

    void asm_add(const string&reg1, const string&reg2) {
        asm_out << "    add " << reg1 << ", " << reg2 << endl;
    }

    void asm_substract(const string&reg1, const string&reg2) {
        asm_out << "    sub " << reg1 << ", " << reg2 << endl;
    }

    void asm_multiply(const string&reg) {
        asm_out << "    mul " << reg << endl;
    }

    void asm_divide(const string&reg) {
        asm_out << "    xor rdx, rdx\n    div " << reg << endl;
    }

    void asm_logical_and(const string&reg1, const string&reg2) {
        asm_out << "    and " << reg1 << ", " << reg2 << endl;
    }

    void asm_logical_or(const string&reg1, const string&reg2) {
        asm_out << "    or " << reg1 << ", " << reg2 << endl;
    }

    void asm_logical_not(const string&reg) {
        asm_out << "    not " << reg << endl;
    }

    void asm_test(const string&reg) {
        asm_out << "    test " << reg << ", " << reg << endl;
    }

    void asm_cmp(const string&reg1, const string&reg2) {
        asm_out << "    cmp " << reg1 << ", " << reg2 << endl;
    }

    void asm_set_equal() {
        asm_out << "    setz al" << endl;
    }

    void asm_set_not_equal() {
        asm_out << "    setnz al" << endl;
    }

    void asm_set_greater() {
        asm_out << "    setg al" << endl;
    }

    void asm_set_less() {
        asm_out << "    setl al" << endl;
    }

    void asm_set_greater_equal() {
        asm_out << "    setge al" << endl;
    }

    void asm_set_less_equal() {
        asm_out << "    setle al" << endl;
    }

    void asm_jump_zero(const string&label) {
        asm_out << "    jz " << label << endl;
    }

    void asm_jump(const string&label) {
        asm_out << "    jmp " << label << endl;
    }

    void asm_label(const string&label) {
        asm_out << label << ":" << endl;
    }

    void asm_print_int() {
        asm_out << "    call _print_int" << endl;
    }

    void asm_print_char(const string&pointer) {
        asm_out << "    mov rax, 1\n    mov rdi, 1\n    mov rdx, 1\n    lea rsi, " << pointer << "\n    syscall\n";
    }

    void asm_init_mem() {
        asm_out << "    mov rax, 12\n    mov rdi, 0\n    syscall\n";
    }

    void asm_alloc_mem() {
        asm_out << "    mov rax, 12\n    syscall\n";
    }

    void asm_read_char() {
        asm_out << "    inc rsp\n    mov rax, 0\n    mov rdi, 0\n    mov rdx, 1\n    lea rsi, [rsp]\n    syscall\n";
        stack_size++;
    }

    void asm_exit() {
        asm_out << "    mov rax, 60\n    syscall\n";
    }

    void asm_header() {
        asm_out << "%include \"printer.asm\"" << endl
                << "global _start" << endl
                << "_start:" << endl;
    }
};
