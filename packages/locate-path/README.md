# @cap/locate-path

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:locate-path` ([https://github.com/sindresorhus/locate-path](https://github.com/sindresorhus/locate-path))

Zero-allocation, Clean-room MIT implementation of `locate-path` v7 core semantics for CP, reusing `joinDirAndName` from `@cap/find-up`.

## Features

- **Constants**: `ENTRY_ANY = 0`, `ENTRY_FILE = 1`, `ENTRY_DIR = 2`.
- **`LocateOptions`**: `{ cwd, entry_type, allow_symlinks, preserve_order }` with `LocateOptions.defaults(cwd)`.
- **`FsEntry`**: `{ path, kind, is_symlink }`.
- **`locatePathInEntries(candidates, entries, opts, out): usize?`**: Resolves relative/absolute candidates and writes the first matching entry path into `out`.
- **`locateAllPathsInEntries(candidates, entries, opts, matched_indices): usize`**: Collects indices of all matching candidates.
- **`isAbsolutePath(p: string): bool`**: Detects POSIX, Windows drive, and UNC absolute paths.

## License

MIT
