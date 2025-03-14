#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <string>
#include <vector>
#include "AST.h"
#include "Nodes.h"

/**
 * Parser interface: given a pattern string, produce an AST.
 * We'll parse the pattern from start to end.
 *
 * The grammar (simplified) might be:
 *
 *   PATTERN        = EXPR [ \O{ number } ]?
 *   EXPR           = TERM { + TERM }*
 *   TERM           = FACTOR { FACTOR }*
 *   FACTOR         = GROUP | ANY | CHAR | FACTOR* | FACTOR{N} | FACTOR\I
 *   GROUP          = ( EXPR ) 
 *   ANY            = '.'
 *   CHAR           = any non-special character
 *   \I             = sets ignore-case mode
 *   {N}            = exact repetition
 *   +              = alternation
 *   \O{ number }   = indicates which capturing group to output
 *
 * (You can refine or alter as needed.)
 */

std::shared_ptr<ASTNode> parsePattern(const std::string& pattern, int& outputGroupIndex);

#endif // PARSER_H
