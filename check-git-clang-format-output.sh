#!/bin/bash

set -e
set -x

if git rev-parse --verify HEAD >/dev/null 2>&1; then
    origin_commit=$(echo "$GITHUB_CONTEXT" | jq -r '.event.pull_request.base.sha')
    current_commit=$(echo "$GITHUB_CONTEXT" | jq -r '.event.pull_request.head.sha')
    echo "Running clang-format against parent commit $origin_commit, and $current_commit"
else
    base_commit=4b825dc642cb6eb9a060e54bf8d69288fbee4904
    origin_commit=4b825dc642cb6eb9a060e54bf8d69288fbee4904
    echo "Running clang-format against branch $base_commit"
fi

output="$(./git-clang-format --binary clang-format --commit "$current_commit" "$origin_commit" --diff)"
if [ "$output" = "no modified files to format" ] || [ "$output" = "clang-format did not modify any files" ]; then
    echo "clang-format passed."
    exit 0
else
    echo "clang-format failed:"
    echo "$output"
    exit 1
fi
