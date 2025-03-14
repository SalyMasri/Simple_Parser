#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include "Parser.h"
#include "Nodes.h"

/**
 * A small helper that tries to find a match of the given AST anywhere in the input string.
 * If found, returns the position and sets up the captures in the context.
 * If not found, returns npos.
 */
static size_t findMatch(const std::shared_ptr<ASTNode>& ast, const std::string& text,
                        std::vector<std::optional<CaptureGroup>>& captures)
{
    //  search: try from each position in text
    for (size_t start=0; start <= text.size(); ++start) {
        MatchContext ctx { text, start, captures, false };
        // group 0 is entire match
        ctx.captures.resize(1, std::nullopt);

        size_t savedPos = ctx.position;
        if (ast->match(ctx)) {
            // success
            // store entire match as capture group 0
            if (!ctx.captures[0].has_value()) {
                ctx.captures[0] = CaptureGroup{savedPos, ctx.position, true};
            }
            captures = ctx.captures;
            return savedPos;
        }
    }
    return std::string::npos;
}

int main(int argc, char* argv[])
{
    // 1) Read pattern from command-line
    // 2) Read input text from stdin
    // If a match is found, print the entire match or the requested group.
    // If no match, exit with code EXIT_FAILURE (no output).

    if (argc < 2) {
        std::cerr << "Usage: match \"PATTERN\" < input.txt\n";
        return EXIT_FAILURE;
    }

    std::string pattern = argv[1];

    // Read from stdin into a single string
    std::ostringstream buf;
    buf << std::cin.rdbuf();
    std::string input = buf.str();

    // parse the pattern
    int outputGroup = 0; // default: group 0 = entire match
    auto ast = parsePattern(pattern, outputGroup);
    if (!ast) {
        // parse failure, we produce no output, exit failure
        return EXIT_FAILURE;
    }

    // attempt to find a match
    std::vector<std::optional<CaptureGroup>> captures;
    size_t pos = findMatch(ast, input, captures);

    if (pos == std::string::npos) {
        // no match
        return EXIT_FAILURE; // no output
    }

    // we have a match. By default or if \O{0} is used, we print the entire match.
    // if the user gave \O{N}, we attempt to print group N if it exists.
    // then exit success
    if (outputGroup >= (int)captures.size() || !captures[outputGroup].has_value()) {
        // if that group isn't found or index out of range, we do nothing
        // but we consider the match successful => exit success with no output
        // (the assignment states if a match is found we DO produce output. 
        
        return EXIT_SUCCESS;
    }

    CaptureGroup cg = captures[outputGroup].value();
    size_t length = cg.endIndex - cg.startIndex;
    std::string matched = input.substr(cg.startIndex, length);

    // Print the matched substring from the requested group
    std::cout << matched << std::endl;

    // Return success
    return EXIT_SUCCESS;
}
