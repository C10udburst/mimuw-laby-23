#include "trick.h"
#include "utils.h"
#include <sstream>
#include "../common/utils.h"

void client::Trick::from_string(const std::string &line) {
    auto ss2 = utils::parseline(line, "TRICK");
    std::string draw_str;
    if (ss2.peek() != '1') {// trick number is 1 digit
        char c;
        ss2.read(&c, 1);
        draw_str = std::string {c};
    } else {
       draw_str = client::get_trick1(line, "TRICK");
       ss2.ignore(draw_str.length());
    }
    draw = std::stoi(draw_str);
    for (auto &card: table)
        ss2 >> card;
}
