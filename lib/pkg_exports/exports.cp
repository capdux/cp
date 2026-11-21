// Source: Absorbed from /data/dev/bun/src/resolver/package_json.cp

/// 导出目标类别（路径映射或 `null` 显式阻断）
pub enum ExportTargetKind: u8 {
    path = 0,
    blocked = 1,
}

/// 单条条件导出规则（如 `"bun"` / `"import"` / `"require"` / `"types"` / `"default"`）
pub struct ExportConditionRule {
    condition: []const u8 = "default",
    kind: ExportTargetKind = ExportTargetKind.path,
    target: []const u8 = "",

    static fn toPath(condition: []const u8, target: []const u8): ExportConditionRule {
        return ExportConditionRule{
            condition: condition,
            kind: ExportTargetKind.path,
            target: target,
        };
    }

    static fn block(condition: []const u8): ExportConditionRule {
        return ExportConditionRule{
            condition: condition,
            kind: ExportTargetKind.blocked,
            target: "",
        };
    }
}

/// 单个子路径导出条目（最多 8 条条件规则按声明顺序求值）
pub struct ExportSubpathEntry {
    subpath: []const u8 = "",
    rules: [8]ExportConditionRule = [
        ExportConditionRule{}, ExportConditionRule{}, ExportConditionRule{}, ExportConditionRule{},
        ExportConditionRule{}, ExportConditionRule{}, ExportConditionRule{}, ExportConditionRule{},
    ],
    rule_count: usize = 0,
}

/// 解析状态码
pub enum ResolveStatus: u8 {
    matched = 0,
    blocked = 1,
    not_found = 2,
    invalid_subpath = 3,
    buffer_too_small = 4,
}

/// `ExportsMap.resolve` 解析结果
pub struct ResolveResult {
    status: ResolveStatus,
    path: []const u8 = "",
    matched_condition: []const u8 = "",

    fn isMatched(): bool { return _.status == ResolveStatus.matched; }
    fn isBlocked(): bool { return _.status == ResolveStatus.blocked; }
    fn isNotFound(): bool { return _.status == ResolveStatus.not_found; }
}

fn containsTraversal(s: []const u8): bool {
    let pos: usize = 0;
    while (pos < s.len) {
        if (s[pos] == '\\') {
            return true;
        }
        const seg_start = pos;
        while (pos < s.len && s[pos] != '/') {
            pos += 1;
        }
        const seg = s[seg_start..pos];
        if (std.mem.eql(u8, seg, "..")) {
            return true;
        }
        if (pos < s.len && s[pos] == '/') {
            pos += 1;
        }
    }
    return false;
}

fn isValidSubpath(subpath: []const u8): bool {
    if (std.mem.eql(u8, subpath, ".")) {
        return true;
    }
    if (subpath.len < 2 || subpath[0] != '.' || subpath[1] != '/') {
        return false;
    }
    return !containsTraversal(subpath);
}

fn conditionMatches(cond: []const u8, active: []const []const u8): bool {
    if (std.mem.eql(u8, cond, "default")) {
        return true;
    }
    for (c of active) {
        if (std.mem.eql(u8, cond, c)) {
            return true;
        }
    }
    return false;
}

fn substituteTarget(target: []const u8, capture: ([]const u8)?, out: []u8): ([]const u8)? {
    if (cap := capture) {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < target.len) {
            if (target[i] == '*') {
                if (pos + cap.len > out.len) { return null; }
                if (cap.len > 0) {
                    @memcpy(out[pos .. pos + cap.len], cap);
                    pos += cap.len;
                }
            } else {
                if (pos + 1 > out.len) { return null; }
                out[pos] = target[i];
                pos += 1;
            }
            i += 1;
        }
        return out[0..pos];
    }
    if (target.len > out.len) { return null; }
    if (target.len > 0) {
        @memcpy(out[0..target.len], target);
    }
    return out[0..target.len];
}

/// Node.js / Bun `package.json` `"exports"` 条件子路径解析引擎（固定容量 32 条目，零堆分配）
pub struct ExportsMap {
    entries: [32]ExportSubpathEntry = [
        ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{},
        ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{},
        ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{},
        ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{},
        ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{},
        ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{},
        ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{},
        ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{}, ExportSubpathEntry{},
    ],
    len: usize = 0,

    static fn init(): ExportsMap {
        return ExportsMap{};
    }

    /// 注册子路径及其有序条件规则（超过容量返回 `false`）
    fn addSubpath(*_, subpath: []const u8, rules: []const ExportConditionRule): bool {
        if (_.len >= 32 || rules.len > 8 || subpath.len == 0) {
            return false;
        }
        let entry = ExportSubpathEntry{
            subpath: subpath,
            rule_count: rules.len,
        };
        let i: usize = 0;
        while (i < rules.len) {
            entry.rules[i] = rules[i];
            i += 1;
        }
        _.entries[_.len] = entry;
        _.len += 1;
        return true;
    }

    /// 快捷注册无条件直接路径映射（等价于单条 `"default"` 规则）
    fn addDirect(*_, subpath: []const u8, target: []const u8): bool {
        const rules: [1]ExportConditionRule = [ExportConditionRule.toPath("default", target)];
        return _.addSubpath(subpath, rules[0..]);
    }

    /// 快捷注册 `null` 阻断子路径（等价于 `package.json` 中 `"./internal/*": null`）
    fn blockSubpath(*_, subpath: []const u8): bool {
        const rules: [1]ExportConditionRule = [ExportConditionRule.block("default")];
        return _.addSubpath(subpath, rules[0..]);
    }

    fn evaluateEntry(
        entry: ExportSubpathEntry,
        active_conditions: []const []const u8,
        capture: ([]const u8)?,
        out: []u8,
    ): ResolveResult {
        let i: usize = 0;
        while (i < entry.rule_count) {
            const rule = entry.rules[i];
            if (conditionMatches(rule.condition, active_conditions)) {
                if (rule.kind == ExportTargetKind.blocked) {
                    return ResolveResult{
                        status: ResolveStatus.blocked,
                        matched_condition: rule.condition,
                    };
                }
                const resolved = substituteTarget(rule.target, capture, out) ?? return ResolveResult{
                    status: ResolveStatus.buffer_too_small,
                    matched_condition: rule.condition,
                };
                if (containsTraversal(resolved)) {
                    return ResolveResult{ status: ResolveStatus.invalid_subpath };
                }
                return ResolveResult{
                    status: ResolveStatus.matched,
                    path: resolved,
                    matched_condition: rule.condition,
                };
            }
            i += 1;
        }
        return ResolveResult{ status: ResolveStatus.not_found };
    }

    /// 按 Node.js `PACKAGE_EXPORTS_RESOLVE` 规范解析子路径（精确优先 -> 最长前缀/后缀通配符 -> 条件顺序求值）
    fn resolve(subpath: []const u8, active_conditions: []const []const u8, out: []u8): ResolveResult {
        if (!isValidSubpath(subpath)) {
            return ResolveResult{ status: ResolveStatus.invalid_subpath };
        }

        // 1. 精确子路径匹配优先
        let i: usize = 0;
        while (i < _.len) {
            const entry = _.entries[i];
            if (std.mem.indexOfScalar(u8, entry.subpath, '*') == null
                && std.mem.eql(u8, entry.subpath, subpath))
            {
                return _.evaluateEntry(entry, active_conditions, null, out);
            }
            i += 1;
        }

        // 2. 通配符匹配：按 PATTERN_KEY_COMPARE（前缀最长优先，其次后缀最长）
        let best_idx: usize? = null;
        let best_prefix_len: usize = 0;
        let best_suffix_len: usize = 0;
        let best_capture: []const u8 = "";

        let j: usize = 0;
        while (j < _.len) {
            const entry = _.entries[j];
            if (star := std.mem.indexOfScalar(u8, entry.subpath, '*')) {
                const prefix = entry.subpath[0..star];
                const suffix = entry.subpath[star + 1 ..];
                if (subpath.len >= prefix.len + suffix.len
                    && std.mem.eql(u8, subpath[0..prefix.len], prefix)
                    && std.mem.eql(u8, subpath[subpath.len - suffix.len ..], suffix))
                {
                    const is_better = (best_idx == null)
                        || (prefix.len > best_prefix_len)
                        || (prefix.len == best_prefix_len && suffix.len > best_suffix_len);
                    if (is_better) {
                        best_idx = j;
                        best_prefix_len = prefix.len;
                        best_suffix_len = suffix.len;
                        best_capture = subpath[prefix.len .. subpath.len - suffix.len];
                    }
                }
            }
            j += 1;
        }

        if (idx := best_idx) {
            if (containsTraversal(best_capture)) {
                return ResolveResult{ status: ResolveStatus.invalid_subpath };
            }
            return _.evaluateEntry(_.entries[idx], active_conditions, best_capture, out);
        }

        return ResolveResult{ status: ResolveStatus.not_found };
    }
}
