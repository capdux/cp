# @cap/semver-diff

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:semver-diff` ([https://github.com/sindresorhus/semver-diff](https://github.com/sindresorhus/semver-diff))

Zero-allocation, Clean-room MIT implementation of `semver-diff` v5 for CP, reusing `@cap/semver-range`.

## Features

- **Diff Kind Constants**: `DIFF_NONE` (`0`), `DIFF_MAJOR` (`1`), `DIFF_MINOR` (`2`), `DIFF_PATCH` (`3`), `DIFF_PREMAJOR` (`4`), `DIFF_PREMINOR` (`5`), `DIFF_PREPATCH` (`6`), `DIFF_PRERELEASE` (`7`), `DIFF_BUILD` (`8`).
- **`semverDiffKind(ver_a: string, ver_b: string): u8`**: Classifies strict upgrades (`ver_b > ver_a`) or build metadata changes when precedence is equal; returns `DIFF_NONE` for equal, downgraded, or invalid versions.
- **`semverDiff(ver_a: string, ver_b: string): string?`**: Returns `"major"`, `"minor"`, `"patch"`, `"premajor"`, `"preminor"`, `"prepatch"`, `"prerelease"`, `"build"`, or `null`.
- **`isBreakingUpgrade(ver_a: string, ver_b: string): bool`**: Detects `major` / `premajor` upgrades as well as `0.x` `minor` / `preminor` upgrades.

## Dependencies

- `@cap/semver-range` (`1.0.0`)

## License

MIT
