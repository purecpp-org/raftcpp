#!/bin/bash

set -e

pull_request=$(echo "$GITHUB_CONTEXT" | jq -r '.event.pull_request')
if [ "$pull_request" != null ]; then
    echo "this is pull request, run git-clang-format"
    origin_commit=$(echo "$GITHUB_CONTEXT" | jq -r '.event.pull_request.base.sha')
    current_commit=$(echo "$GITHUB_CONTEXT" | jq -r '.event.pull_request.head.sha')
    echo "Running clang-format against parent commit $origin_commit, and $current_commit"
else
    echo "this is not pull request, it is already run git-clang-format"
    exit 0
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
