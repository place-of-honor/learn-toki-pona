# Agent instructions

Apply the shared evidence and acceptance guardrails in
`isomorphisms/ai-ci/AGENTS.md`.

Before changing this repository, read its README and repository-local
documentation, inspect the current branch/worktree and nearby active work, and
preserve established architecture, terminology, source/build layout, and
explicit current human corrections.

Keep this file repository-specific. Add local rules as the project develops; do
not copy the shared `ai-ci` rulebook here.

## Business Chinese consumer

`isomorphisms/business-chinese` currently consumes this repository's reusable
language-learning software:
https://github.com/isomorphisms/business-chinese

Keep terminal, Android-interface, game, and other reusable learner development
in this repository for now. Business-Chinese-specific corpus material and
published Business Chinese APKs/binaries belong in that consumer repository.

Do not fork or copy the learner implementation there merely to customize the
language. Factor the learner into a separate standalone repository only after
an explicit decision that it is sufficiently independent to stand on its own.
