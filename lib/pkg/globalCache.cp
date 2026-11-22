// Source: Absorbed from /data/dev/bun/src/options_types/GlobalCache.cp | Synced: 2026-11-22T19:33:30Z (Bun GlobalCache auto-install & resolution fallback policy engine)
// Pure-CP zero-allocation package auto-install & global cache resolution policy engine.

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Global cache & auto-install strategy mode (aligned with Bun `options.GlobalCache` / `--install=<mode>`).
pub enum GlobalCacheMode: u8 {
    AllowInstall = 0,
    ReadOnly = 1,
    Auto = 2,
    Force = 3,
    Fallback = 4,
    Disable = 5,

    /// Strictly parses Bun CLI `--install=<mode>` values (`auto`, `force`, `disable`, `fallback`).
    static fn parseCliFlag(raw: []const u8): GlobalCacheMode? {
        if (strEql(raw, "auto")) return GlobalCacheMode.Auto;
        if (strEql(raw, "force")) return GlobalCacheMode.Force;
        if (strEql(raw, "disable")) return GlobalCacheMode.Disable;
        if (strEql(raw, "fallback")) return GlobalCacheMode.Fallback;
        return null;
    }

    /// Parses all `GlobalCacheMode` variants including internal `allow_install` / `read_only`.
    static fn parse(raw: []const u8): GlobalCacheMode? {
        if (m := GlobalCacheMode.parseCliFlag(raw)) return m;
        if (strEql(raw, "allow_install") || strEql(raw, "allow-install")) return GlobalCacheMode.AllowInstall;
        if (strEql(raw, "read_only") || strEql(raw, "read-only")) return GlobalCacheMode.ReadOnly;
        return null;
    }

    /// Returns the canonical lowercase string name.
    fn asStr(): []const u8 {
        return switch (_) {
            .AllowInstall => "allow_install",
            .ReadOnly => "read_only",
            .Auto => "auto",
            .Force => "force",
            .Fallback => "fallback",
            .Disable => "disable",
        };
    }

    /// Returns true if inline version specifiers in import paths (e.g. `lodash@4.17.21`) are permitted.
    fn allowVersionSpecifier(): bool {
        return _ == GlobalCacheMode.Force;
    }

    /// Returns true if the global cache may be consulted given whether a local `node_modules` folder exists.
    fn canUse(has_node_modules_folder: bool): bool {
        if (has_node_modules_folder) {
            return switch (_) {
                .Fallback, .AllowInstall, .Force => true,
                .ReadOnly, .Disable, .Auto => false,
            };
        } else {
            return switch (_) {
                .ReadOnly, .Fallback, .AllowInstall, .Auto, .Force => true,
                .Disable => false,
            };
        }
    }

    /// Returns true unless global cache is explicitly disabled.
    fn isEnabled(): bool {
        return _ != GlobalCacheMode.Disable;
    }

    /// Returns true if missing packages may be automatically installed into the global cache.
    fn canInstall(): bool {
        return switch (_) {
            .Auto, .AllowInstall, .Force, .Fallback => true,
            .ReadOnly, .Disable => false,
        };
    }

    /// Returns true if the global cache is restricted to read-only lookups.
    fn isReadOnly(): bool {
        return _ == GlobalCacheMode.ReadOnly;
    }
}

/// Resolution action produced by `GlobalCachePolicy.evaluate`.
pub enum GlobalCacheAction: u8 {
    UseLocalNodeModules = 0,
    UseGlobalCacheHit = 1,
    InstallToGlobalCache = 2,
    RejectDisabled = 3,
    RejectVersionSpecifier = 4,
}

/// Structured decision output from `GlobalCachePolicy.evaluate`.
pub struct GlobalCacheDecision {
    action: GlobalCacheAction = GlobalCacheAction.RejectDisabled,
    allow_network_install: bool = false,
    allow_version_specifier: bool = false,
    use_global_cache: bool = false,

    fn isResolved(): bool {
        return _.action == GlobalCacheAction.UseLocalNodeModules ||
            _.action == GlobalCacheAction.UseGlobalCacheHit ||
            _.action == GlobalCacheAction.InstallToGlobalCache;
    }
}

/// Zero-allocation global cache & auto-install resolution policy evaluator.
pub struct GlobalCachePolicy {
    mode: GlobalCacheMode = GlobalCacheMode.Auto,
    has_node_modules: bool = false,
    offline: bool = false,

    static fn init(mode: GlobalCacheMode, has_node_modules: bool): GlobalCachePolicy {
        return GlobalCachePolicy{
            mode: mode,
            has_node_modules: has_node_modules,
            offline: false,
        };
    }

    static fn offlinePolicy(mode: GlobalCacheMode, has_node_modules: bool): GlobalCachePolicy {
        return GlobalCachePolicy{
            mode: mode,
            has_node_modules: has_node_modules,
            offline: true,
        };
    }

    /// Evaluates how an import should be resolved against local `node_modules` and the global cache.
    fn evaluate(
        local_found: bool,
        global_cache_hit: bool,
        has_version_specifier: bool,
    ): GlobalCacheDecision {
        const allow_ver = _.mode.allowVersionSpecifier();
        const can_use_global = _.mode.canUse(_.has_node_modules);
        const can_install_now = !_.offline && _.mode.canInstall() && can_use_global;

        if (has_version_specifier && !allow_ver) {
            return GlobalCacheDecision{
                action: GlobalCacheAction.RejectVersionSpecifier,
                allow_network_install: can_install_now,
                allow_version_specifier: false,
                use_global_cache: can_use_global,
            };
        }

        // Force mode always bypasses local node_modules and resolves exclusively through the global cache.
        if (_.mode == GlobalCacheMode.Force) {
            if (global_cache_hit) {
                return GlobalCacheDecision{
                    action: GlobalCacheAction.UseGlobalCacheHit,
                    allow_network_install: can_install_now,
                    allow_version_specifier: allow_ver,
                    use_global_cache: true,
                };
            }
            if (can_install_now) {
                return GlobalCacheDecision{
                    action: GlobalCacheAction.InstallToGlobalCache,
                    allow_network_install: true,
                    allow_version_specifier: allow_ver,
                    use_global_cache: true,
                };
            }
            return GlobalCacheDecision{
                action: GlobalCacheAction.RejectDisabled,
                allow_network_install: false,
                allow_version_specifier: allow_ver,
                use_global_cache: true,
            };
        }

        if (local_found) {
            return GlobalCacheDecision{
                action: GlobalCacheAction.UseLocalNodeModules,
                allow_network_install: can_install_now,
                allow_version_specifier: allow_ver,
                use_global_cache: can_use_global,
            };
        }

        if (!can_use_global) {
            return GlobalCacheDecision{
                action: GlobalCacheAction.RejectDisabled,
                allow_network_install: false,
                allow_version_specifier: allow_ver,
                use_global_cache: false,
            };
        }

        if (global_cache_hit) {
            return GlobalCacheDecision{
                action: GlobalCacheAction.UseGlobalCacheHit,
                allow_network_install: can_install_now,
                allow_version_specifier: allow_ver,
                use_global_cache: true,
            };
        }

        if (can_install_now) {
            return GlobalCacheDecision{
                action: GlobalCacheAction.InstallToGlobalCache,
                allow_network_install: true,
                allow_version_specifier: allow_ver,
                use_global_cache: true,
            };
        }

        return GlobalCacheDecision{
            action: GlobalCacheAction.RejectDisabled,
            allow_network_install: false,
            allow_version_specifier: allow_ver,
            use_global_cache: can_use_global,
        };
    }
}
