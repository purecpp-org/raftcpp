#!/bin/bash

set -e
set -x

if git rev-parse --verify HEAD >/dev/null 2>&1; then
  # Not in a pull request, so compare against parent commit
  base_commit=`git rev-parse --verify HEAD`
  #echo "$(git rev-parse --git-dir)"
  builtin cd "$(git rev-parse --git-dir)"
  ls -l
  builtin cd refs
  ls
  builtin cd remotes
  ls -l
  builtin cd pull
  ls -l
  #git checkout --progress --force refs/heads/master >/dev/null 2>&1
  #origin_commit=`git rev-parse --verify refs/heads/master`
  echo "Running clang-format against parent commit $base_commit, and $origin_commit"
else
  base_commit=4b825dc642cb6eb9a060e54bf8d69288fbee4904
  origin_commit=4b825dc642cb6eb9a060e54bf8d69288fbee4904
  echo "Running clang-format against branch $base_commit, with hash $(git rev-parse "$base_commit")"
fi
output="$(./git-clang-format --binary /usr/bin/clang-format --commit "$base_commit" "$origin_commit" --diff)"
if [ "$output" = "no modified files to format" ] || [ "$output" = "clang-format did not modify any files" ] ; then
  echo "clang-format passed."
  exit 0
else
  echo "clang-format failed:"
  echo "$output"
  exit 1
fi
