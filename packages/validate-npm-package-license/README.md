[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/validate-npm-package-license

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:validate-npm-package-license` ([https://github.com/kemitchell/validate-npm-package-license.js](https://github.com/kemitchell/validate-npm-package-license.js))

Zero-allocation, Clean-room MIT implementation of `validate-npm-package-license` v3 for CP, reusing `isValidSpdxExpression` from `@cap/spdx-expression-parse`.

## Features

- **`LicenseValidation`**: `{ valid_for_new_packages, valid_for_old_packages, spdx, unlicensed, in_file }`.
- **`validateLicense(license: string): LicenseValidation`**:
  - Valid SPDX expressions → `valid_for_new_packages: true`, `valid_for_old_packages: true`, `spdx: true`
  - `"UNLICENSED"` → `valid_for_new_packages: true`, `valid_for_old_packages: true`, `unlicensed: true`
  - `"SEE LICENSE IN <filename>"` → `valid_for_new_packages: true`, `valid_for_old_packages: true`, `in_file: "<filename>"`
  - Legacy values (`"BSD"`, `"MIT/X11"`, `"WTFPL"`) → `valid_for_new_packages: false`, `valid_for_old_packages: true`
  - Invalid or empty strings → `valid_for_new_packages: false`, `valid_for_old_packages: false`
- **`isValidNewLicense(license: string): bool`**: Convenience check for newly published packages.

## License

MIT
