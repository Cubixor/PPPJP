#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "generator.hpp"
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

void write_file(string filename, const string&contents) {
    filename = filename + ".asm";

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

    //Tokenize
    Tokenizer tokenizer(content);
    const vector<Token> tokens = tokenizer.tokenize();


    //Create a parse tree
    Parser parser(tokens);
    const optional<NodeStart> tree = parser.parse_program();


    //Generate code
    Generator generator((tree.value()));
    const string asm_code = generator.generate_program();


    write_file(filename, asm_code);

    string cmd = "nasm -felf64 " + filename + ".asm && ld " + filename + ".o -o "+ filename;
    system(cmd.c_str());

    cout << "[Sukces] Pomyślnie skompilowano plik!" << endl;


    return 0;
}
