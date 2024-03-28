#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "asm_generator.hpp"
#include "ir_generator.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"

using namespace std;


string read_file(const string&filename) {
    ifstream code_file(filename);
    stringstream content_stream;
    content_stream << code_file.rdbuf();
    string content = content_stream.str();
    code_file.close();

    return content;
}

void write_file(const string&filename, const string&contents) {
    ofstream asm_file(filename);
    asm_file << contents;
    asm_file.close();
}

int main(const int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "[BŁĄD] Nieprawidłowe użycie! Wpisz: pppjp <plik.pppp>" << endl;
        return 1;
    }

    const string content = read_file(argv[1]);
    string filename = argv[1];
    filename = filename.substr(0, filename.find_last_of('.'));

    cout << "[INFO] Rozpoczęto proces kompilacji pliku '" << argv[1] << "'..." << endl;

    //Tokenize
    auto tokenization_start = chrono::high_resolution_clock::now();

    Tokenizer tokenizer(content);
    const vector<Token> tokens = tokenizer.tokenize();

    auto tokenization_end = chrono::high_resolution_clock::now();
    auto tokenization_time = chrono::duration_cast<chrono::microseconds>(tokenization_end - tokenization_start);
    cout << "   [SUKCES] Pomyślnie stokenizowano kod!  [" << tokenization_time.count() << " μs]" << endl;


    //Create a parse tree
    auto parsing_start = chrono::high_resolution_clock::now();

    Parser parser(tokens);
    const optional<NodeStart> tree = parser.parse_program();

    auto parsing_end = chrono::high_resolution_clock::now();
    auto parsing_time = chrono::duration_cast<chrono::microseconds>(parsing_end - parsing_start);
    cout << "   [SUKCES] Pomyślnie utworzono drzewo parsowania!  [" << parsing_time.count() << " μs]" << endl;


    //Generate intermediate code, while performing semantic analysis
    auto irgen_start = chrono::high_resolution_clock::now();

    IRGenerator ir_generator((tree.value()));
    vector<TACInstruction> instructions = ir_generator.generate_program();

    auto irgen_end = chrono::high_resolution_clock::now();
    auto irgen_time = chrono::duration_cast<chrono::microseconds>(irgen_end - irgen_start);
    cout << "   [SUKCES] Pomyślnie wygenerowano pośrednią reprezentację kodu!  [" << irgen_time.count() << " μs]" <<
            endl;


    write_file(filename + ".ppprw", ir_generator.ir_to_string());


    //Generate assembly code
    auto asmgen_start = chrono::high_resolution_clock::now();

    ASMGenerator asm_generator(instructions);
    string asm_code = asm_generator.generate_program();

    auto asmgen_end = chrono::high_resolution_clock::now();
    auto asmgen_time = chrono::duration_cast<chrono::microseconds>(asmgen_end - asmgen_start);
    cout << "   [SUKCES] Pomyślnie wygenerowano kod assembly!  [" << asmgen_time.count() << " μs]" << endl;

    write_file(filename + ".asm", asm_code);

    string cmd = "nasm -felf64 " + filename + ".asm && ld " + filename + ".o -o " + filename;
    if (int code = system(cmd.c_str()); code != 0) {
        exit(code);
    }

    cout << "[SUKCES] Pomyślnie skompilowano plik '"<<filename<<"'!" << endl;


    return 0;
}
