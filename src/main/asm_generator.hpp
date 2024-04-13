#pragma once

#include <cassert>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

#include "ir_generator.hpp"

class ASMGenerator {
public:
    explicit ASMGenerator(std::vector<TACInstruction> &instructions) : instructions(std::move(instructions)) {
    }

    void assign_variable(const std::string &ident, const std::string &value) {
        if (ident.starts_with("t")) {
            asm_push_stack(value);
        } else {
            const bool reassignment = stack_vars.contains(ident);
            load_stack_var(value, RAX);

            if (reassignment) {
                asm_mov_reg(get_var_pointer(ident), RAX);
            } else {
                stack_vars.try_emplace(ident, stack_size);
                asm_push_stack(RAX);
            }
        }
    }

    void generate_expression(const TACInstruction &instr) {
        load_stack_var(instr.arg1.value(), RAX);
        if (instr.arg2.has_value())
            load_stack_var(instr.arg2.value(), RBX);

        std::string result_reg = RAX;

        switch (instr.op) {
            case OperationType::add:
                asm_add(RAX, RBX);
                break;
            case OperationType::multiply:
                asm_multiply(RBX);
                break;
            case OperationType::subtract:
                asm_substract(RAX, RBX);
                break;
            case OperationType::divide:
                asm_divide(RBX);
                break;
            case OperationType::modulo:
                asm_divide(RBX);
                result_reg = RDX;
                break;
            case OperationType::is_equal:
                asm_cmp(RAX, RBX);
                asm_set_equal();
                break;
            case OperationType::not_equal:
                asm_cmp(RAX, RBX);
                asm_set_not_equal();
                break;
            case OperationType::is_greater:
                asm_cmp(RAX, RBX);
                asm_set_greater();
                break;
            case OperationType::is_less:
                asm_cmp(RAX, RBX);
                asm_set_less();
                break;
            case OperationType::is_greater_equal:
                asm_cmp(RAX, RBX);
                asm_set_greater_equal();
                break;
            case OperationType::is_less_equal:
                asm_cmp(RAX, RBX);
                asm_set_less_equal();
                break;
            case OperationType::log_and:
                asm_logical_and(RAX, RBX);
                break;
            case OperationType::log_or:
                asm_logical_or(RAX, RBX);
                break;
            case OperationType::log_not:
                asm_logical_not(RAX);
                break;

            default:
                assert(false);
        }

        asm_push_stack(result_reg);
    }

    void generate_instruction(const TACInstruction &instr) {
        switch (instr.op) {
            case OperationType::label: {
                asm_label(instr.arg1.value());
                break;
            }
            case OperationType::jump: {
                asm_jump(instr.arg1.value());
                break;
            }
            case OperationType::jump_false: {
                load_stack_var(instr.arg1.value(), RAX);
                asm_test(RAX);
                asm_jump_zero(instr.arg2.value());
                break;
            }
            case OperationType::assign: {
                assign_variable(instr.result.value(), instr.arg1.value());
                break;
            }
            case OperationType::prog_exit: {
                load_stack_var(instr.arg1.value(), RDI);
                asm_exit();
                break;
            }
            case OperationType::print_int: {
                load_stack_var(instr.arg1.value(), RAX);
                asm_print_int();
                break;
            }
            case OperationType::print_char: {
                load_stack_var(instr.arg1.value(), RAX);
                asm_push_stack(RAX);
                asm_print_char("[rsp]");
                asm_pop_stack(RAX);
                break;
            }
            case OperationType::read_char: {
                asm_read_char();
                break;
            }
            case OperationType::bgn_scope: {
                begin_scope();
                break;
            }
            case OperationType::end_scope: {
                end_scope();
                break;
            }
            case OperationType::array_allocate: {
                size_t loc = heap_pointers.top();
                const std::string offset = get_loc_offset(loc);
                asm_mov_reg(RDI, offset);
                load_stack_var(instr.arg1.value(), RAX);
                asm_mov_reg(RBX, "8");
                asm_multiply(RBX);
                asm_add(RDI, RAX);
                asm_alloc_mem();

                stack_vars.try_emplace(instr.result.value(), loc);
                heap_pointers.push(stack_size);
                asm_push_stack(RAX);

                break;
            }
            case OperationType::array_assign: {
                load_stack_var(instr.result.value(), RBX);
                load_stack_var(instr.arg1.value(), RAX);
                asm_mov_reg(RCX, "8");
                asm_multiply(RCX);
                asm_add(RAX, RBX);
                load_stack_var(instr.arg2.value(), RBX);
                asm_mov_reg("QWORD [RAX]", RBX);

                break;
            }
            case OperationType::array_get: {
                load_stack_var(instr.arg1.value(), RBX);
                load_stack_var(instr.arg2.value(), RAX);
                asm_mov_reg(RCX, "8");
                asm_multiply(RCX);
                asm_add(RAX, RBX);
                asm_mov_reg(RBX, "QWORD [RAX]");
                asm_push_stack(RBX);

                break;
            }

            default:
                generate_expression(instr);
        }
    }

    [[nodiscard]] std::string generate_program() {
        asm_header();
        asm_init_mem();
        heap_pointers.push(stack_size);
        asm_push_stack(RAX);


        for (TACInstruction const &instruction: instructions) {
            generate_instruction(instruction);
        }

        return asm_out.str();
    }

private:
    const std::string RAX = "rax";
    const std::string RBX = "rbx";
    const std::string RCX = "rcx";
    const std::string RDX = "rdx";
    const std::string RDI = "rdi";
    const std::string RSI = "rsi";

    const std::string RSP = "rsp";
    const std::string RBP = "rbp";

    std::stringstream asm_out;

    size_t stack_size = 0;
    std::map<std::string, size_t> stack_vars;

    std::stack<size_t> heap_pointers;

    std::vector<size_t> scopes;

    std::vector<TACInstruction> instructions;

    void begin_scope() {
        scopes.push_back(stack_vars.size());
    }

    void end_scope() {
        bool heap_freed = false;
        while (heap_pointers.top() > scopes.back()) {
            heap_freed = true;
            heap_pointers.pop();
        }

        if (heap_freed) {
            asm_mov_reg(RDI, get_loc_offset(heap_pointers.top()));
            asm_alloc_mem();
        }

        const size_t var_count = stack_vars.size() - scopes.back();
        asm_add(RSP, std::to_string(var_count * 8));
        stack_size -= var_count;

        /*for (auto it = stack_vars.begin(); it != stack_vars.end();) {
            if (it->second > scopes.back()) {
                it = stack_vars.erase(it);
            }
            else {
                ++it;
            }
        }*/

        erase_if(stack_vars, [this](auto it) {
            return it.second > scopes.back();
        });


        scopes.pop_back();
    }

    static bool is_num(const std::string &str) {
        char *p;
        strtol(str.c_str(), &p, 10);
        return *p == 0;
    }

    void load_stack_var(const std::string &ident, const std::string &reg) {
        if (is_num(ident)) {
            asm_mov_reg(reg, ident);
        } else if (ident[0] == 't') {
            asm_pop_stack(reg);
        } else {
            const std::string pointer = get_var_pointer(ident);
            asm_mov_reg(reg, pointer);
        }
    }

    std::string get_var_pointer(const std::string &ident) const {
        const size_t stack_loc = stack_vars.at(ident);
        return get_loc_offset(stack_loc);
    }

    std::string get_loc_offset(const size_t stack_loc) const {
        const std::string offset = std::to_string((stack_size - stack_loc - 1) * 8);

        return "QWORD [rsp + " + offset + "]";
    }

    void asm_push_stack(const std::string &reg) {
        asm_out << "    push " << reg << std::endl;
        stack_size++;
    }

    void asm_pop_stack(const std::string &reg) {
        asm_out << "    pop " << reg << std::endl;
        stack_size--;
    }

    void asm_mov_reg(const std::string &reg, const std::string &val) {
        asm_out << "    mov " << reg << ", " << val << std::endl;
    }

    void asm_add(const std::string &reg1, const std::string &reg2) {
        asm_out << "    add " << reg1 << ", " << reg2 << std::endl;
    }

    void asm_substract(const std::string &reg1, const std::string &reg2) {
        asm_out << "    sub " << reg1 << ", " << reg2 << std::endl;
    }

    void asm_multiply(const std::string &reg) {
        asm_out << "    mul " << reg << std::endl;
    }

    void asm_divide(const std::string &reg) {
        asm_out << "    xor rdx, rdx\n    div " << reg << std::endl;
    }

    void asm_logical_and(const std::string &reg1, const std::string &reg2) {
        asm_out << "    and " << reg1 << ", " << reg2 << std::endl;
    }

    void asm_logical_or(const std::string &reg1, const std::string &reg2) {
        asm_out << "    or " << reg1 << ", " << reg2 << std::endl;
    }

    void asm_logical_not(const std::string &reg) {
        asm_out << "    not " << reg << std::endl;
    }

    void asm_test(const std::string &reg) {
        asm_out << "    movzx rax, al\n    test " << reg << ", " << reg << std::endl;
    }

    void asm_cmp(const std::string &reg1, const std::string &reg2) {
        asm_out << "    cmp " << reg1 << ", " << reg2 << std::endl;
    }

    void asm_set_equal() {
        asm_out << "    setz al" << std::endl;
    }

    void asm_set_not_equal() {
        asm_out << "    setnz al" << std::endl;
    }

    void asm_set_greater() {
        asm_out << "    setg al" << std::endl;
    }

    void asm_set_less() {
        asm_out << "    setl al" << std::endl;
    }

    void asm_set_greater_equal() {
        asm_out << "    setge al" << std::endl;
    }

    void asm_set_less_equal() {
        asm_out << "    setle al" << std::endl;
    }

    void asm_jump_zero(const std::string &label) {
        asm_out << "    jz " << label << std::endl;
    }

    void asm_jump(const std::string &label) {
        asm_out << "    jmp " << label << std::endl;
    }

    void asm_label(const std::string &label) {
        asm_out << label << ":" << std::endl;
    }

    void asm_print_int() {
        asm_out << "    call _print_int" << std::endl;
    }

    void asm_print_char(const std::string &pointer) {
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
        asm_out << "%include \"printer.asm\"" << std::endl
                << "global _start" << std::endl
                << "_start:" << std::endl;
    }
};
