#include <iostream>
#include <climits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>

using namespace std;

unordered_map<string, int> num_values = {
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

unordered_map<string, int> multipliers = {
    {"tysiące", 1000},
    {"tysięcy", 1000},
    {"miliony", 1000000},
    {"milionów", 1000000},
    {"miliardy", 1000000000},
    {"miliardów", 1000000000},
};

vector<std::string> splitString(const string&str) {
    istringstream iss(str);
    vector<std::string> tokens;
    string token;
    while (iss >> token) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

int digit_count(const int i) {
    return static_cast<int>(log10(static_cast<double>(i))) + 1;
}

int parse_number(const string&number) {
    const vector<string> number_split = splitString(number);

    int result = 0;

    int multiplier = 1;
    int prev_result = INT_MIN;

    for (auto it = number_split.rbegin(); it != number_split.rend(); ++it) {
        if (multipliers.contains(*it)) {
            const int* new_multiplier = &multipliers[*it];
            if (*new_multiplier <= multiplier) {
                return -1;
            }

            multiplier = *new_multiplier;
            continue;
        }

        if (!num_values.contains(*it)) {
            cerr << "Unknown number: ~" << *it << "~" << endl;
            return -1;
        }

        const int curr_result = multiplier * num_values[*it];
        if (digit_count(prev_result) >= digit_count(curr_result)) {
            return -1;
        }
        prev_result = curr_result;
        result += prev_result;
    }
    return result;
}


/*int main() {
    string number_str;
    cout << "Podaj liczbę: ";
    getline(cin, number_str);

    const int result = parse_number(number_str);
    if (result == -1) {
        cerr << "Invalid number: ~" << number_str << "~" << endl;
    }

    cout << result << endl;
    return 0;
}*/
