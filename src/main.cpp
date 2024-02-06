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
    const size_t extIndex = filename.find_last_of('.') + 1;
    filename = filename.substr(0, extIndex) + "asm";

    ofstream asm_file(filename);
    asm_file << contents;
    asm_file.close();
}

int main(const int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Wrong usage! Use pppjp <file.pppp>" << endl;
        return 1;
    }

    string content = read_file(argv[1]);

    //Tokenize
    Tokenizer tokenizer(content);
    const vector<Token> tokens = tokenizer.tokenize();


    //Create a parse tree
    Parser parser(tokens);
    const optional<NodeStart> tree = parser.parse_program();

    if (!tree.has_value()) {
        std::cerr << "No exit statement found" << std::endl;
        exit(EXIT_FAILURE);
    }


    //Generate code
    Generator generator((tree.value()));
    const string asm_code = generator.generate_program();


    write_file(argv[1], asm_code);

    system("nasm -felf64 test.asm && ld test.o -o test");

    cout << "Successfully compiled the file!" << endl;


    return 0;
}
