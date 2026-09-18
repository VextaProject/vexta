# Vexta Release Workflow

## Repositories

### Private development repository

```text
~/Projects/vexta
```

This repository contains the full Vexta development history.

It is used for development, testing, consensus work, and release builds.

**Never push from this repository.**

### Public release repository

```text
~/vexta-public
```

This repository contains the public Vexta source tree and is the **only repository allowed to push to GitHub**.

Official GitHub repository:

https://github.com/VextaProject/Vexta

## Release Process

1. Complete development and testing in `~/Projects/vexta`.

2. Verify that all intended tracked changes are committed.

3. Build and validate the release binaries.

4. Generate SHA-256 checksums for all release packages.

5. Synchronize the public repository from the finalized development commit.

   Example:

```bash
cd ~/vexta-public
git fetch ~/Projects/vexta <release-branch>
git checkout -B <public-release-branch> FETCH_HEAD
```

6. Review the complete public diff and repository status.

7. Update public documentation, including:

   - `README.md`
   - `RELEASE_WORKFLOW.md`
   - release notes
   - other release documentation when required

8. Commit the final public release state.

9. Create the Vexta release tag.

   Vexta release tags use the format:

```text
vexta-vX.Y.Z
```

   Example:

```text
vexta-v0.3.0
```

10. Verify that the release tag points to the intended release commit.

11. Push the release branch and Vexta release tag to GitHub **only from `~/vexta-public`**.

12. Create the GitHub Release.

13. Attach the official release archives and publish their SHA-256 checksums.

14. Verify the published source, tag, release files, and checksums before announcing the release.

## Release Validation

Before publishing a release, verify:

- the public tracked tree contains only intended changes
- the release commit is correct
- the Vexta release tag points to the correct commit
- Linux binaries report the intended Vexta Core version
- Windows binaries report the intended Vexta Core version
- required consensus and proof-of-work tests pass
- release archives contain the expected binaries
- SHA-256 checksums match the final release archives

## Important Rules

Never push directly from:

```text
~/Projects/vexta
```

Never expose private development branches, private tags, temporary build files, or unrelated untracked files through the public repository.

Never use destructive cleanup commands such as `git clean` in the private development repository unless the contents have been reviewed explicitly.

Only push Vexta public releases from:

```text
~/vexta-public
```

Release tags should use the Vexta-specific naming convention:

```text
vexta-vX.Y.Z
```
