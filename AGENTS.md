# Umbra Repository Guide

## Project

Umbra is a long-term Unreal Engine 5 top-down dark action RPG. Core gameplay and reusable systems belong in C++; Blueprints configure assets, presentation, and rapid iteration. JetBrains Rider is the primary IDE, and GitHub is the source-control remote.

The current milestone is the project foundation and prototype validation. Do not expand scope into complete skill, equipment, loot, or save systems unless explicitly requested.

## Layout

- `Source/Umbra/`: the `Umbra` runtime module and gameplay C++.
- `Config/`: project-wide Unreal configuration.
- `Content/`: Unreal assets and maps; `.uasset` and `.umap` are Git LFS files.
- `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`: generated local data; never commit.

Prefer feature-oriented folders under `Source/Umbra/` as the codebase grows (for example `Characters`, `Controllers`, `GameModes`, `Input`, and `AI`). Keep public headers minimal and use forward declarations where practical.

## C++ and Blueprint Boundary

- C++ owns stable rules, base classes, components, interfaces, input handling, and reusable gameplay logic.
- Blueprints own asset references, tunable defaults, animation/VFX/SFX wiring, UI presentation, and prototype composition.
- Expose only deliberate extension points with `BlueprintReadOnly`, `BlueprintCallable`, or `BlueprintImplementableEvent`; avoid putting foundational rules only in a Level Blueprint.

## Naming

- Follow Unreal conventions: `A` actors, `U` UObject types, `F` structs, `E` enums, `I` interfaces, and `b` booleans.
- Use PascalCase for types/functions and descriptive Unreal property names.
- C++ gameplay types use the `Umbra` project prefix where ambiguity is likely.
- Suggested asset prefixes: `BP_`, `WBP_`, `IA_`, `IMC_`, `ABP_`, `M_`, `MI_`, `T_`, `S_`, and `NS_`.

## Build and Validation

- Generate/refresh Rider project files from the `.uproject` after module or target changes.
- Build the `UmbraEditor` target for Win64 Development in Rider or with Unreal's `Build.bat`.
- For gameplay-facing changes, open the project, check the Output Log, and run the smallest relevant PIE smoke test.
- Every change report must state exactly what was validated and what was not validated.

## Git and Asset Safety

- Inspect `git status` before and after work; keep generated directories out of commits.
- Never push, force-push, rewrite history, reset destructively, clean user files, or discard unrelated changes without explicit user approval.
- Do not directly edit, fabricate, or replace `.uasset`/`.umap` binary contents. Asset changes must be made and saved through the matching Unreal Editor version.
- Do not delete user files or migrate assets unless the task explicitly requires it.
- Keep Unreal binary assets under Git LFS and verify LFS status when adding new binary asset types.
