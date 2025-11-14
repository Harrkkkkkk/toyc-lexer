#include <bits/stdc++.h>
#include "lexer.h"
#include "parser.h"

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::ostringstream sb;
    sb << std::cin.rdbuf();
    std::string src = sb.str();

    Lexer lexer(src);
    auto toks = lexer.tokenize();

    // if lexer produced unterminated comment (we detect by encountering EOF without closing)
    // Simple heuristic: if any Unknown token representing unclosed comment appears - but our lexer just returns EOF.
    // We'll proceed to parse; parser will detect missing tokens like '}' or ')' etc.

    Parser parser(toks);
    parser.parse();

    if (parser.is_accept()) {
        std::cout << "accept\n";
    } else {
        std::cout << "reject\n";
        for (int ln : parser.get_errors()) {
            std::cout << ln << "\n";
        }
    }
    return 0;
}

