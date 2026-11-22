// Source: Absorbed from /data/dev/bun/src/options_types/BundleEnums.cp | Synced: 2026-11-22T19:37:00Z (BundleFormat, WindowsExeOptions PE metadata & BundlePackageMap zero-alloc policy)
// Pure-CP zero-allocation bundler output format enum, Windows executable PE metadata options, and package bundling rule map.

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Bundler output module format (aligned with Bun `options.Format`).
pub enum BundleFormat: u8 {
    Esm = 0,
    Iife = 1,
    Cjs = 2,
    InternalBakeDev = 3,

    /// Returns true if ES6 `import` / `export` syntax is preserved in emitted output.
    fn keepES6ImportExportSyntax(): bool {
        return _ == BundleFormat.Esm;
    }

    /// Returns true if this format is ECMAScript Modules (`esm`).
    fn isESM(): bool {
        return _ == BundleFormat.Esm;
    }

    /// Returns true if the emitted module is unconditionally evaluated in strict mode.
    fn isAlwaysStrictMode(): bool {
        return _ == BundleFormat.Esm;
    }

    /// Returns true if this format supports multi-chunk code splitting.
    fn supportsCodeSplitting(): bool {
        return _ == BundleFormat.Esm;
    }

    /// Returns true if top-level `await` is natively supported by this output format.
    fn supportsTopLevelAwait(): bool {
        return _ == BundleFormat.Esm;
    }

    /// Returns the canonical lowercase string identifier.
    fn asStr(): []const u8 {
        return switch (_) {
            .Esm => "esm",
            .Iife => "iife",
            .Cjs => "cjs",
            .InternalBakeDev => "internal_bake_dev",
        };
    }

    /// Parses a format string (`"esm"`, `"cjs"`, `"iife"`, `"internal_bake_dev"`), or `null`.
    static fn fromString(raw: []const u8): BundleFormat? {
        if (strEql(raw, "esm")) return BundleFormat.Esm;
        if (strEql(raw, "cjs")) return BundleFormat.Cjs;
        if (strEql(raw, "iife")) return BundleFormat.Iife;
        if (strEql(raw, "internal_bake_dev")) return BundleFormat.InternalBakeDev;
        return null;
    }
}

/// Validates a Windows PE `FILEVERSION`/`PRODUCTVERSION` dotted quad (`A.B.C.D`, each `0..=65535`).
fn isValidWindowsQuadVersion(ver: []const u8): bool {
    if (ver.len == 0) return false;
    let parts: usize = 0;
    let i: usize = 0;
    while (i < ver.len) {
        if (parts >= 4) return false;
        let val: u32 = 0;
        let digits: usize = 0;
        while (i < ver.len && ver[i] != '.') {
            const ch = ver[i];
            if (ch < '0' || ch > '9') return false;
            val = val * 10 + @as(u32, ch - '0');
            if (val > 65535) return false;
            digits += 1;
            i += 1;
        }
        if (digits == 0) return false;
        parts += 1;
        if (i < ver.len) {
            if (ver[i] != '.') return false;
            i += 1;
            if (i == ver.len) return false; // trailing dot
        }
    }
    return parts == 4;
}

/// Windows standalone executable PE metadata & subsystem options (aligned with Bun `WindowsOptions`).
pub struct WindowsExeOptions {
    hide_console: bool = false,
    icon: ([]const u8)? = null,
    title: ([]const u8)? = null,
    publisher: ([]const u8)? = null,
    version: ([]const u8)? = null,
    description: ([]const u8)? = null,
    copyright: ([]const u8)? = null,

    /// Returns true if any PE version-info / icon metadata field is populated.
    fn hasMetadata(): bool {
        return _.icon != null ||
            _.title != null ||
            _.publisher != null ||
            _.version != null ||
            _.description != null ||
            _.copyright != null;
    }

    /// Returns the target PE subsystem identifier (`"WINDOWS_GUI"` when `hide_console` is true, else `"WINDOWS_CUI"`).
    fn subsystemName(): []const u8 {
        if (_.hide_console) return "WINDOWS_GUI";
        return "WINDOWS_CUI";
    }

    /// Returns true if `version` is unset or a valid 4-part Windows version (`A.B.C.D`, `0..=65535`).
    fn isValidVersion(): bool {
        if (v := _.version) {
            return isValidWindowsQuadVersion(v);
        }
        return true;
    }
}

/// Per-package bundling directive (aligned with Bun `BundlePackage`).
pub enum BundlePackageMode: u8 {
    Always = 0,
    Never = 1,

    fn shouldBundle(): bool {
        return _ == BundlePackageMode.Always;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Always => "always",
            .Never => "never",
        };
    }

    static fn fromString(raw: []const u8): BundlePackageMode? {
        if (strEql(raw, "always")) return BundlePackageMode.Always;
        if (strEql(raw, "never")) return BundlePackageMode.Never;
        return null;
    }
}

/// Maximum number of explicit package bundling rules stored in `BundlePackageMap`.
pub const BUNDLE_PACKAGE_MAX_RULES: usize = 64;

/// Error set for `BundlePackageMap`.
pub err BundlePackageError {
    CapacityExceeded,
    EmptyPackageName,
}

/// Single package bundling rule entry.
pub struct BundlePackageRule {
    name: []const u8 = "",
    mode: BundlePackageMode = BundlePackageMode.Always,
}

/// Zero-allocation fixed-capacity package bundling policy map (aligned with Bun `BundlePackage.Map`).
pub struct BundlePackageMap {
    rules: [BUNDLE_PACKAGE_MAX_RULES]BundlePackageRule = [BundlePackageRule{}; BUNDLE_PACKAGE_MAX_RULES],
    len: usize = 0,

    static fn init(): BundlePackageMap {
        return BundlePackageMap{};
    }

    fn count(): usize {
        return _.len;
    }

    /// Inserts or updates a bundling rule for `pkg_name`.
    fn set(*_, pkg_name: []const u8, mode: BundlePackageMode): usize ! BundlePackageError {
        if (pkg_name.len == 0) return BundlePackageError.EmptyPackageName;
        let i: usize = 0;
        while (i < _.len) {
            if (strEql(_.rules[i].name, pkg_name)) {
                _.rules[i].mode = mode;
                return i;
            }
            i += 1;
        }
        if (_.len >= BUNDLE_PACKAGE_MAX_RULES) return BundlePackageError.CapacityExceeded;
        const idx = _.len;
        _.rules[idx] = BundlePackageRule{
            name: pkg_name,
            mode: mode,
        };
        _.len += 1;
        return idx;
    }

    /// Looks up the exact bundling mode for `pkg_name`, or `null` if not configured.
    fn get(pkg_name: []const u8): BundlePackageMode? {
        let i: usize = 0;
        while (i < _.len) {
            if (strEql(_.rules[i].name, pkg_name)) return _.rules[i].mode;
            i += 1;
        }
        return null;
    }

    /// Removes the rule for `pkg_name` if present; returns true if removed.
    fn remove(*_, pkg_name: []const u8): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (strEql(_.rules[i].name, pkg_name)) {
                _.len -= 1;
                if (i < _.len) {
                    _.rules[i] = _.rules[_.len];
                }
                _.rules[_.len] = BundlePackageRule{};
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Evaluates whether `pkg_name` should be bundled, falling back to `default_mode`.
    fn shouldBundle(pkg_name: []const u8, default_mode: BundlePackageMode): bool {
        if (m := _.get(pkg_name)) {
            return m.shouldBundle();
        }
        return default_mode.shouldBundle();
    }
}
