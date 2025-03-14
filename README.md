# Pattern Matching Parser & Runtime

## Overview
This project implements a custom pattern matching parser and runtime capable of evaluating regular expressions with extended operators. The parser processes a given pattern and matches it against an input text, returning a success or failure based on whether a match is found.

## Technologies & Tools Used
- Programming Language: C++
- Parsing Techniques: Direct Parsing & Parse Tree Representation
- Data Structures: Abstract Syntax Tree (AST)
- Execution Handling: `EXIT_SUCCESS` for matches, `EXIT_FAILURE` for no match

---

## Parsing & Grammar Implementation
Designed a custom grammar supporting:
- Logical OR (`+`) – Matches one of two alternatives  
- Repetition (`*`) – Matches one or more repetitions  
- Grouping (`()`) – Captures subexpressions  
- Wildcard (`.`) – Matches any single character  
- Exact Match (`{N}`) – Matches exactly `N` occurrences  
- Case Insensitivity (`\I`) – Ignores capitalization  
- Output Selection (`\O{}`) – Extracts specific capture groups  

---

## Runtime Execution & Matching
Implemented two approaches:
- Direct Parsing – Matches patterns by applying operations directly  
- Parse Tree Evaluation – Builds an AST and traverses it for evaluation  
- Efficient Backtracking – Handles failed matches by rolling back  
- Performance Optimization – Minimizes redundant evaluations  

---

## Final Results
- Successfully parsed and matched expressions against input text  
- Correctly evaluated patterns using custom parsing logic  
- Returned expected exit codes (`EXIT_SUCCESS` or `EXIT_FAILURE`)  
- Well-structured code with clear documentation and execution examples  

