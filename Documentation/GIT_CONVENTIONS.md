# Git Conventions

Rules for branches, commits, pull requests and releases. Every branch name,
commit message and PR in this repository must follow them.

## 1. Workflow — GitHub Flow

- `main` is always buildable and all tests pass on it.
- Every change is made in a short-lived branch created from the latest `main`.
- A branch is merged into `main` only through a pull request with green CI.
- PRs are merged with **Squash and merge**: one PR = one commit in `main`.
- The branch is deleted after the merge.
- No direct commits to `main` (the only exception was the initial commit).

```
main ──●─────────────●─────────────●──── (tag v0.1.0)
        \           / \           /
         ●──●──●───●   ●──●──●───●
     docs/project-      feat/window-
       foundation        gl-context
```

## 2. Types

One shared list of types is used in branch names, commit messages and PR
titles.

| Type       | Use for                                                      |
|------------|--------------------------------------------------------------|
| `feat`     | New functionality visible to the player or developer         |
| `fix`      | Bug fix                                                      |
| `refactor` | Code change that neither adds a feature nor fixes a bug      |
| `perf`     | Performance improvement                                      |
| `test`     | Adding or changing tests only                                |
| `docs`     | Documentation only                                           |
| `build`    | Build system: CMake, presets, vcpkg                          |
| `ci`       | GitHub Actions workflows                                     |
| `asset`    | Adding or changing assets (models, textures, sounds, maps)   |
| `chore`    | Anything else (configs, cleanup, tooling)                    |

## 3. Scopes

A scope names the module a change touches. It is optional in commit messages
but recommended. New scopes are added here when a new module appears.

| Scope      | Module                                           |
|------------|--------------------------------------------------|
| `core`     | Application loop, time, logging, utilities       |
| `platform` | Window, OpenGL context, input (SDL3)             |
| `renderer` | Everything OpenGL                                |
| `ecs`      | ECS setup, generic components and systems        |
| `game`     | Gameplay: player, weapons, enemies, pickups      |
| `ai`       | Enemy AI and pathfinding                         |
| `physics`  | Movement and collision                           |
| `world`    | Level loading, map format, level compiler        |
| `audio`    | Sound and music                                  |
| `ui`       | HUD, menus, debug overlay                        |
| `save`     | Save and load system                             |
| `config`   | Settings and data-driven configuration           |
| `tests`    | Test infrastructure                              |
| `docs`     | Documentation                                    |

## 4. Branches

**Format:** `<type>/<short-description>`

- Lowercase, words separated by hyphens (kebab-case).
- 2–5 words, describes the goal of the branch.
- No issue numbers, no names, no dates.

**Examples**

```
feat/fly-camera
feat/window-gl-context
fix/mouse-jitter-on-alt-tab
build/cmake-vcpkg-setup
docs/project-foundation
refactor/resource-manager
```

## 5. Commits

**Format** ([Conventional Commits](https://www.conventionalcommits.org)):

```
<type>(<scope>): <summary>

<body>

<footer>
```

**Summary line**

- At most 72 characters, no period at the end.
- Imperative mood, lowercase after the colon: "add", "fix", "remove" —
  not "added" or "adds".
  Test: *"If applied, this commit will **&lt;summary&gt;**"*.
- The scope may be omitted when a change spans many modules.

**Body** (optional for trivial commits, required otherwise)

- Separated from the summary by one blank line.
- Wrapped at 72 characters.
- Explains **what** changed and **why**, not how — the diff shows how.

**Footer** (optional)

- `Refs: #12` — links an issue.
- `Co-Authored-By: Name <email>` — credits a co-author.

**Examples**

```
feat(renderer): add OpenGL debug message callback

Route driver messages from GL_KHR_debug to the logger so that invalid
calls are reported immediately with a readable source and severity
instead of silently producing a black screen.
```

```
fix(platform): keep relative mouse mode after alt-tab

SDL drops relative mode when the window loses focus. Re-enable it on
SDL_EVENT_WINDOW_FOCUS_GAINED while the game is not paused.
```

```
docs: add git conventions
```

## 6. Pull Requests

**Title:** same format as a commit summary line. With squash merge the PR
title becomes the commit in `main`, so it must describe the whole branch.

```
feat(platform): create SDL3 window with OpenGL 4.6 core context
```

**Description:** follow the template in
[`.github/pull_request_template.md`](../.github/pull_request_template.md).
It is filled in automatically when a PR is opened on GitHub.

| Section              | Content                                                    |
|----------------------|------------------------------------------------------------|
| **Summary**          | 1–3 sentences: what this PR does and why                   |
| **Changes**          | Bullet list of the notable changes                         |
| **How to test**      | Steps a reviewer follows to see the change working         |
| **Milestone**        | The milestone the PR belongs to, e.g. `0.1 Foundation`     |
| **Checklist**        | Build, tests, warnings and docs confirmed                  |

**Rules**

- One PR = one logical goal. If the description needs "and also…", split it.
- CI must be green before merging.
- The squash commit message body is the PR's **Summary** + **Changes**.

## 7. Versions and releases

**Scheme:** `MAJOR.MINOR.PATCH`

| Stage               | Versions                  | Meaning                           |
|---------------------|---------------------------|-----------------------------------|
| Development         | `0.1.0` … `0.9.0`         | One minor version per milestone   |
| Release             | `1.0.0`                   | Finished game                     |
| After release       | `1.1.0`, `1.2.0`, …       | New features                      |
|                     | `1.0.1`, `1.0.2`, …       | Bug fixes                         |

- There is **no** `0.10`: after `0.9` comes `1.0`.
- Fixes to a development milestone use a patch number: `0.3.1`.

**Releasing a milestone**

1. All milestone branches are merged into `main` and CI is green.
2. The last branch of the milestone also prepares the release:
   - 1–4 screenshots of the milestone go to
     `Documentation/Screenshots/v<version>/` (for example
     `Documentation/Screenshots/v0.1.0/RotatingCube.png`) and are shown in the
     README: one row per version, the newest version first, every picture
     176×99 with a caption under it and a link to the full-size file;
   - `Documentation/ROADMAP.md` and the README mark the milestone as done.
3. An annotated tag is created on `main`:
   ```
   git tag -a v0.1.0 -m "Milestone 0.1 — Foundation"
   git push origin v0.1.0
   ```
4. The pushed tag starts the **Release** workflow
   (`.github/workflows/Release.yml`). It builds and tests Release, checks that
   the tag matches `project(... VERSION ...)`, packages the game with
   `cmake --install` into `Abomination-<version>-Windows-x64.zip` and creates a
   **draft** GitHub Release with the archive attached.
5. The draft is finished on GitHub (*Releases → Edit*): the title gets the
   milestone name (`Abomination 0.1 — Foundation`), the generated list of pull
   requests is turned into a short changelog, the screenshots are added. Then
   **Publish release**.

A failed workflow is fixed with a normal pull request; the tag is then moved
to the fixed commit (`git tag -d`, `git push --delete origin`, tag again). The
package of any branch can be checked without a release: *Actions → Release →
Run workflow* keeps the archive as a workflow artifact.

Pull requests do not contain screenshots: the pictures of every version are
collected in the repository instead.
