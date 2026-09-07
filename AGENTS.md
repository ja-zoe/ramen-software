# Ramen Agent Guide

This repository uses the `spec-driven-dev` skill at
`.agents/skills/spec-driven-dev/SKILL.md`. Read the skill, `changes/CONTEXT.md`,
and `docs/architecture.md` before planning or implementing feature work.
Claude Code loads the same agent resources through the `.claude` symlink. Its
project settings enable the skill's `spec-gate` hooks; if a merge or turn is
blocked, reconcile the revision markers and test checklist instead of bypassing
the gate.

## Development workflow

- Track feature work in `changes/` using the structure and templates defined by
  `spec-driven-dev`.
- Write and review a feature's spec and test scheme before changing source code.
- Use `main` as the single integration branch.
- Create each revision set on `feat/setN-<slug>` from `main`.
- Create each feature on `feat/setN/RN.M-<slug>` from its set branch.
- Update `_set.md` markers in real time: `[ ]` planned, `[~]` in progress,
  `[t]` tests passing, and `[x]` merged.
- A feature may merge into its set branch only after every check in that
  feature's test scheme has actually passed.
- A set may merge into `main` only after all features pass, the application
  boots cleanly, and the user explicitly approves the merge.
- Keep implementation details and deviations in the feature log; keep
  `_set.md` limited to status, open decisions, DB roll-up, and set milestones.

## Repository shape

This repository holds firmware and both web applications together. That is
deliberate; see `docs/architecture.md`.

- `firmware/arduino/` is the currently deployed node firmware and the reference
  for node behavior.
- `web/packages/` holds shared code. `web/apps/` holds the two deployment
  targets.
- The device protocol is the central artifact. Firmware, embedded UI, cloud
  backend, and cloud UI are all consumers of it. Do not invent a
  dashboard-only protocol; reconcile changes across both sides.

## Constraints that are easy to violate by accident

- **Never commit `config.h`.** It holds Wi-Fi and Supabase credentials and is
  gitignored throughout. Only `config.h.example` is tracked.
- **`web/apps/embedded` must not import `web/apps/cloud`**, authentication,
  analytics, or the Supabase client. It ships inside ESP32 flash and its bundle
  budget is a real constraint.
- **`web/packages/ui` must not import `device-client`** or any transport. It
  depends on `device-model` only.
- Build configuration describes immutable capabilities. Runtime configuration
  describes mutable behavior. Do not move something across that line without
  recording the decision in `docs/architecture.md`.

## Project state

Structure and contracts are defined; build tooling and application code are
not. The web workspace has no package manifests yet. Record durable decisions
in `changes/CONTEXT.md` as they are made.
