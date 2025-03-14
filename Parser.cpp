#include "Parser.h"
#include "Nodes.h"
#include <cctype>
#include <iostream>
#include <optional>

// We'll make a small, simple parser with recursive functions to illustrate.

// A lightweight "cursor" struct:
struct Cursor {
    const std::string& text;
    size_t pos;
    Cursor(const std::string& t) : text(t), pos(0) {}
    bool end() const { return pos >= text.size(); }
    char current() const { return end() ? '\0' : text[pos]; }
    void advance() { if(!end()) pos++; }
    void retreat() { if(pos>0) pos--; }
};

// Forward declarations of parse functions
static std::shared_ptr<ASTNode> parseExpr(Cursor& cur);
static std::shared_ptr<ASTNode> parseTerm(Cursor& cur);
static std::shared_ptr<ASTNode> parseFactor(Cursor& cur);
static std::shared_ptr<ASTNode> parseGroup(Cursor& cur);
static std::shared_ptr<ASTNode> parseCount(std::shared_ptr<ASTNode> base, Cursor& cur);
static std::shared_ptr<ASTNode> parseIgnoreCase(std::shared_ptr<ASTNode> base, Cursor& cur);

// Helper to skip whitespace if needed (only if pattern can contain spaces)
static void skipSpaces(Cursor& cur) {
    while (!cur.end() && std::isspace(cur.current())) {
        ++cur.pos;  // Directly advance instead of using cur.advance();
    }
}


static bool matchChar(Cursor& cur, char c) {
   // std::cout << "matchChar() called with: " << c << "\n";

    if (!cur.end() && cur.current() == c) {
        cur.advance();
        return true;
    }
    return false;
}

// parsePattern: top-level entry point
std::shared_ptr<ASTNode> parsePattern(const std::string& pattern, int& outputGroupIndex) {
    Cursor cur(pattern);
    //std::cout << "parsePattern() called\n";

    auto ast = parseExpr(cur);

    // Now see if there's an \O{N} at the end
    // Check if we have "\O{"
    if (!cur.end() && cur.current() == '\\') {
        size_t savedPos = cur.pos;
        cur.advance();
        if(!cur.end() && (cur.current() == 'O' || cur.current() == 'o')) {
            cur.advance();
            if(!cur.end() && cur.current() == '{') {
                // parse a number
                cur.advance();
                // parse digits
                std::string digits;
                while(!cur.end() && std::isdigit(cur.current())) {
                    digits.push_back(cur.current());
                    cur.advance();
                }
                if(!digits.empty() && matchChar(cur, '}')) {
                    // we got \O{###}
                    outputGroupIndex = std::stoi(digits);
                } else {
                    // revert
                    cur.pos = savedPos; 
                }
            } else {
                // revert
                cur.pos = savedPos;
            }
        } else {
            // revert
            cur.pos = savedPos;
        }
    }

    // If the AST is null, might indicate parse failure. 
    // In real code, you'd want robust error handling. 
    return ast;
}

/**
 * parseExpr:
 *   EXPR = TERM { + TERM }*
 *   parse left-associative. E.g. a+b+c -> or(or(a,b),c).
 */
static std::shared_ptr<ASTNode> parseExpr(Cursor& cur) {
    //std::cout << "parseExpr() called\n";

    auto left = parseTerm(cur);
    if(!left) return nullptr;

   // skipSpaces(cur);
    while (matchChar(cur, '+')) {
        // parse the right side
        auto right = parseTerm(cur);
        if(!right) return nullptr;
        // build OrNode
        left = std::make_shared<OrNode>(left, right);
       // skipSpaces(cur);
    }
    return left;
}

/**
 * parseTerm:
 *   TERM = FACTOR { FACTOR }*
 *   This is like "sequence" of factors.
 */

static std::shared_ptr<ASTNode> parseTerm(Cursor& cur) {
   // std::cout << "parseTerm() called\n";

    auto seq = std::make_shared<SequenceNode>();
    while (true) {
        auto f = parseFactor(cur);
        if (!f) {
            // ...
            if (seq->hasChildren()) return seq;
            return nullptr;
        }
        seq->addChild(f);
    }
    // If sequence is empty, return nullptr, else return the node
    // We'll do a small hack to see if it has children. We'll try to dynamic_cast to SequenceNode.
    // A simpler approach: we can store them in a vector and check size.
}




/**
 * parseFactor:
 *   FACTOR = 
 *       ( GROUP ) 
 *     | '.' 
 *     | single character
 *     | FACTOR '*' 
 *     | FACTOR '{N}'
 *     | FACTOR '\I'
 *     ...
 */
static std::shared_ptr<ASTNode> parseFactor(Cursor& cur) {
    //skipSpaces(cur);
    //std::cout << "parseFactor() called\n";

    // Handle group
    if (matchChar(cur, '(')) {
        auto g = parseGroup(cur);
        return g; 
    }

    // Handle dot: '.'
    if (!cur.end() && cur.current() == '.') {
        cur.advance();
        // Start with a DotNode
        std::shared_ptr<ASTNode> base = std::make_shared<DotNode>();

        // See if there's * or {N} right after it
        auto expanded = parseCount(base, cur);
        if (expanded) {
            base = expanded;
        }

        // See if there's \I right after that
        expanded = parseIgnoreCase(base, cur);
        if (expanded) {
            base = expanded;
        }

        return base;
    }

    // Handle single character if it's not a special one
    if (!cur.end() && std::string("+*().{}\\").find(cur.current()) == std::string::npos) {
        char c = cur.current();
        cur.advance();

        // Start with a CharacterNode
        std::shared_ptr<ASTNode> base = std::make_shared<CharacterNode>(c);

        // Check if {N} or * 
        auto expanded = parseCount(base, cur);
        if (expanded) {
            base = expanded;
        }

        // Check if \I 
        expanded = parseIgnoreCase(base, cur);
        if (expanded) {
            base = expanded;
        }

        return base;
    }

    return nullptr;
}



/**
 * parseGroup:
 *   '(' EXPR ')'
 *   We also treat each group as a capturing group. We'll store an incremental index for capturing.
 *   For simplicity, let's store them all in group index 0 for now, or we can do real indexing with
 *   a static counter. A robust approach would require more tracking.
 */
static int groupCounter = 1; // group 0 is entire match




static std::shared_ptr<ASTNode> parseGroup(Cursor& cur) {
    // parse expression inside (...)
   // std::cout << "parseGroup() called\n";

    auto e = parseExpr(cur);
    //skipSpaces(cur);

    // We expect a ')'
    if (!matchChar(cur, ')')) {
        return nullptr;
    }

    // Build a GroupNode capturing into groupCounter
    auto groupNode = std::make_shared<GroupNode>(e, groupCounter++);
    std::shared_ptr<ASTNode> base = groupNode;

    // Possibly parse {N} or * or \I after the group
    auto expanded = parseCount(base, cur);
    if (expanded) {
        base = expanded;
    }
    expanded = parseIgnoreCase(base, cur);
    if (expanded) {
        base = expanded;
    }

    return base;
}

/**
 * parseCount: tries to parse '{N}' after the node or '*'
 */
static std::shared_ptr<ASTNode> parseCount(std::shared_ptr<ASTNode> base, Cursor& cur) {
    //skipSpaces(cur);
    //std::cout << "parseCount() called\n";

    // Check for '*'
    if (matchChar(cur, '*')) {
        // Return a StarNode that wraps 'base'
        return std::make_shared<StarNode>(base);
    }


    // Check for '{N}'
    if (!cur.end() && cur.current() == '{') {
        cur.advance();
        // parse number
        std::string digits;
        while (!cur.end() && std::isdigit(cur.current())) {
            digits.push_back(cur.current());
            cur.advance();
        }
        if (!matchChar(cur, '}')) {
            // parse error or no closing '}', skip or revert if you want robust code
            return nullptr;
        }
        if (digits.empty()) return nullptr;
        int n = std::stoi(digits);
        return std::make_shared<CountNode>(base, n);
    }
    // No expansion
    return nullptr;
}

/**
 * parseIgnoreCase: tries to parse '\I' after the node
 */
static std::shared_ptr<ASTNode> parseIgnoreCase(std::shared_ptr<ASTNode> base, Cursor& cur) {
    //skipSpaces(cur);
    //std::cout << "parseIgnoreCase() called\n";

    if (!cur.end() && cur.current() == '\\') {
        size_t savedPos = cur.pos;
        cur.advance();
        if (!cur.end() && (cur.current() == 'I' || cur.current() == 'i')) {
            cur.advance();
            // create an IgnoreCase node
            return std::make_shared<IgnoreCaseNode>(base);
        } else {
            // revert
            cur.pos = savedPos;
        }
    }
    return nullptr;
}
