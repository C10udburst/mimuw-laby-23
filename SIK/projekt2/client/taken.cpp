#include "taken.h"
#include "utils.h"
#include <sstream>

void client::Taken::from_string(const std::string &line) {
    loser = line[line.length() - 2];
    // make stringstream but skip TAKEN and N\r\n from end
    auto ss = std::istringstream {line.substr(sizeof "TAKEN" - 1, line.length() - (sizeof "TAKENS\r\n" - 1))};
    std::string draw_str;
    if (ss.peek() != '1') {// trick number is 1 digit
        char c;
        ss.read(&c, 1);
        draw_str = std::string {c};
    } else {
       draw_str = client::get_trick1(line, "TAKEN");
       ss.ignore(draw_str.length());
    }
    draw = std::stoi(draw_str);
    for (auto &card: table)
        ss >> card;
}
