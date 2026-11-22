// Source: Absorbed from /data/dev/bun/src/js_parser/ast/ImportScanner.cp | Synced: 2026-11-22T19:14:26Z
// ============================================================================
// lib/js/importTrim.cp — Zero-Alloc ESM / TypeScript Unused Import Trimmer
// ============================================================================
// Implements Bun's `js_parser/ast/ImportScanner.cp` unused-import culling
// policy in 100% pure CP with zero dynamic heap allocation:
// - Always disables & strips macro imports (`with { type: "macro" }`)
// - Always preserves bare side-effect imports (`import "./polyfill"`)
// - Evaluates `importsNotUsedAsValues` (`remove`/`preserve`/`error_mode`),
//   `verbatimModuleSyntax`, bundler/minifier mode, namespace property access
//   liveness, and separate TypeScript vs runtime use counts.

pub const IMPORT_TRIM_MAX_ITEMS: usize = 32;

/// Mirrors `tsconfig.json`'s `importsNotUsedAsValues` setting.
pub enum TsImportsNotUsedAsValues: u8 {
    remove = 0,
    preserve = 1,
    error_mode = 2,
}

/// Configuration controlling whether unused imports are trimmed
/// (1:1 aligned with Bun's `ImportScanner` rationale for Svelte/partial modules vs bundling).
pub struct ImportTrimPolicy {
    is_typescript: bool = true,
    is_bundling: bool = false,
    minify_identifiers: bool = false,
    verbatim_module_syntax: bool = false,
    imports_not_used_as_values: TsImportsNotUsedAsValues = TsImportsNotUsedAsValues.remove,

    static fn tsDefault(): ImportTrimPolicy {
        return ImportTrimPolicy{
            is_typescript: true,
            is_bundling: false,
            minify_identifiers: false,
            verbatim_module_syntax: false,
            imports_not_used_as_values: TsImportsNotUsedAsValues.remove,
        };
    }

    static fn tsBundler(): ImportTrimPolicy {
        return ImportTrimPolicy{
            is_typescript: true,
            is_bundling: true,
            minify_identifiers: false,
            verbatim_module_syntax: false,
            imports_not_used_as_values: TsImportsNotUsedAsValues.remove,
        };
    }

    /// Returns true if unused value/type bindings should be pruned from `import` statements.
    fn shouldTrimUnusedImports(): bool {
        if (!_.is_typescript) return false;
        if (_.verbatim_module_syntax) return false;
        if (!_.is_bundling && !_.minify_identifiers &&
            _.imports_not_used_as_values != TsImportsNotUsedAsValues.remove)
        {
            return false;
        }
        return true;
    }
}

/// Usage metadata for a single default, namespace (`* as ns`), or named import binding.
pub struct ImportBindingUsage {
    name: []const u8 = "",
    runtime_use_count: u32 = 0,
    ts_use_count: u32 = 0,
    has_namespace_prop_access: bool = false,
    is_type_only: bool = false,

    static fn runtimeUsed(name: []const u8, count: u32): ImportBindingUsage {
        return ImportBindingUsage{
            name: name,
            runtime_use_count: count,
            ts_use_count: count,
            has_namespace_prop_access: false,
            is_type_only: false,
        };
    }

    static fn typeOnly(name: []const u8): ImportBindingUsage {
        return ImportBindingUsage{
            name: name,
            runtime_use_count: 0,
            ts_use_count: 1,
            has_namespace_prop_access: false,
            is_type_only: true,
        };
    }

    static fn unused(name: []const u8): ImportBindingUsage {
        return ImportBindingUsage{
            name: name,
            runtime_use_count: 0,
            ts_use_count: 0,
            has_namespace_prop_access: false,
            is_type_only: false,
        };
    }
}

/// Input descriptor for a single ESM/TS `import` statement.
pub struct ImportStmtInput {
    is_macro: bool = false,
    has_default: bool = false,
    default_binding: ImportBindingUsage = ImportBindingUsage{},
    has_star: bool = false,
    star_binding: ImportBindingUsage = ImportBindingUsage{},
    named_items: [IMPORT_TRIM_MAX_ITEMS]ImportBindingUsage = [ImportBindingUsage{}; IMPORT_TRIM_MAX_ITEMS],
    named_count: usize = 0,

    static fn sideEffectOnly(): ImportStmtInput {
        return ImportStmtInput{};
    }

    static fn macroImport(): ImportStmtInput {
        return ImportStmtInput{ is_macro: true };
    }

    fn setDefault(*_, binding: ImportBindingUsage) {
        _.has_default = true;
        _.default_binding = binding;
    }

    fn setStar(*_, binding: ImportBindingUsage) {
        _.has_star = true;
        _.star_binding = binding;
    }

    fn addNamed(*_, binding: ImportBindingUsage): bool {
        if (_.named_count >= IMPORT_TRIM_MAX_ITEMS) return false;
        _.named_items[_.named_count] = binding;
        _.named_count += 1;
        return true;
    }
}

/// Final disposition for an `import` statement after trimming.
pub enum ImportStmtDecision: u8 {
    keep_with_bindings = 0,
    keep_side_effect_only = 1,
    omit_completely = 2,
    disable_macro = 3,
}

/// Zero-allocation result returned by `evaluateImportStmt`.
pub struct ImportTrimResult {
    decision: ImportStmtDecision = ImportStmtDecision.keep_side_effect_only,
    keep_default: bool = false,
    keep_star: bool = false,
    kept_named_indices: [IMPORT_TRIM_MAX_ITEMS]usize = [0; IMPORT_TRIM_MAX_ITEMS],
    kept_named_count: usize = 0,
    removed_binding_count: usize = 0,
    mark_record_unused: bool = false,
    is_unused_in_typescript: bool = true,
}

/// Evaluates an `import` statement against `policy`, pruning unused/type-only bindings
/// and deciding whether to keep bindings, downgrade to a side-effect import, or omit.
pub fn evaluateImportStmt(input: ImportStmtInput, policy: ImportTrimPolicy): ImportTrimResult {
    let res = ImportTrimResult{};

    if (input.is_macro) {
        res.decision = ImportStmtDecision.disable_macro;
        res.mark_record_unused = true;
        return res;
    }

    const had_any_binding = input.has_default || input.has_star || input.named_count > 0;
    if (!had_any_binding) {
        // Bare side-effect import (`import "polyfill"`) is always preserved.
        res.decision = ImportStmtDecision.keep_side_effect_only;
        res.is_unused_in_typescript = false;
        return res;
    }

    const trim = policy.shouldTrimUnusedImports();

    if (input.has_default) {
        const b = input.default_binding;
        if (policy.is_typescript && b.ts_use_count != 0) {
            res.is_unused_in_typescript = false;
        }
        const drop = (policy.is_typescript && b.is_type_only) ||
            (trim && b.runtime_use_count == 0);
        if (drop) {
            res.removed_binding_count += 1;
        } else {
            res.keep_default = true;
        }
    }

    if (input.has_star) {
        const b = input.star_binding;
        if (policy.is_typescript && b.ts_use_count != 0) {
            res.is_unused_in_typescript = false;
        }
        const drop = (policy.is_typescript && b.is_type_only) ||
            (trim && b.runtime_use_count == 0 && !b.has_namespace_prop_access);
        if (drop) {
            res.removed_binding_count += 1;
        } else {
            res.keep_star = true;
        }
    }

    let i: usize = 0;
    while (i < input.named_count) {
        const b = input.named_items[i];
        if (policy.is_typescript && b.ts_use_count != 0) {
            res.is_unused_in_typescript = false;
        }
        const drop = (policy.is_typescript && b.is_type_only) ||
            (trim && b.runtime_use_count == 0);
        if (drop) {
            res.removed_binding_count += 1;
        } else {
            res.kept_named_indices[res.kept_named_count] = i;
            res.kept_named_count += 1;
        }
        i += 1;
    }

    const has_remaining = res.keep_default || res.keep_star || res.kept_named_count > 0;
    if (has_remaining) {
        res.decision = ImportStmtDecision.keep_with_bindings;
        return res;
    }

    // All bindings were culled: decide between omitting vs keeping side-effect import.
    if (policy.is_typescript &&
        !policy.verbatim_module_syntax &&
        policy.imports_not_used_as_values == TsImportsNotUsedAsValues.remove)
    {
        res.decision = ImportStmtDecision.omit_completely;
        res.mark_record_unused = true;
    } else {
        res.decision = ImportStmtDecision.keep_side_effect_only;
    }
    return res;
}
