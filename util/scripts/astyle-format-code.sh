#!/bin/bash

# Description: This script changes the style format of
#              all the source code of the project.

# Style and format the source code recursively
astyle --style=kr \
       --indent=spaces=3 \
       --indent-switches --indent-modifiers --indent-classes --indent-cases \
       --align-pointer=type --align-reference=type \
       --attach-return-type-decl --attach-return-type \
       --unpad-paren \
       --indent-preproc-block --indent-preproc-define --indent-col1-comments \
       --remove-brackets \
       --convert-tabs \
       --close-templates \
       --max-code-length=100 \
       --max-instatement-indent=50 \
       --lineend=windows \
       --suffix=none \
       --recursive \
       ../../*.h ../../*.cpp

# End message
echo
echo Code styling complete!
echo
