#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <cctype>
#include <optional>

/**
 * A small struct to store capturing information:
 * - start index in the input
 * - end index in the input
 */
struct CaptureGroup {
    size_t startIndex;
    size_t endIndex;
    bool valid;
};

struct MatchContext {
    const std::string& input;          // entire input
    size_t position;                   // current matching position
    std::vector<std::optional<CaptureGroup>> captures; // store captures, group 0 = entire match if found
    bool ignoreCase;                   // global or local flag if case-insensitive

    // Helper to check if we are out of bounds
    bool atEnd() const { return position >= input.size(); }
    // Helper to peek current character
    char currentChar() const { return atEnd() ? '\0' : input[position]; }
};

/**
 * Base class for all AST nodes. Each node must implement match() to attempt
 * matching from the current MatchContext::position forward.
 * 
 * If matching is successful, we update the context (for example, move position).
 * If not successful, we restore or return false.
 */
class ASTNode {
public:
    virtual ~ASTNode() = default;

    // Attempt to match starting at context.position. 
    // Returns true if match is successful (and moves context.position).
    // Returns false otherwise (and reverts changes to context).
    virtual bool match(MatchContext& ctx) = 0;
};

/**
 * We will implement the various node types:
 *  1) CharacterNode       - matches a single character
 *  2) DotNode             - matches any single character
 *  3) SequenceNode        - matches a list of subpatterns sequentially
 *  4) OrNode              - alternation
 *  5) GroupNode           - capturing group (with optional index)
 *  6) StarNode            - one-or-more repetition of a subpattern
 *  7) CountNode           - matches exactly N subpatterns
 *  8) IgnoreCaseNode      - sets ignore-case mode for subpattern
 *  9) OutputGroupNode     - signals which group index to output
 */

#endif // AST_H
