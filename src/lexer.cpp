// src/lexer.cpp
// Hand-written lexer for ToyC as specified.
// Compile: g++ -std=c++17 -O2 -o toyc_lexer lexer.cpp

#include <bits/stdc++.h>
using namespace std;

enum TokenKind { TK_KEYWORD, TK_IDENT, TK_INTCONST, TK_SYMBOL };

struct Token {
    TokenKind kind;
    string typ;   // textual type for output (e.g. "'int'" or "Ident" or "IntConst")
    string text;  // content string
};

static const unordered_set<string> keywords = {
    "int","void","if","else","while","break","continue","return"
};

string input;
size_t posi = 0, len_input = 0;

// helper
bool is_alpha(char c){ return (c=='_') || ('a'<=c && c<='z') || ('A'<=c && c<='Z'); }
bool is_digit(char c){ return '0'<=c && c<='9'; }
bool is_alnum(char c){ return is_alpha(c) || is_digit(c); }

char peek_char(size_t offset=0){
    size_t p = posi + offset;
    if(p < len_input) return input[p];
    return '\0';
}
char get_char(){
    if(posi < len_input) return input[posi++];
    return '\0';
}

// skip whitespace & comments
void skip_space_and_comments(){
    while(true){
        // whitespace
        while(posi < len_input &&
             (input[posi]==' ' || input[posi]=='\t' || input[posi]=='\n' || input[posi]=='\r'))
            posi++;

        // line comment
        if(posi+1 < len_input && input[posi]=='/' && input[posi+1]=='/'){
            posi += 2;
            while(posi < len_input && input[posi] != '\n') posi++;
            continue;
        }

        // block comment
        if(posi+1 < len_input && input[posi]=='/' && input[posi+1]=='*'){
            posi += 2;
            while(posi+1 < len_input){
                if(input[posi]=='*' && input[posi+1]=='/'){
                    posi += 2;
                    break;
                }
                posi++;
            }
            continue;
        }

        break;
    }
}

// identifier / keyword
Token read_ident_or_keyword(){
    size_t start = posi;
    get_char();
    while(is_alnum(peek_char())) get_char();
    string s = input.substr(start, posi - start);

    Token tk;
    if(keywords.count(s)){
        tk.kind = TK_KEYWORD;
        tk.typ  = "'" + s + "'";
        tk.text = s;
    } else {
        tk.kind = TK_IDENT;
        tk.typ  = "Ident";
        tk.text = s;
    }
    return tk;
}

// read unsigned integer constant (no leading sign)
Token read_number(){
    size_t start = posi;
    while(is_digit(peek_char())) get_char();
    Token tk;
    tk.kind = TK_INTCONST;
    tk.typ  = "IntConst";
    tk.text = input.substr(start, posi - start);
    return tk;
}

bool starts_with(const string &s){
    return posi + s.size() <= len_input &&
           input.compare(posi, s.size(), s) == 0;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    {
        ostringstream ss;
        ss << cin.rdbuf();
        input = ss.str();
    }
    len_input = input.size();

    vector<Token> tokens;

    while(true){
        skip_space_and_comments();
        if(posi >= len_input) break;

        char c = peek_char();

        // identifier / keyword
        if(is_alpha(c)){
            tokens.push_back(read_ident_or_keyword());
            continue;
        }

        // number (unsigned; '-' is tokenized separately)
        if(is_digit(c)){
            tokens.push_back(read_number());
            continue;
        }

        // multi-character operators (must check before single-char)
        static const vector<string> multi_ops = {
            "<=" , ">=" , "==" , "!=" , "&&" , "||"
        };
        bool matched = false;
        for(const auto &op : multi_ops){
            if(starts_with(op)){
                tokens.push_back({ TK_SYMBOL, "'" + op + "'", op });
                posi += op.size();
                matched = true;
                break;
            }
        }
        if(matched) continue;

        // one-character operators / symbols
        // include '!' as single-character logical not
        static const string one_ops = "(){},;+-*/%<>!=|&";
        char ch = get_char();

        string s(1, ch);
        if(one_ops.find(ch) != string::npos){
            // Note: for '|' and '&' single char appearing alone are allowed tokens,
            // but '||' and '&&' were matched above and won't reach here.
            tokens.push_back({ TK_SYMBOL, "'" + s + "'", s });
        } else {
            // unknown char, still output as symbol (helps debugging)
            tokens.push_back({ TK_SYMBOL, "'" + s + "'", s });
        }
    }

    // output
    for(size_t i=0;i<tokens.size();i++){
        const auto &t = tokens[i];
        cout << i << ":" << t.typ << ":\"" << t.text << "\"\n";
    }

    return 0;
}
