#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

enum class TokenType {
    exit,
    int_lit_num,
    int_lit_mul,
    paren_open,
    paren_close,
    sq_brkt_open,
    sq_brkt_close,
    cur_brkt_close,
    cur_brkt_open,
    backtick,
    var_decl,
    var_type_int,
    var_type_boolean,
    var_ident,
    var_assign,
    add,
    multiply,
    substract,
    divide,
    modulo,
    cond_if,
    cond_else,
    colon,
    loop,
    loop_break,
    loop_continue,
    print,
    bool_true,
    bool_false,
    equal,
    not_equal,
    greater,
    greater_equal,
    less,
    less_equal,
    logical_and,
    logical_or,
    logical_not
};

struct Token {
    TokenType type;
    optional<string> value;
};

const inline set stmt_tokens = {
    TokenType::var_decl, TokenType::exit, TokenType::cur_brkt_open, TokenType::backtick, TokenType::cond_if,
    TokenType::loop, TokenType::loop_break, TokenType::loop_continue, TokenType::print
};
const inline set int_tokens = {TokenType::int_lit_num, TokenType::int_lit_mul};
const inline set term_tokens = {TokenType::sq_brkt_open, TokenType::backtick, TokenType::paren_open, TokenType::bool_true, TokenType::bool_false};
const inline set arithmetic_tokens = {
    TokenType::add, TokenType::substract, TokenType::divide, TokenType::multiply, TokenType::modulo
};
const inline set boolean_tokens = {
    TokenType::equal, TokenType::not_equal, TokenType::greater, TokenType::greater_equal, TokenType::less, TokenType::less_equal
};

inline unordered_map<TokenType, string> token_names = {
    {TokenType::exit, "kończwaść (<expression>)"},
    {TokenType::int_lit_num, "[<integer_literal>]"},
    {TokenType::int_lit_mul, ""},
    {TokenType::paren_open, "("},
    {TokenType::paren_close, ")"},
    {TokenType::sq_brkt_close, "]"},
    {TokenType::sq_brkt_open, "["},
    {TokenType::var_decl, "zmienna"},
    {TokenType::var_type_int, "całkowita"},
    {TokenType::var_type_boolean, "logiczna"},
    {TokenType::var_ident, "<variable_name>"},
    {TokenType::backtick, "`"},
    {TokenType::var_assign, "równa"},
    {TokenType::backtick, "`"},
};

static string get_token_names(const set<TokenType>&expected) {
    string result;

    for (TokenType type: expected) {
        result += token_names[type] + '/';
    }

    result.pop_back();

    return result;
}

static int get_prec(const TokenType type) {
    switch (type) {
        case TokenType::multiply:
        case TokenType::divide:
        case TokenType::modulo:
            return 5;
        case TokenType::add:
        case TokenType::substract:
            return 4;
        case TokenType::equal:
        case TokenType::not_equal:
        case TokenType::greater:
        case TokenType::greater_equal:
        case TokenType::less:
        case TokenType::less_equal:
            return 3;
        case TokenType::logical_and:
            return 2;
        case TokenType::logical_or:
            return 1;
        default:
            assert(false); //Unreachable
    }
}

inline unordered_map<string, int> num_values = {
    {"zero", 0},
    {"jeden", 1},
    {"dwa", 2},
    {"trzy", 3},
    {"cztery", 4},
    {"pięć", 5},
    {"sześć", 6},
    {"siedem", 7},
    {"osiem", 8},
    {"dziewięć", 9},
    {"dziesięć", 10},
    {"jedenaście", 11},
    {"dwanaście", 12},
    {"trzynaście", 13},
    {"czternaście", 14},
    {"piętnaście", 15},
    {"szesnaście", 16},
    {"siedemnaście", 17},
    {"osiemnaście", 18},
    {"dzięwiętnaście", 19},
    {"dwadzieścia", 20},
    {"trzydzieści", 30},
    {"czterdzieści", 40},
    {"pięćdziesiąt", 50},
    {"sześćdziesiąt", 60},
    {"siedemdziesiąt", 70},
    {"osiemdziesiąt", 80},
    {"dziewięćdziesiąt", 90},
    {"sto", 100},
    {"dwieście", 200},
    {"trzysta", 300},
    {"czterysta", 400},
    {"pięćset", 500},
    {"sześćset", 600},
    {"siedemset", 700},
    {"osiemset", 800},
    {"dziewięćset", 900},
    {"tysiąc", 1000},
    {"milion", 1000000},
    {"miliard", 1000000000}
};

inline unordered_map<string, int> multipliers = {
    {"tysiące", 1000},
    {"tysięcy", 1000},
    {"miliony", 1000000},
    {"milionów", 1000000},
    {"miliardy", 1000000000},
    {"miliardów", 1000000000},
};

class Tokenizer {
public:
    explicit Tokenizer(string contents) : contents(move(contents)) {
    }

    vector<Token> tokenize() {
        contents += " ";
        bool comment = false;
        for (int i = 0; i < contents.length(); i++) {
            if ((contents[i] == '\n' && comment) || contents[i] == '#') {
                comment = !comment;
                continue;
            }

            if (comment) {
                continue;
            }

            if (isspace(contents[i])) {
                create_from_buff();
            }
            else if (auto token = try_create_token(contents[i])) {
                create_from_buff();

                tokens.push_back(token.value());
            }
            else {
                buff += contents[i];
            }
        }
        return tokens;
    }

    [[nodiscard]] static optional<Token> try_create_token(const char c) {
        if (c == '(') {
            return Token{TokenType::paren_open, {}};
        }
        if (c == ')') {
            return Token{TokenType::paren_close, {}};
        }
        if (c == '[') {
            return Token{TokenType::sq_brkt_open, {}};
        }
        if (c == ']') {
            return Token{TokenType::sq_brkt_close, {}};
        }
        if (c == '{') {
            return Token{TokenType::cur_brkt_open, {}};
        }
        if (c == '}') {
            return Token{TokenType::cur_brkt_close, {}};
        }
        if (c == '`') {
            return Token{TokenType::backtick, {}};
        }
        if (c == ':') {
            return Token{TokenType::colon, {}};
        }

        return {};
    }

    void create_from_buff() {
        if (buff.empty()) return;

        const Token token = create_token();

        tokens.push_back(token);
        buff.clear();
    }

    [[nodiscard]] Token create_token() const {
        if (buff == "kończwaść") {
            return Token{TokenType::exit, {}};
        }
        if (buff == "zmienna") {
            return Token{TokenType::var_decl, {}};
        }
        if (buff == "całkowita") {
            return Token{TokenType::var_type_int, {}};
        }
        if (buff == "równa") {
            return Token{TokenType::var_assign, {}};
        }
        if (buff == "dodać") {
            return Token{TokenType::add, {}};
        }
        if (buff == "odjąć") {
            return Token{TokenType::substract, {}};
        }
        if (buff == "razy") {
            return Token{TokenType::multiply, {}};
        }
        if (buff == "podzielić") {
            return Token{TokenType::divide, {}};
        }
        if (buff == "modulo") {
            return Token{TokenType::modulo, {}};
        }
        if (buff == "jeśli") {
            return Token{TokenType::cond_if, {}};
        }
        if (buff == "przeciwnie") {
            return Token{TokenType::cond_else, {}};
        }
        if (buff == "powtarzaj") {
            return Token{TokenType::loop, {}};
        }
        if (buff == "przerwij") {
            return Token{TokenType::loop_break, {}};
        }
        if (buff == "kontynuuj") {
            return Token{TokenType::loop_continue, {}};
        }
        if (buff == "wyświetl") {
            return Token{TokenType::print, {}};
        }
        if (buff == "logiczna") {
            return Token{TokenType::var_type_boolean, {}};
        }
        if (buff == "prawda") {
            return Token{TokenType::bool_true, {}};
        }
        if (buff == "fałsz") {
            return Token{TokenType::bool_false, {}};
        }
        if (buff == "równe") {
            return Token{TokenType::equal, {}};
        }
        if (buff == "różne") {
            return Token{TokenType::not_equal, {}};
        }

        if (num_values.contains(buff)) {
            return Token{TokenType::int_lit_num, buff};
        }
        if (multipliers.contains(buff)) {
            return Token{TokenType::int_lit_mul, buff};
        }

        return Token{TokenType::var_ident, buff};
    }

private:
    vector<Token> tokens;
    string buff;
    string contents;
};
