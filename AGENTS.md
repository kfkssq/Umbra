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
- Prefer clear, direct implementations; avoid unnecessary abstractions and inheritance.
- Read combat values through the existing GAS chain. UI formats results and observes attributes; it must not reimplement combat calculations.
- Separate core rules from tuning parameters. Each parameter must have a documented configuration source and override order.
- Comments explain reasons, units, and boundaries, rather than restating code.
- Keep new features and refactors separate, with changes independently reviewable and reversible.
- Update affected documentation with every feature change and provide relevant validation steps.

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
- Use the engine associated with `Umbra.uproject` (currently 5.8; historical validation used 5.8.2). Do not guess the local install path. Example: `& '<UE_ROOT>\Engine\Build\BatchFiles\Build.bat' UmbraEditor Win64 Development '-Project=<absolute path>\Umbra.uproject' -WaitMutex`.
- Run relevant `Umbra.*` tests in Session Frontend → Automation; exact names, coverage and PIE steps are in [Docs/Progress.md](Docs/Progress.md). Documentation-only work needs link/source checks, not an invented build result.

## Git and Asset Safety

- Inspect `git status` before and after work; keep generated directories out of commits.
- Never push, force-push, rewrite history, reset destructively, clean user files, or discard unrelated changes without explicit user approval.
- Do not directly edit, fabricate, or replace `.uasset`/`.umap` binary contents. Asset changes must be made and saved through the matching Unreal Editor version.
- Do not delete user files or migrate assets unless the task explicitly requires it.
- Keep Unreal binary assets under Git LFS and verify LFS status when adding new binary asset types.
- Move or rename referenced assets through Unreal Editor, maintain references and review redirectors; never move them directly in the filesystem.
- Preserve third-party resource provenance/license information; see [Docs/EditorSetup.md](Docs/EditorSetup.md). Asset filenames alone do not prove Blueprint parents, graphs or references; mark unread/unverified contents “待编辑器确认”.

## Documentation Entry Points

- [Architecture](Docs/Architecture.md): class map, ownership, data flows and maintenance evidence.
- [Editor setup](Docs/EditorSetup.md): configuration sources, Blueprint contracts, units and resource rules.
- [Progress](Docs/Progress.md): implemented, verified, pending verification, known issues and next steps. Historical test results are not current validation.
