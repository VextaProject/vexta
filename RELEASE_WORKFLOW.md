# Vexta Release Workflow

## Repositories

- Private development repository:
  `~/Projects/vexta`

  Contains the full development history.
  Push is disabled for all remotes.

- Public release repository:
  `~/vexta-public`

  Contains only clean public snapshots.
  This is the only repository allowed to push to GitHub.

## Future release process

1. Finish and test changes in `~/Projects/vexta`.
2. Create release binaries and checksums.
3. Copy the current tracked source snapshot into `~/vexta-public`.
4. Review the public diff carefully.
5. Commit the public snapshot with a release-focused commit message.
6. Create a version tag such as `v0.1.1`.
7. Push only from `~/vexta-public`.
8. Create the GitHub Release and attach binaries and checksum files.

## Important

Never push directly from `~/Projects/vexta`.
Never push private phase tags or development branches to the public repository.
