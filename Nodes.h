#ifndef NODES_H
#define NODES_H

#include "AST.h"
#include <iostream>
#include <algorithm>
#include <cassert>

/**
 * CharacterNode
 * Matches exactly one character c, respecting ignoreCase if ctx.ignoreCase is true.
 */
class CharacterNode : public ASTNode {
public:
    explicit CharacterNode(char c) : ch(c) {}

    bool match(MatchContext& ctx) override {
        //std::cout << "CharacterNode::match() called\n";

        if (ctx.atEnd()) return false;

        char inputChar = ctx.currentChar();
        // If ignoring case, compare lowercased
        if (ctx.ignoreCase) {
            if (std::tolower(inputChar) == std::tolower(ch)) {
                ctx.position++;
                return true;
            }
        } else {
            if (inputChar == ch) {
                ctx.position++;
                return true;
            }
        }
        return false;
    }

private:
    char ch;
};

/**
 * DotNode
 * Matches any single character (unless we are at the end).
 */
class DotNode : public ASTNode {
public:
    bool match(MatchContext& ctx) override {
        //std::cout << "DotNode::match() called\n";

        if (ctx.atEnd()) return false;
        // accept one character
        ctx.position++;
        return true;
    }
};

/**
 * SequenceNode
 * Matches a sequence of child patterns in order (like concatenation).
 */
class SequenceNode : public ASTNode {
public:
    void addChild(std::shared_ptr<ASTNode> node) {
        if (node) children.push_back(node);
    }

    bool hasChildren() const {
        return !children.empty();
    }

    bool match(MatchContext& ctx) override {
        //std::cout << "SequenceNode::match() called\n";

        size_t savedPos = ctx.position;
        for (auto& c : children) {
            if (!c->match(ctx)) {
                ctx.position = savedPos;
                return false;
            }
        }
        return true;
    }

private:
    std::vector<std::shared_ptr<ASTNode>> children;
};


/**
 * OrNode (Alternation)
 *  e.g. subpattern1 + subpattern2
 */
class OrNode : public ASTNode {
public:
    OrNode(std::shared_ptr<ASTNode> left, std::shared_ptr<ASTNode> right)
        : lhs(std::move(left)), rhs(std::move(right)) {}

    bool match(MatchContext& ctx) override {
        //std::cout << "OrNode::match() called\n";

        size_t savedPos = ctx.position;
        // Try LHS
        if (lhs->match(ctx)) {
            return true;
        }
        // revert and try RHS
        ctx.position = savedPos;
        if (rhs->match(ctx)) {
            return true;
        }
        // if both fail
        return false;
    }

private:
    std::shared_ptr<ASTNode> lhs, rhs;
};

/**
 * GroupNode
 *  - uses an internal subpattern.
 *  - if groupIndex >= 0, we store the substring matched in that capture slot.
 */
class GroupNode : public ASTNode {
public:
    explicit GroupNode(std::shared_ptr<ASTNode> subExpr, int index = -1)
        : expr(std::move(subExpr)), groupIndex(index) {}

    bool match(MatchContext& ctx) override {
        //std::cout << "GroupNode::match() called\n";

        size_t startPos = ctx.position;
        if (!expr->match(ctx)) {
            return false;
        }
        size_t endPos = ctx.position;

        // if capturing
        if (groupIndex >= 0) {
            // Make sure we have enough slots in captures
            if (groupIndex >= (int)ctx.captures.size()) {
                ctx.captures.resize(groupIndex + 1, std::nullopt);
            }
            ctx.captures[groupIndex] = CaptureGroup{startPos, endPos, true};
        }
        return true;
    }

private:
    std::shared_ptr<ASTNode> expr;
    int groupIndex; // -1 means no capturing, 0 means entire match, etc.
};

/**
 * StarNode
 *  - pattern*
 *  - matches one or more times the subpattern
 *    (if you need "zero or more", you can adapt as needed or follow the assignment's specs).
 */
class StarNode : public ASTNode {
    public:
        explicit StarNode(std::shared_ptr<ASTNode> subExpr)
            : expr(std::move(subExpr)) {}
    
        bool match(MatchContext& ctx) override {
           // std::cout << "StarNode::match() called\n";
    
            // We want 1+ matches as per the assignment "passes one or more" or adapt for "zero or more"
            int count = 0;
            while(true) {
                size_t savedPos = ctx.position;
                if(!expr->match(ctx)) { // should have child, define greed
                    // revert last if not matched
                    ctx.position = savedPos;
                    break;
                }
                count++;
            }
            // If we require at least 1 repetition, check count:
            return (count >= 1);
        }
    
    private:
        std::shared_ptr<ASTNode> expr;
    };
    


/**
 * CountNode
 *  - pattern{N}
 *  - Matches exactly N occurrences of the subpattern.
 */
class CountNode : public ASTNode {
public:
    CountNode(std::shared_ptr<ASTNode> subExpr, int n)
        : expr(std::move(subExpr)), count(n) {}

    bool match(MatchContext& ctx) override {
        //std::cout << "CountNode::match() called\n";

        size_t savedPos = ctx.position;
        for (int i = 0; i < count; i++) {
            if (!expr->match(ctx)) {
                ctx.position = savedPos;
                return false;
            }
        }
        return true;
    }

private:
    std::shared_ptr<ASTNode> expr;
    int count;
};

/**
 * IgnoreCaseNode
 *  - sets the context ignoreCase to true for the subpattern
 *    restore previous ignoreCase after matching
 */
class IgnoreCaseNode : public ASTNode {
public:
    explicit IgnoreCaseNode(std::shared_ptr<ASTNode> subExpr)
        : expr(std::move(subExpr)) {}

    bool match(MatchContext& ctx) override {
        //std::cout << "IgnoreCaseNode::match() called\n";

        bool savedFlag = ctx.ignoreCase;
        ctx.ignoreCase = true;
        bool ok = expr->match(ctx);
        ctx.ignoreCase = savedFlag;  // restore
        return ok;
    }

private:
    std::shared_ptr<ASTNode> expr;
};

/**
 * OutputGroupNode
 *  - This node doesn't affect matching; it just stores which group to eventually print out.
 *  - We'll handle the logic externally to see which group to output.
 *    In some designs, we might store this in the context.
 */
class OutputGroupNode : public ASTNode {
public:
    explicit OutputGroupNode(int idx) : groupIndex(idx) {}

    bool match(MatchContext& ctx) override {
       // std::cout << "OutputGroupNode::match() called\n";

        // This node by itself doesn't fail the match; it's more like a marker.
        // We'll do nothing special here. Return true so that the sequence can continue.
        return true;
    }

    int getGroupIndex() const { return groupIndex; }

private:
    int groupIndex;
};

#endif // NODES_H
