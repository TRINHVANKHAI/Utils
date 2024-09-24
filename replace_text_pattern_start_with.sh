#!/bin/bash

#Words start with 'S' and end with '_t', delete '_t'
#For example: SMyStruct_t is translate to SMyStruct
#Thank ChatGPT gave me the solution

sed -e 's/\bS\w*_t\b/&######/g; s/_t######//g'   <input file>
