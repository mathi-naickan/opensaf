#!/bin/bash

hgeditor=""
if test -n "$VISUAL" -a -z "$hgeditor"; then
    hgeditor=$(type -p "$VISUAL")
fi
if test -n "$EDITOR" -a -z "$hgeditor"; then
    hgeditor=$(type -p "$EDITOR")
fi
if test -z "$hgeditor"; then
    hgeditor=$(type -p emacs)
fi
if test -z "$hgeditor"; then
    hgeditor=$(type -p vi)
fi
if test -z "$hgeditor"; then
    echo "Cannot find a suitable editor. Please set the environment variable"
    echo "EDITOR"
    exit 1
fi

template_file=$(hg root)/tools/devel/review/commit.template
if test -s "$template_file"; then
    grep -E "^HG:" "$1" >& /dev/null && cp "$template_file" "$1"
    test -s "$1" || cp "$template_file" "$1"
    "$hgeditor" "$1"
    diff -Bw "$template_file" "$1" > /dev/null && exit 1
    sed -i -e "/^#/d" "$1"
else
    "$hgeditor" "$1"
fi
grep '[^ \t]' "$1" > /dev/null
