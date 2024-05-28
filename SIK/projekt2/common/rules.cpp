#include "rules.h"
#include <sstream>

std::ostream &kierki::operator<<(std::ostream &os, const kierki::Rule &rule) {
    os << static_cast<int>(rule);
    return os;
}

std::istream &kierki::operator>>(std::istream &is, kierki::Rule &rule) {
    char type;
    is.read(&type, 1);
    if (type < '0' || type > '7') {
        rule = Rule::NIL;
        return is;
    }
    rule = static_cast<Rule>(type - '0');
    return is;
}
