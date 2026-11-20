// Source: cp/lib/feature/flags.cp (Pure CP Feature Flags, Consistent Bucketing & Experimentation Engine)
// Pure CP zero-external-dependency Feature Flags, deterministic FNV-1a consistent bucketing,
// attribute targeting rule engine, and multi-variant A/B/n experimentation engine.

const std = @import("std");

pub const BUCKET_SCALE: u32 = 10000;

pub err FeatureError {
    FlagNotFound,
    InvalidPercentage,
    InvalidWeights,
    DuplicateFlag,
    OutOfMemory,
}

pub enum MatchOp {
    Equals,
    NotEquals,
    Contains,
    StartsWith,
    InList,
    NumericGt,
    NumericLt,
}

pub enum EvalReason {
    StaticDisabled,
    KillSwitch,
    RuleMatch,
    RolloutIncluded,
    RolloutExcluded,
    DefaultFallback,
    FlagNotFound,
}

pub enum FlagKind {
    Bool,
    Experiment,
}

pub struct EvalContext {
    alloc: std.mem.Allocator;
    user_id: []u8;
    attrs: std.StringHashMapUnmanaged([]u8) = .empty;

    pub fn init(alloc: std.mem.Allocator, user_id: string): EvalContext ! FeatureError {
        const uid_copy = alloc.dupe(u8, user_id) catch return FeatureError.OutOfMemory;
        return EvalContext{
            alloc: alloc,
            user_id: uid_copy,
            attrs: .empty,
        };
    }

    pub fn deinit(*_) {
        let it = _.attrs.iterator();
        while (entry := it.next()) {
            _.alloc.free(entry.key_ptr.*);
            _.alloc.free(entry.value_ptr.*);
        }
        _.attrs.deinit(_.alloc);
        _.alloc.free(_.user_id);
    }

    pub fn setUserId(*_, user_id: string): void ! FeatureError {
        const uid_copy = _.alloc.dupe(u8, user_id) catch return FeatureError.OutOfMemory;
        _.alloc.free(_.user_id);
        _.user_id = uid_copy;
    }

    pub fn setAttr(*_, key: string, val: string): void ! FeatureError {
        if (entry := _.attrs.getEntry(key)) {
            const val_copy = _.alloc.dupe(u8, val) catch return FeatureError.OutOfMemory;
            _.alloc.free(entry.value_ptr.*);
            entry.value_ptr.* = val_copy;
        } else {
            const key_copy = _.alloc.dupe(u8, key) catch return FeatureError.OutOfMemory;
            errdefer _.alloc.free(key_copy);
            const val_copy = _.alloc.dupe(u8, val) catch return FeatureError.OutOfMemory;
            errdefer _.alloc.free(val_copy);
            _.attrs.put(_.alloc, key_copy, val_copy) catch return FeatureError.OutOfMemory;
        }
    }

    pub fn getAttr(key: string): string? {
        if (v := _.attrs.get(key)) {
            return v;
        }
        return null;
    }
}

pub struct TargetingRule {
    attr_key: string;
    op: MatchOp;
    value: string;
    enabled_result: bool = true;
    variant_result: string = "";
}

struct OwnedTargetingRule {
    attr_key: []u8;
    op: MatchOp;
    value: []u8;
    enabled_result: bool;
    variant_result: []u8;

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.attr_key);
        alloc.free(_.value);
        if (_.variant_result.len > 0) {
            alloc.free(_.variant_result);
        }
    }
}

pub struct VariantWeight {
    name: string;
    weight_bps: u32;
}

struct OwnedVariantWeight {
    name: []u8;
    weight_bps: u32;

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.name);
    }
}

pub struct FeatureDecision {
    flag_key: string;
    enabled: bool;
    variant: string;
    reason: EvalReason;
    bucket: u32;
}

struct FlagEntry {
    key: []u8;
    kind: FlagKind;
    default_enabled: bool;
    rollout_pct: u32;
    killed: bool = false;
    default_variant: []u8;
    rules: std.ArrayListUnmanaged(OwnedTargetingRule) = .empty;
    variants: std.ArrayListUnmanaged(OwnedVariantWeight) = .empty;

    fn deinit(*_, alloc: std.mem.Allocator) {
        for (*r of _.rules.items) {
            r.deinit(alloc);
        }
        _.rules.deinit(alloc);
        for (*v of _.variants.items) {
            v.deinit(alloc);
        }
        _.variants.deinit(alloc);
        if (_.default_variant.len > 0) {
            alloc.free(_.default_variant);
        }
        alloc.free(_.key);
    }
}

/// Deterministic 64-bit FNV-1a bucketing over `flag_key + ":" + user_id` mapped to `0..9999`.
/// Guarantees monotonicity: increasing rollout percentage strictly retains all previously included users.
pub fn consistentBucket(flag_key: string, user_id: string): u32 {
    const FNV1A_64_OFFSET: u64 = (u64(0xcbf29ce4) << 32) | u64(0x84222325);
    const FNV1A_64_PRIME: u64 = 0x100000001b3;
    let h: u64 = FNV1A_64_OFFSET;

    for (b of flag_key) {
        h ^= u64(b);
        h = h *% FNV1A_64_PRIME;
    }
    h ^= u64(':');
    h = h *% FNV1A_64_PRIME;
    for (b of user_id) {
        h ^= u64(b);
        h = h *% FNV1A_64_PRIME;
    }

    return u32(h % u64(BUCKET_SCALE));
}

fn matchRule(rule: OwnedTargetingRule, ctx: EvalContext): bool {
    let attr_val: string = "";
    if (v := ctx.getAttr(rule.attr_key)) {
        attr_val = v;
    } else {
        return false;
    }

    switch (rule.op) {
        .Equals => return std.mem.eql(u8, attr_val, rule.value),
        .NotEquals => return !std.mem.eql(u8, attr_val, rule.value),
        .Contains => return std.mem.indexOf(u8, attr_val, rule.value) != null,
        .StartsWith => return std.mem.startsWith(u8, attr_val, rule.value),
        .InList => {
            let it = std.mem.splitScalar(u8, rule.value, ',');
            while (token := it.next()) {
                const trimmed = std.mem.trim(u8, token, " \t");
                if (std.mem.eql(u8, attr_val, trimmed)) {
                    return true;
                }
            }
            return false;
        },
        .NumericGt => {
            const lhs = std.fmt.parseFloat(f64, attr_val) catch return false;
            const rhs = std.fmt.parseFloat(f64, rule.value) catch return false;
            return lhs > rhs;
        },
        .NumericLt => {
            const lhs = std.fmt.parseFloat(f64, attr_val) catch return false;
            const rhs = std.fmt.parseFloat(f64, rule.value) catch return false;
            return lhs < rhs;
        },
    }
}

fn appendJsonEscaped(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, s: string): void ! FeatureError {
    buf.append(alloc, '"') catch return FeatureError.OutOfMemory;
    for (ch of s) {
        switch (ch) {
            '"' => buf.appendSlice(alloc, "\\\"") catch return FeatureError.OutOfMemory,
            '\\' => buf.appendSlice(alloc, "\\\\") catch return FeatureError.OutOfMemory,
            '\n' => buf.appendSlice(alloc, "\\n") catch return FeatureError.OutOfMemory,
            '\r' => buf.appendSlice(alloc, "\\r") catch return FeatureError.OutOfMemory,
            '\t' => buf.appendSlice(alloc, "\\t") catch return FeatureError.OutOfMemory,
            else => {
                if (ch < 0x20) {
                    buf.append(alloc, ' ') catch return FeatureError.OutOfMemory;
                } else {
                    buf.append(alloc, ch) catch return FeatureError.OutOfMemory;
                }
            },
        }
    }
    buf.append(alloc, '"') catch return FeatureError.OutOfMemory;
}

fn appendUint(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, val: u64): void ! FeatureError {
    if (val == 0) {
        buf.append(alloc, '0') catch return FeatureError.OutOfMemory;
    } else {
        let tmp: [24]u8 = [0; 24];
        let n: usize = 0;
        let v: u64 = val;
        while (v > 0) {
            tmp[n] = u8('0' + (v % 10));
            v /= 10;
            n += 1;
        }
        while (n > 0) {
            n -= 1;
            buf.append(alloc, tmp[n]) catch return FeatureError.OutOfMemory;
        }
    }
}

pub struct FeatureEngine {
    alloc: std.mem.Allocator;
    flags: std.ArrayListUnmanaged(FlagEntry) = .empty;
    index_map: std.StringHashMapUnmanaged(usize) = .empty;

    pub fn init(alloc: std.mem.Allocator): FeatureEngine {
        return FeatureEngine{
            alloc: alloc,
            flags: .empty,
            index_map: .empty,
        };
    }

    pub fn deinit(*_) {
        for (*f of _.flags.items) {
            f.deinit(_.alloc);
        }
        _.flags.deinit(_.alloc);
        _.index_map.deinit(_.alloc);
    }

    pub fn defineBoolFlag(*_, key: string, default_enabled: bool, rollout_pct: u32): void ! FeatureError {
        if (key.len == 0) return FeatureError.FlagNotFound;
        if (rollout_pct > 100) return FeatureError.InvalidPercentage;
        if (_.index_map.contains(key)) return FeatureError.DuplicateFlag;

        const key_copy = _.alloc.dupe(u8, key) catch return FeatureError.OutOfMemory;
        errdefer _.alloc.free(key_copy);

        const idx = _.flags.items.len;
        _.flags.append(_.alloc, FlagEntry{
            key: key_copy,
            kind: .Bool,
            default_enabled: default_enabled,
            rollout_pct: rollout_pct,
            killed: false,
            default_variant: "",
            rules: .empty,
            variants: .empty,
        }) catch return FeatureError.OutOfMemory;

        _.index_map.put(_.alloc, key_copy, idx) catch {
            let popped = _.flags.pop();
            if (*p := popped) {
                p.deinit(_.alloc);
            }
            return FeatureError.OutOfMemory;
        };
    }

    pub fn defineExperiment(*_, key: string, variants: []const VariantWeight, default_variant: string): void ! FeatureError {
        if (key.len == 0) return FeatureError.FlagNotFound;
        if (variants.len == 0) return FeatureError.InvalidWeights;
        if (_.index_map.contains(key)) return FeatureError.DuplicateFlag;

        let total_bps: u64 = 0;
        for (v of variants) {
            if (v.name.len == 0) return FeatureError.InvalidWeights;
            total_bps += u64(v.weight_bps);
        }
        if (total_bps != u64(BUCKET_SCALE)) return FeatureError.InvalidWeights;

        const key_copy = _.alloc.dupe(u8, key) catch return FeatureError.OutOfMemory;
        errdefer _.alloc.free(key_copy);

        let def_var_copy: []u8 = "";
        if (default_variant.len > 0) {
            def_var_copy = _.alloc.dupe(u8, default_variant) catch return FeatureError.OutOfMemory;
        }
        errdefer {
            if (def_var_copy.len > 0) _.alloc.free(def_var_copy);
        }

        let owned_variants: std.ArrayListUnmanaged(OwnedVariantWeight) = .empty;
        errdefer {
            for (*ov of owned_variants.items) {
                ov.deinit(_.alloc);
            }
            owned_variants.deinit(_.alloc);
        }

        for (v of variants) {
            const name_copy = _.alloc.dupe(u8, v.name) catch return FeatureError.OutOfMemory;
            owned_variants.append(_.alloc, OwnedVariantWeight{
                name: name_copy,
                weight_bps: v.weight_bps,
            }) catch {
                _.alloc.free(name_copy);
                return FeatureError.OutOfMemory;
            };
        }

        const idx = _.flags.items.len;
        _.flags.append(_.alloc, FlagEntry{
            key: key_copy,
            kind: .Experiment,
            default_enabled: true,
            rollout_pct: 100,
            killed: false,
            default_variant: def_var_copy,
            rules: .empty,
            variants: owned_variants,
        }) catch return FeatureError.OutOfMemory;

        _.index_map.put(_.alloc, key_copy, idx) catch {
            let popped = _.flags.pop();
            if (*p := popped) {
                p.deinit(_.alloc);
            }
            return FeatureError.OutOfMemory;
        };
    }

    pub fn addRule(*_, flag_key: string, rule: TargetingRule): void ! FeatureError {
        const idx_opt = _.index_map.get(flag_key);
        if (idx_opt == null) return FeatureError.FlagNotFound;
        const idx = idx_opt.?;

        const attr_copy = _.alloc.dupe(u8, rule.attr_key) catch return FeatureError.OutOfMemory;
        errdefer _.alloc.free(attr_copy);
        const val_copy = _.alloc.dupe(u8, rule.value) catch return FeatureError.OutOfMemory;
        errdefer _.alloc.free(val_copy);

        let var_copy: []u8 = "";
        if (rule.variant_result.len > 0) {
            var_copy = _.alloc.dupe(u8, rule.variant_result) catch return FeatureError.OutOfMemory;
        }
        errdefer {
            if (var_copy.len > 0) _.alloc.free(var_copy);
        }

        _.flags.items[idx].rules.append(_.alloc, OwnedTargetingRule{
            attr_key: attr_copy,
            op: rule.op,
            value: val_copy,
            enabled_result: rule.enabled_result,
            variant_result: var_copy,
        }) catch return FeatureError.OutOfMemory;
    }

    pub fn setKillSwitch(*_, flag_key: string, killed: bool): void ! FeatureError {
        const idx_opt = _.index_map.get(flag_key);
        if (idx_opt == null) return FeatureError.FlagNotFound;
        const idx = idx_opt.?;
        _.flags.items[idx].killed = killed;
    }

    pub fn setRollout(*_, flag_key: string, pct: u32): void ! FeatureError {
        if (pct > 100) return FeatureError.InvalidPercentage;
        const idx_opt = _.index_map.get(flag_key);
        if (idx_opt == null) return FeatureError.FlagNotFound;
        const idx = idx_opt.?;
        _.flags.items[idx].rollout_pct = pct;
    }

    pub fn evaluateBool(flag_key: string, ctx: EvalContext): FeatureDecision {
        return _.evaluateBoolWithDefault(flag_key, ctx, false);
    }

    pub fn evaluateBoolWithDefault(flag_key: string, ctx: EvalContext, default_val: bool): FeatureDecision {
        const idx = _.index_map.get(flag_key) ?? {
            return FeatureDecision{
                flag_key: flag_key,
                enabled: default_val,
                variant: "",
                reason: .FlagNotFound,
                bucket: 0,
            };
        };

        const flag = _.flags.items[idx];
        const bucket = consistentBucket(flag.key, ctx.user_id);

        if (flag.killed) {
            return FeatureDecision{
                flag_key: flag.key,
                enabled: false,
                variant: flag.default_variant,
                reason: .KillSwitch,
                bucket: bucket,
            };
        }

        for (rule of flag.rules.items) {
            if (matchRule(rule, ctx)) {
                return FeatureDecision{
                    flag_key: flag.key,
                    enabled: rule.enabled_result,
                    variant: if (rule.variant_result.len > 0) rule.variant_result else flag.default_variant,
                    reason: .RuleMatch,
                    bucket: bucket,
                };
            }
        }

        if (flag.rollout_pct == 0) {
            if (!flag.default_enabled) {
                return FeatureDecision{
                    flag_key: flag.key,
                    enabled: false,
                    variant: flag.default_variant,
                    reason: .StaticDisabled,
                    bucket: bucket,
                };
            }
            return FeatureDecision{
                flag_key: flag.key,
                enabled: true,
                variant: flag.default_variant,
                reason: .DefaultFallback,
                bucket: bucket,
            };
        }

        const threshold: u32 = flag.rollout_pct * 100;
        if (bucket < threshold) {
            return FeatureDecision{
                flag_key: flag.key,
                enabled: true,
                variant: flag.default_variant,
                reason: .RolloutIncluded,
                bucket: bucket,
            };
        }

        return FeatureDecision{
            flag_key: flag.key,
            enabled: false,
            variant: flag.default_variant,
            reason: .RolloutExcluded,
            bucket: bucket,
        };
    }

    pub fn evaluateVariant(flag_key: string, ctx: EvalContext): FeatureDecision {
        return _.evaluateVariantWithDefault(flag_key, ctx, "");
    }

    pub fn evaluateVariantWithDefault(flag_key: string, ctx: EvalContext, default_variant: string): FeatureDecision {
        const idx = _.index_map.get(flag_key) ?? {
            return FeatureDecision{
                flag_key: flag_key,
                enabled: false,
                variant: default_variant,
                reason: .FlagNotFound,
                bucket: 0,
            };
        };

        const flag = _.flags.items[idx];
        const bucket = consistentBucket(flag.key, ctx.user_id);

        if (flag.killed) {
            return FeatureDecision{
                flag_key: flag.key,
                enabled: false,
                variant: flag.default_variant,
                reason: .KillSwitch,
                bucket: bucket,
            };
        }

        for (rule of flag.rules.items) {
            if (matchRule(rule, ctx)) {
                return FeatureDecision{
                    flag_key: flag.key,
                    enabled: rule.enabled_result,
                    variant: if (rule.variant_result.len > 0) rule.variant_result else flag.default_variant,
                    reason: .RuleMatch,
                    bucket: bucket,
                };
            }
        }

        if (flag.variants.items.len > 0) {
            let cumulative: u32 = 0;
            for (v of flag.variants.items) {
                cumulative += v.weight_bps;
                if (bucket < cumulative) {
                    return FeatureDecision{
                        flag_key: flag.key,
                        enabled: true,
                        variant: v.name,
                        reason: .RolloutIncluded,
                        bucket: bucket,
                    };
                }
            }
        }

        return FeatureDecision{
            flag_key: flag.key,
            enabled: flag.default_enabled,
            variant: flag.default_variant,
            reason: .DefaultFallback,
            bucket: bucket,
        };
    }

    pub fn flagCount(): usize {
        return _.flags.items.len;
    }

    pub fn toJsonSnapshot(): ([]u8) ! FeatureError {
        let buf: std.ArrayListUnmanaged(u8) = .empty;
        errdefer buf.deinit(_.alloc);

        buf.appendSlice(_.alloc, "{\"flags\":[") catch return FeatureError.OutOfMemory;

        let i: usize = 0;
        while (i < _.flags.items.len) : (i += 1) {
            if (i > 0) {
                buf.append(_.alloc, ',') catch return FeatureError.OutOfMemory;
            }
            const f = _.flags.items[i];
            buf.appendSlice(_.alloc, "{\"key\":") catch return FeatureError.OutOfMemory;
            appendJsonEscaped(*buf, _.alloc, f.key)?;

            const kind_str: string = if (f.kind == .Bool) "bool" else "experiment";
            buf.appendSlice(_.alloc, ",\"kind\":\"") catch return FeatureError.OutOfMemory;
            buf.appendSlice(_.alloc, kind_str) catch return FeatureError.OutOfMemory;
            buf.appendSlice(_.alloc, "\",\"default_enabled\":") catch return FeatureError.OutOfMemory;
            buf.appendSlice(_.alloc, if (f.default_enabled) "true" else "false") catch return FeatureError.OutOfMemory;

            buf.appendSlice(_.alloc, ",\"rollout_pct\":") catch return FeatureError.OutOfMemory;
            appendUint(*buf, _.alloc, u64(f.rollout_pct))?;
            buf.appendSlice(_.alloc, ",\"killed\":") catch return FeatureError.OutOfMemory;
            buf.appendSlice(_.alloc, if (f.killed) "true" else "false") catch return FeatureError.OutOfMemory;

            buf.appendSlice(_.alloc, ",\"default_variant\":") catch return FeatureError.OutOfMemory;
            appendJsonEscaped(*buf, _.alloc, f.default_variant)?;

            buf.appendSlice(_.alloc, ",\"rules_count\":") catch return FeatureError.OutOfMemory;
            appendUint(*buf, _.alloc, u64(f.rules.items.len))?;
            buf.appendSlice(_.alloc, ",\"variants\":[") catch return FeatureError.OutOfMemory;

            let j: usize = 0;
            while (j < f.variants.items.len) : (j += 1) {
                if (j > 0) {
                    buf.append(_.alloc, ',') catch return FeatureError.OutOfMemory;
                }
                const v = f.variants.items[j];
                buf.appendSlice(_.alloc, "{\"name\":") catch return FeatureError.OutOfMemory;
                appendJsonEscaped(*buf, _.alloc, v.name)?;
                buf.appendSlice(_.alloc, ",\"weight_bps\":") catch return FeatureError.OutOfMemory;
                appendUint(*buf, _.alloc, u64(v.weight_bps))?;
                buf.append(_.alloc, '}') catch return FeatureError.OutOfMemory;
            }

            buf.appendSlice(_.alloc, "]}") catch return FeatureError.OutOfMemory;
        }

        buf.appendSlice(_.alloc, "]}") catch return FeatureError.OutOfMemory;
        return buf.toOwnedSlice(_.alloc) catch return FeatureError.OutOfMemory;
    }
}
