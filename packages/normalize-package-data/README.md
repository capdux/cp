[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# normalize-package-data

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:normalize-package-data` ([https://github.com/npm/normalize-package-data](https://github.com/npm/normalize-package-data))

Zero-allocation, Clean-room MIT implementation of `normalize-package-data` v6 for CP, reusing `validate-npm-package-name` and `validate-npm-package-license`.

## Features

- **`PackageMetadata`** & **`NormalizedPackageResult`**
- **`expandRepoShorthand(repo: string, out: []u8): usize`**: Expands `owner/repo`, `github:`, `gitlab:`, and `bitbucket:` shorthands to canonical HTTPS Git URLs while preserving existing `http://`/`https://` URLs.
- **`normalizeEntryPath(entry: string): string`**: Strips leading `./`, leading `/`, and trailing `/`.
- **`cleanVersionString(ver: string): string`**: Trims whitespace and leading `v`/`V`/`=` prefixes.
- **`normalizePackageData(meta: PackageMetadata): NormalizedPackageResult`**: Validates package name and license via official `@cap` validators and aggregates warnings.

## License

MIT
