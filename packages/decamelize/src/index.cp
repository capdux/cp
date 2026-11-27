// @cap/decamelize — Convert a camelCase / PascalCase string into a separated lowercase string
// Clean-room MIT CP implementation of npm `decamelize` (120M+/wk, used by yargs / meow / decamelize-keys)
// Zero-heap, single-pass ASCII converter with acronym & consecutive-uppercase preservation support.

import { decamelizeWithOptions, decamelize } from "./core.cp";

/// Convenience helper: converts camelCase/PascalCase to `snake_case`.
pub fn toSnakeCase(input: string, out: []u8): usize {
    return decamelizeWithOptions(input, "_", false, out);
}

/// Convenience helper: converts camelCase/PascalCase to `kebab-case`.
pub fn toKebabCase(input: string, out: []u8): usize {
    return decamelizeWithOptions(input, "-", false, out);
}

export {
    decamelizeWithOptions,
    decamelize,
    toSnakeCase,
    toKebabCase,
};
