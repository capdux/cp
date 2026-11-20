// Source: cp/lib/rule/rule.cp (Production Rule Engine: FactMap, Condition AST & RuleEngine)
// Pure CP zero-C-dependency declarative production rule engine for risk control,
// dynamic pricing routing, and AI Agent policy decisions.

const std = @import("std");

/// Maximum allowed condition AST nesting depth before returning `MaxDepthExceeded`.
pub const MAX_CONDITION_DEPTH: usize = 32;

/// Error set returned by rule engine registration, state mutation, and evaluation.
pub err RuleError {
    DuplicateRuleId,
    RuleNotFound,
    MaxDepthExceeded,
    TypeMismatch,
    OutOfMemory,
}

/// Typed scalar fact value stored in `FactMap` and compared in `Condition` predicates.
pub enum FactValue {
    Str: string,
    Int: i64,
    Float: f64,
    Bool: bool,

    static fn str(v: string): FactValue {
        return FactValue.Str(v);
    }

    static fn int(v: i64): FactValue {
        return FactValue.Int(v);
    }

    static fn float(v: f64): FactValue {
        return FactValue.Float(v);
    }

    static fn boolean(v: bool): FactValue {
        return FactValue.Bool(v);
    }
}

/// Typed key-value fact context passed to `RuleEngine.evaluate`.
/// Owns all keys and string values inserted via `set*` methods.
pub struct FactMap {
    alloc: std.mem.Allocator,
    entries: std.StringHashMapUnmanaged(FactValue) = .empty,

    /// Initialize an empty `FactMap` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): FactMap {
        return FactMap{
            alloc: alloc,
            entries: .empty,
        };
    }

    /// Release all owned keys, string values, and the underlying hash map.
    pub fn deinit(*_) {
        let it = _.entries.iterator();
        while (entry := it.next()) {
            _.alloc.free(entry.key_ptr.*);
            switch (entry.value_ptr.*) {
                .Str s => _.alloc.free(s),
                else => {},
            }
        }
        _.entries.deinit(_.alloc);
    }

    fn putOwned(*_, key: string, owned_val: FactValue): void ! RuleError {
        const owned_key = _.alloc.dupe(u8, key) catch return RuleError.OutOfMemory;
        if (old_kv := _.entries.fetchRemove(key)) {
            _.alloc.free(old_kv.key);
            freeFactValue(_.alloc, old_kv.value);
        }
        _.entries.put(_.alloc, owned_key, owned_val) catch {
            _.alloc.free(owned_key);
            return RuleError.OutOfMemory;
        };
    }

    /// Insert or update a string fact. Copies both `key` and `val`.
    pub fn setStr(*_, key: string, val: string): void ! RuleError {
        const owned_val = _.alloc.dupe(u8, val) catch return RuleError.OutOfMemory;
        _.putOwned(key, FactValue.Str(owned_val)) catch e => {
            _.alloc.free(owned_val);
            return e;
        };
    }

    /// Insert or update an integer fact.
    pub fn setInt(*_, key: string, val: i64): void ! RuleError {
        _.putOwned(key, FactValue.Int(val))?;
    }

    /// Insert or update a floating-point fact.
    pub fn setFloat(*_, key: string, val: f64): void ! RuleError {
        _.putOwned(key, FactValue.Float(val))?;
    }

    /// Insert or update a boolean fact.
    pub fn setBool(*_, key: string, val: bool): void ! RuleError {
        _.putOwned(key, FactValue.Bool(val))?;
    }

    /// Look up a fact by `key`, returning `null` if absent.
    pub fn get(key: string): FactValue? {
        return _.entries.get(key);
    }

    /// Return the number of stored facts.
    pub fn count(): usize {
        return _.entries.count();
    }
}

/// Comparison operator used in `Condition.Compare`.
pub enum CompareOp {
    Eq,
    Neq,
    Gt,
    Gte,
    Lt,
    Lte,
    Contains,
    StartsWith,
}

/// Payload for `Condition.Compare`.
pub struct CompareSpec {
    field: string;
    op: CompareOp;
    expected: FactValue;
}

/// Payload for `Condition.InList`.
pub struct InListSpec {
    field: string;
    items: []const FactValue;
}

/// Composable predicate AST node evaluated against a `FactMap`.
pub enum Condition {
    And: []const Condition,
    Or: []const Condition,
    Not: *const Condition,
    Compare: CompareSpec,
    Exists: string,
    InList: InListSpec,

    static fn andAll(children: []const Condition): Condition {
        return Condition.And(children);
    }

    static fn orAny(children: []const Condition): Condition {
        return Condition.Or(children);
    }

    static fn notExpr(child: *const Condition): Condition {
        return Condition.Not(child);
    }

    static fn cmp(field: string, op: CompareOp, expected: FactValue): Condition {
        return Condition.Compare(CompareSpec{
            field: field,
            op: op,
            expected: expected,
        });
    }

    static fn exists(field: string): Condition {
        return Condition.Exists(field);
    }

    static fn inList(field: string, items: []const FactValue): Condition {
        return Condition.InList(InListSpec{
            field: field,
            items: items,
        });
    }
}

/// Declarative production rule specification passed to `RuleEngine.addRule`.
pub struct RuleSpec {
    id: string;
    name: string;
    salience: i32 = 0;
    activation_group: string? = null;
    enabled: bool = true;
    condition: Condition;
    action_tag: string;
    action_payload: string = "";
}

/// Audit record for a single matched rule in `RuleEvalReport`.
pub struct FiredRule {
    rule_id: string;
    salience: i32;
    activation_group: string?;
    action_tag: string;
    action_payload: string;
}

/// Full evaluation audit report returned by `RuleEngine.evaluate`.
pub struct RuleEvalReport {
    alloc: std.mem.Allocator,
    matched: []FiredRule,
    evaluated_count: usize,
    skipped_by_group_count: usize,

    /// Release all owned audit strings and the `matched` slice.
    pub fn deinit(*_) {
        for (item of _.matched) {
            _.alloc.free(item.rule_id);
            if (grp := item.activation_group) {
                _.alloc.free(grp);
            }
            _.alloc.free(item.action_tag);
            _.alloc.free(item.action_payload);
        }
        _.alloc.free(_.matched);
    }

    /// Return the highest-priority matched rule, or `null` if no rule fired.
    pub fn topAction(): FiredRule? {
        if (_.matched.len == 0) {
            return null;
        }
        return _.matched[0];
    }
}

fn cloneFactValue(alloc: std.mem.Allocator, val: FactValue): FactValue ! RuleError {
    return switch (val) {
        .Str s => FactValue.Str(alloc.dupe(u8, s) catch return RuleError.OutOfMemory),
        .Int v => FactValue.Int(v),
        .Float v => FactValue.Float(v),
        .Bool v => FactValue.Bool(v),
    };
}

fn freeFactValue(alloc: std.mem.Allocator, val: FactValue) {
    switch (val) {
        .Str s => alloc.free(s),
        else => {},
    }
}

fn cloneCondition(alloc: std.mem.Allocator, cond: &Condition): Condition ! RuleError {
    switch (cond.*) {
        .And children => {
            let cloned = alloc.alloc(Condition, children.len) catch return RuleError.OutOfMemory;
            let initialized: usize = 0;
            errdefer {
                let k: usize = 0;
                while (k < initialized) : (k += 1) {
                    freeCondition(alloc, &cloned[k]);
                }
                alloc.free(cloned);
            }
            for (c of children) {
                cloned[initialized] = cloneCondition(alloc, &c)?;
                initialized += 1;
            }
            return Condition.And(cloned);
        },
        .Or children => {
            let cloned = alloc.alloc(Condition, children.len) catch return RuleError.OutOfMemory;
            let initialized: usize = 0;
            errdefer {
                let k: usize = 0;
                while (k < initialized) : (k += 1) {
                    freeCondition(alloc, &cloned[k]);
                }
                alloc.free(cloned);
            }
            for (c of children) {
                cloned[initialized] = cloneCondition(alloc, &c)?;
                initialized += 1;
            }
            return Condition.Or(cloned);
        },
        .Not child => {
            let ptr = alloc.create(Condition) catch return RuleError.OutOfMemory;
            errdefer alloc.destroy(ptr);
            ptr.* = cloneCondition(alloc, child)?;
            return Condition.Not(ptr);
        },
        .Compare spec => {
            const owned_field = alloc.dupe(u8, spec.field) catch return RuleError.OutOfMemory;
            errdefer alloc.free(owned_field);
            const owned_expected = cloneFactValue(alloc, spec.expected)?;
            return Condition.Compare(CompareSpec{
                field: owned_field,
                op: spec.op,
                expected: owned_expected,
            });
        },
        .Exists field => {
            const owned_field = alloc.dupe(u8, field) catch return RuleError.OutOfMemory;
            return Condition.Exists(owned_field);
        },
        .InList spec => {
            const owned_field = alloc.dupe(u8, spec.field) catch return RuleError.OutOfMemory;
            errdefer alloc.free(owned_field);
            let cloned_items = alloc.alloc(FactValue, spec.items.len) catch return RuleError.OutOfMemory;
            let initialized: usize = 0;
            errdefer {
                let k: usize = 0;
                while (k < initialized) : (k += 1) {
                    freeFactValue(alloc, cloned_items[k]);
                }
                alloc.free(cloned_items);
            }
            for (item of spec.items) {
                cloned_items[initialized] = cloneFactValue(alloc, item)?;
                initialized += 1;
            }
            return Condition.InList(InListSpec{
                field: owned_field,
                items: cloned_items,
            });
        },
    }
}

fn freeCondition(alloc: std.mem.Allocator, cond: &Condition) {
    switch (cond.*) {
        .And children => {
            for (c of children) {
                freeCondition(alloc, &c);
            }
            alloc.free(children);
        },
        .Or children => {
            for (c of children) {
                freeCondition(alloc, &c);
            }
            alloc.free(children);
        },
        .Not child => {
            freeCondition(alloc, child);
            alloc.destroy(child);
        },
        .Compare spec => {
            alloc.free(spec.field);
            freeFactValue(alloc, spec.expected);
        },
        .Exists field => {
            alloc.free(field);
        },
        .InList spec => {
            alloc.free(spec.field);
            for (item of spec.items) {
                freeFactValue(alloc, item);
            }
            alloc.free(spec.items);
        },
    }
}

/// Exact ordering between `i64` and `f64`, returning `-1`, `0`, `1`, or `null` for `NaN`.
fn cmpIntFloat(i: i64, f: f64): i8? {
    if (std.math.isNan(f)) {
        return null;
    }
    if (f <= -9223372036854775808.0) {
        if (f < -9223372036854775808.0) return 1;
        return if (i == std.math.minInt(i64)) 0 else 1;
    }
    if (f >= 9223372036854775808.0) {
        return -1;
    }
    const fi: f64 = i as f64;
    if (fi < f) return -1;
    if (fi > f) return 1;
    // When fi == f, i might differ by rounding for |i| > 2^53; compare exact integer part of f.
    const trunc_f = @trunc(f);
    const f_int = @as(i64, @intFromFloat(trunc_f));
    if (i < f_int) return -1;
    if (i > f_int) return 1;
    if (f > trunc_f) return -1;
    if (f < trunc_f) return 1;
    return 0;
}

fn evalNumericCompare(ord_opt: i8?, op: CompareOp): bool {
    const ord = ord_opt ?? return (op == .Neq);
    return switch (op) {
        .Eq => ord == 0,
        .Neq => ord != 0,
        .Gt => ord > 0,
        .Gte => ord >= 0,
        .Lt => ord < 0,
        .Lte => ord <= 0,
        else => false,
    };
}

fn evalCompareValues(actual: FactValue, op: CompareOp, expected: FactValue): bool ! RuleError {
    switch (op) {
        .Contains, .StartsWith => {
            const a_str = switch (actual) {
                .Str s => s,
                else => return RuleError.TypeMismatch,
            };
            const e_str = switch (expected) {
                .Str s => s,
                else => return RuleError.TypeMismatch,
            };
            if (op == .Contains) {
                return std.mem.indexOf(u8, a_str, e_str) != null;
            }
            return std.mem.startsWith(u8, a_str, e_str);
        },
        .Eq, .Neq => {
            switch (actual) {
                .Str a_s => {
                    const e_s = switch (expected) {
                        .Str s => s,
                        else => return RuleError.TypeMismatch,
                    };
                    const eq = std.mem.eql(u8, a_s, e_s);
                    return if (op == .Eq) eq else !eq;
                },
                .Bool a_b => {
                    const e_b = switch (expected) {
                        .Bool b => b,
                        else => return RuleError.TypeMismatch,
                    };
                    const eq = (a_b == e_b);
                    return if (op == .Eq) eq else !eq;
                },
                .Int a_i => {
                    return switch (expected) {
                        .Int e_i => if (op == .Eq) (a_i == e_i) else (a_i != e_i),
                        .Float e_f => evalNumericCompare(cmpIntFloat(a_i, e_f), op),
                        else => RuleError.TypeMismatch,
                    };
                },
                .Float a_f => {
                    return switch (expected) {
                        .Int e_i => {
                            const rev = cmpIntFloat(e_i, a_f);
                            const ord: i8? = if (rev != null) -rev.? else null;
                            return evalNumericCompare(ord, op);
                        },
                        .Float e_f => {
                            if (std.math.isNan(a_f) || std.math.isNan(e_f)) {
                                return op == .Neq;
                            }
                            return if (op == .Eq) (a_f == e_f) else (a_f != e_f);
                        },
                        else => RuleError.TypeMismatch,
                    };
                },
            }
        },
        .Gt, .Gte, .Lt, .Lte => {
            switch (actual) {
                .Int a_i => {
                    return switch (expected) {
                        .Int e_i => {
                            const ord: i8 = if (a_i < e_i) -1 else if (a_i > e_i) 1 else 0;
                            return evalNumericCompare(ord, op);
                        },
                        .Float e_f => evalNumericCompare(cmpIntFloat(a_i, e_f), op),
                        else => RuleError.TypeMismatch,
                    };
                },
                .Float a_f => {
                    return switch (expected) {
                        .Int e_i => {
                            const rev = cmpIntFloat(e_i, a_f);
                            const ord: i8? = if (rev != null) -rev.? else null;
                            return evalNumericCompare(ord, op);
                        },
                        .Float e_f => {
                            if (std.math.isNan(a_f) || std.math.isNan(e_f)) {
                                return false;
                            }
                            const ord: i8 = if (a_f < e_f) -1 else if (a_f > e_f) 1 else 0;
                            return evalNumericCompare(ord, op);
                        },
                        else => RuleError.TypeMismatch,
                    };
                },
                else => return RuleError.TypeMismatch,
            }
        },
    }
}

fn factValuesEqual(a: FactValue, b: FactValue): bool {
    switch (a) {
        .Str a_s => {
            return switch (b) {
                .Str b_s => std.mem.eql(u8, a_s, b_s),
                else => false,
            };
        },
        .Bool a_b => {
            return switch (b) {
                .Bool b_b => a_b == b_b,
                else => false,
            };
        },
        .Int a_i => {
            return switch (b) {
                .Int b_i => a_i == b_i,
                .Float b_f => (cmpIntFloat(a_i, b_f) ?? 1) == 0,
                else => false,
            };
        },
        .Float a_f => {
            return switch (b) {
                .Int b_i => (cmpIntFloat(b_i, a_f) ?? 1) == 0,
                .Float b_f => !std.math.isNan(a_f) && !std.math.isNan(b_f) && a_f == b_f,
                else => false,
            };
        },
    }
}

fn evalCondition(cond: &Condition, facts: &FactMap, depth: usize): bool ! RuleError {
    if (depth > MAX_CONDITION_DEPTH) {
        return RuleError.MaxDepthExceeded;
    }
    switch (cond.*) {
        .And children => {
            for (c of children) {
                if (!(evalCondition(&c, facts, depth + 1)?)) {
                    return false;
                }
            }
            return true;
        },
        .Or children => {
            for (c of children) {
                if (evalCondition(&c, facts, depth + 1)?) {
                    return true;
                }
            }
            return false;
        },
        .Not child => {
            return !(evalCondition(child, facts, depth + 1)?);
        },
        .Exists field => {
            return facts.get(field) != null;
        },
        .Compare spec => {
            const actual = facts.get(spec.field) ?? return false;
            return evalCompareValues(actual, spec.op, spec.expected);
        },
        .InList spec => {
            const actual = facts.get(spec.field) ?? return false;
            for (item of spec.items) {
                if (factValuesEqual(actual, item)) {
                    return true;
                }
            }
            return false;
        },
    }
}

struct StoredRule {
    id: string;
    name: string;
    salience: i32;
    activation_group: string?;
    enabled: bool;
    condition: Condition;
    action_tag: string;
    action_payload: string;

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.id);
        alloc.free(_.name);
        if (grp := _.activation_group) {
            alloc.free(grp);
        }
        freeCondition(alloc, &_.condition);
        alloc.free(_.action_tag);
        alloc.free(_.action_payload);
    }
}

/// Declarative production rule engine with salience ordering,
/// mutually-exclusive activation groups, and evaluation audit reporting.
pub struct RuleEngine {
    alloc: std.mem.Allocator,
    rules: std.ArrayList(StoredRule) = .empty,
    id_set: std.StringHashMapUnmanaged(void) = .empty,

    /// Initialize an empty `RuleEngine` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): RuleEngine {
        return RuleEngine{
            alloc: alloc,
            rules: .empty,
            id_set: .empty,
        };
    }

    /// Release all owned rules, conditions, and ID index entries.
    pub fn deinit(*_) {
        for (*r of _.rules.items) {
            r.deinit(_.alloc);
        }
        _.rules.deinit(_.alloc);
        _.id_set.deinit(_.alloc);
    }

    /// Return the total number of registered rules (enabled and disabled).
    pub fn ruleCount(): usize {
        return _.rules.items.len;
    }

    /// Register a new rule, deep-cloning its strings and condition AST.
    /// Rules are kept sorted by `salience` descending, preserving insertion order on ties.
    pub fn addRule(*_, spec: RuleSpec): void ! RuleError {
        if (_.id_set.contains(spec.id)) {
            return RuleError.DuplicateRuleId;
        }

        const owned_id = _.alloc.dupe(u8, spec.id) catch return RuleError.OutOfMemory;
        errdefer _.alloc.free(owned_id);

        const owned_name = _.alloc.dupe(u8, spec.name) catch return RuleError.OutOfMemory;
        errdefer _.alloc.free(owned_name);

        let owned_group: string? = null;
        if (grp := spec.activation_group) {
            owned_group = _.alloc.dupe(u8, grp) catch return RuleError.OutOfMemory;
        }
        errdefer {
            if (g := owned_group) {
                _.alloc.free(g);
            }
        }

        const owned_cond = cloneCondition(_.alloc, &spec.condition)?;
        errdefer freeCondition(_.alloc, &owned_cond);

        const owned_tag = _.alloc.dupe(u8, spec.action_tag) catch return RuleError.OutOfMemory;
        errdefer _.alloc.free(owned_tag);

        const owned_payload = _.alloc.dupe(u8, spec.action_payload) catch return RuleError.OutOfMemory;
        errdefer _.alloc.free(owned_payload);

        let insert_idx: usize = _.rules.items.len;
        let i: usize = 0;
        while (i < _.rules.items.len) : (i += 1) {
            if (_.rules.items[i].salience < spec.salience) {
                insert_idx = i;
                break;
            }
        }

        const stored = StoredRule{
            id: owned_id,
            name: owned_name,
            salience: spec.salience,
            activation_group: owned_group,
            enabled: spec.enabled,
            condition: owned_cond,
            action_tag: owned_tag,
            action_payload: owned_payload,
        };

        _.rules.insert(_.alloc, insert_idx, stored) catch return RuleError.OutOfMemory;
        _.id_set.put(_.alloc, owned_id, {}) catch {
            * = _.rules.orderedRemove(insert_idx);
            return RuleError.OutOfMemory;
        };
    }

    /// Enable or disable an existing rule by `rule_id`.
    pub fn setEnabled(*_, rule_id: string, enabled: bool): void ! RuleError {
        for (*r of _.rules.items) {
            if (std.mem.eql(u8, r.id, rule_id)) {
                r.enabled = enabled;
                return;
            }
        }
        return RuleError.RuleNotFound;
    }

    /// Evaluate all enabled rules against `facts` in salience-descending order.
    /// Non-empty `activation_group`s short-circuit lower-priority rules in the same group once matched.
    pub fn evaluate(alloc: std.mem.Allocator, facts: &FactMap): RuleEvalReport ! RuleError {
        let fired_groups: std.StringHashMapUnmanaged(void) = .empty;
        defer fired_groups.deinit(alloc);

        let matched_list: std.ArrayList(FiredRule) = .empty;
        errdefer {
            for (item of matched_list.items) {
                alloc.free(item.rule_id);
                if (g := item.activation_group) {
                    alloc.free(g);
                }
                alloc.free(item.action_tag);
                alloc.free(item.action_payload);
            }
            matched_list.deinit(alloc);
        }

        let evaluated_count: usize = 0;
        let skipped_by_group_count: usize = 0;

        for (rule of _.rules.items) {
            if (!rule.enabled) {
                continue;
            }
            if (grp := rule.activation_group) {
                if (grp.len > 0 && fired_groups.contains(grp)) {
                    skipped_by_group_count += 1;
                    continue;
                }
            }

            evaluated_count += 1;
            const is_match = evalCondition(&rule.condition, facts, 0)?;
            if (is_match) {
                const dup_id = alloc.dupe(u8, rule.id) catch return RuleError.OutOfMemory;
                errdefer alloc.free(dup_id);

                let dup_grp: string? = null;
                if (grp := rule.activation_group) {
                    dup_grp = alloc.dupe(u8, grp) catch return RuleError.OutOfMemory;
                }
                errdefer {
                    if (g := dup_grp) {
                        alloc.free(g);
                    }
                }

                const dup_tag = alloc.dupe(u8, rule.action_tag) catch return RuleError.OutOfMemory;
                errdefer alloc.free(dup_tag);

                const dup_payload = alloc.dupe(u8, rule.action_payload) catch return RuleError.OutOfMemory;
                errdefer alloc.free(dup_payload);

                matched_list.append(alloc, FiredRule{
                    rule_id: dup_id,
                    salience: rule.salience,
                    activation_group: dup_grp,
                    action_tag: dup_tag,
                    action_payload: dup_payload,
                }) catch return RuleError.OutOfMemory;

                if (grp := rule.activation_group) {
                    if (grp.len > 0) {
                        fired_groups.put(alloc, grp, {}) catch return RuleError.OutOfMemory;
                    }
                }
            }
        }

        const matched_slice = matched_list.toOwnedSlice(alloc) catch return RuleError.OutOfMemory;
        return RuleEvalReport{
            alloc: alloc,
            matched: matched_slice,
            evaluated_count: evaluated_count,
            skipped_by_group_count: skipped_by_group_count,
        };
    }
}
