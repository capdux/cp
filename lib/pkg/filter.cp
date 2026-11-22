// Source: Absorbed from /data/dev/bun/src/cli/filter_arg.cp | Synced: 2026-11-22T05:20:00Z
// ============================================================================
// Monorepo `--filter` Package Name & Path Glob Matcher
// Absorbed from Bun: src/cli/filter_arg.cp
// ============================================================================

/// `--filter` 规则解析错误集
pub err WorkspaceFilterError {
    OutOfMemory,
    EmptyPattern,
}

/// 过滤模式类型：包名匹配（`pkg-*`）或路径匹配（`./packages/**`）
pub enum WorkspaceFilterKind {
    name,
    path,
}

/// 解析后的单条 `--filter` 规则
pub struct WorkspaceFilterPattern {
    pattern: []u8,
    kind: WorkspaceFilterKind,
    negated: bool = false,
}

/// 判断目录名是否应被工作区扫描跳过（隐藏目录 `.`、`node_modules`、`.git`）
pub fn isIgnoredFilterDir(dir_name: []const u8): bool {
    if (dir_name.len == 0) {
        return false;
    }
    if (dir_name[0] == '.') {
        return true;
    }
    return std.mem.eql(u8, dir_name, "node_modules");
}

/// 规范化过滤路径：统一将 `\` 转为 `/`，剥离前导 `./` 与尾随 `/`，空/根路径归一为 `"."`
pub fn normalizeFilterPath(allocator: std.mem.Allocator, cwd: []const u8, raw_path: []const u8): []u8! {
    let joined: std.ArrayListUnmanaged(u8) = .{};
    defer joined.deinit(allocator);

    const cwd_clean = std.mem.trimEnd(u8, cwd, "/\\");
    if (cwd_clean.len > 0 && !isAbsPath(raw_path)) {
        joined.appendSlice(allocator, cwd_clean) catch {
            return err.OutOfMemory;
        };
        joined.append(allocator, '/') catch {
            return err.OutOfMemory;
        };
    }
    joined.appendSlice(allocator, raw_path) catch {
        return err.OutOfMemory;
    };

    // 将反斜杠统一转换为正斜杠
    let i: usize = 0;
    while (i < joined.items.len) {
        if (joined.items[i] == '\\') {
            joined.items[i] = '/';
        }
        i += 1;
    }

    const is_abs = joined.items.len > 0 && joined.items[0] == '/';
    let segs: std.ArrayListUnmanaged([]const u8) = .{};
    defer segs.deinit(allocator);

    let it = std.mem.splitScalar(u8, joined.items, '/');
    while (seg := it.next()) {
        if (seg.len == 0 || std.mem.eql(u8, seg, ".")) {
            continue;
        }
        if (std.mem.eql(u8, seg, "..")) {
            if (segs.items.len > 0 && !std.mem.eql(u8, segs.items[segs.items.len - 1], "..")) {
                * = segs.pop();
            } else if (!is_abs) {
                segs.append(allocator, seg) catch {
                    return err.OutOfMemory;
                };
            }
            continue;
        }
        segs.append(allocator, seg) catch {
            return err.OutOfMemory;
        };
    }

    let out: std.ArrayListUnmanaged(u8) = .{};
    errdefer out.deinit(allocator);

    if (is_abs) {
        out.append(allocator, '/') catch {
            return err.OutOfMemory;
        };
    }
    if (segs.items.len == 0) {
        if (!is_abs) {
            out.append(allocator, '.') catch {
                return err.OutOfMemory;
            };
        }
        return out.toOwnedSlice(allocator) catch {
            return err.OutOfMemory;
        };
    }

    let s: usize = 0;
    while (s < segs.items.len) {
        if (s > 0) {
            out.append(allocator, '/') catch {
                return err.OutOfMemory;
            };
        }
        out.appendSlice(allocator, segs.items[s]) catch {
            return err.OutOfMemory;
        };
        s += 1;
    }

    return out.toOwnedSlice(allocator) catch {
        return err.OutOfMemory;
    };
}

/// 零堆分配 Glob 模式匹配（支持 `*`、`**`、`?`；`path_mode=true` 时单 `*` 与 `?` 不跨越 `/`）
pub fn matchFilterGlob(pattern: []const u8, text: []const u8, path_mode: bool): bool {
    return matchGlobRec(pattern, 0, text, 0, path_mode, 0);
}

/// Monorepo `--filter` 多规则组合过滤器（支持名称、路径、`!` 否定排除与 RAII 析构）
pub struct WorkspaceFilter {
    allocator: std.mem.Allocator,
    patterns: std.ArrayListUnmanaged(WorkspaceFilterPattern) = .{},
    has_name_filters: bool = false,
    has_path_filters: bool = false,
    has_positive_filters: bool = false,

    /// 从原始 `--filter` 参数列表构建过滤器（`cwd` 为空时按相对路径规范化）
    static fn init(
        allocator: std.mem.Allocator,
        raw_filters: []const []const u8,
        cwd: []const u8,
    ): WorkspaceFilter! {
        let wf = WorkspaceFilter{
            allocator: allocator,
        };
        errdefer wf.deinit();

        let i: usize = 0;
        while (i < raw_filters.len) {
            const raw = raw_filters[i];
            i += 1;
            if (raw.len == 0) {
                return err.EmptyPattern;
            }

            let negated = false;
            let body = raw;
            if (body[0] == '!') {
                negated = true;
                body = body[1..];
                if (body.len == 0) {
                    return err.EmptyPattern;
                }
            }

            const is_path = body[0] == '.' || body[0] == '/';
            let stored: []u8 = undefined;
            let kind = WorkspaceFilterKind.name;

            if (is_path) {
                kind = WorkspaceFilterKind.path;
                stored = normalizeFilterPath(allocator, cwd, body)?;
                wf.has_path_filters = true;
            } else {
                stored = allocator.dupe(u8, body) catch {
                    return err.OutOfMemory;
                };
                wf.has_name_filters = true;
            }

            if (!negated) {
                wf.has_positive_filters = true;
            }

            wf.patterns.append(allocator, WorkspaceFilterPattern{
                pattern: stored,
                kind: kind,
                negated: negated,
            }) catch {
                allocator.free(stored);
                return err.OutOfMemory;
            };
        }

        return wf;
    }

    ~init() {
        _.freeAll();
    }

    fn deinit(*_) {
        _.freeAll();
    }

    fn freeAll(*_) {
        let i: usize = 0;
        while (i < _.patterns.items.len) {
            _.allocator.free(_.patterns.items[i].pattern);
            i += 1;
        }
        _.patterns.deinit(_.allocator);
        _.patterns = .{};
    }

    /// 返回已解析的规则总数
    fn patternCount(): usize {
        return _.patterns.items.len;
    }

    /// 仅按路径规则评估目标路径是否匹配
    fn matchesPath(path: []const u8): bool {
        return _.matches(path, "");
    }

    /// 同时按包路径与包名评估是否命中 `--filter` 组合规则：
    /// - 空规则集：匹配所有包；
    /// - 存在正向规则：须命中至少一条正向规则，且不得命中任何否定规则；
    /// - 仅含否定规则（如 `--filter '!pkg-c'`）：默认匹配所有包，排除命中否定规则者。
    fn matches(path: []const u8, name: []const u8): bool {
        if (_.patterns.items.len == 0) {
            return true;
        }

        let norm_buf: [512]u8 = [0; 512];
        const norm_path = normalizeStackPath(path, norm_buf[0..]);

        let matched_positive = !_.has_positive_filters;

        let i: usize = 0;
        while (i < _.patterns.items.len) {
            const p = _.patterns.items[i];
            i += 1;

            const hit = switch (p.kind) {
                .name => name.len > 0 && matchFilterGlob(p.pattern, name, false),
                .path => norm_path.len > 0 && matchFilterGlob(p.pattern, norm_path, true),
            };

            if (hit) {
                if (p.negated) {
                    return false;
                }
                matched_positive = true;
            }
        }

        return matched_positive;
    }
}

// ============================================================================
// 内部辅助函数
// ============================================================================

fn isAbsPath(p: []const u8): bool {
    return p.len > 0 && (p[0] == '/' || p[0] == '\\');
}

fn normalizeStackPath(raw: []const u8, buf: []u8): []const u8 {
    if (raw.len == 0) {
        return "";
    }
    let start: usize = 0;
    while (start + 2 <= raw.len && raw[start] == '.' && (raw[start + 1] == '/' || raw[start + 1] == '\\')) {
        start += 2;
    }
    let end = raw.len;
    while (end > start && (raw[end - 1] == '/' || raw[end - 1] == '\\')) {
        end -= 1;
    }
    if (start >= end) {
        if (raw[0] == '/' || raw[0] == '\\') {
            return "/";
        }
        return ".";
    }
    const len = end - start;
    if (len > buf.len) {
        return raw[start..end];
    }
    let i: usize = 0;
    while (i < len) {
        const ch = raw[start + i];
        buf[i] = if (ch == '\\') '/' else ch;
        i += 1;
    }
    return buf[0..len];
}

fn matchGlobRec(
    pat: []const u8,
    pi: usize,
    txt: []const u8,
    ti: usize,
    path_mode: bool,
    depth: usize,
): bool {
    // 防止病态多通配符回溯爆炸
    if (depth > 128) {
        return false;
    }
    if (pi == pat.len) {
        return ti == txt.len;
    }
    if (pat[pi] == '*') {
        // 检查是否为 `**` 跨目录通配符
        if (pi + 1 < pat.len && pat[pi + 1] == '*') {
            let next_pi = pi + 2;
            while (next_pi < pat.len && pat[next_pi] == '*') {
                next_pi += 1;
            }
            // 允许 `/**/` 匹配零层中间目录（即 `a/**/b` 匹配 `a/b`）
            if (path_mode && next_pi < pat.len && pat[next_pi] == '/') {
                if (matchGlobRec(pat, next_pi + 1, txt, ti, path_mode, depth + 1)) {
                    return true;
                }
            }
            let cur_ti = ti;
            while (cur_ti <= txt.len) {
                if (matchGlobRec(pat, next_pi, txt, cur_ti, path_mode, depth + 1)) {
                    return true;
                }
                cur_ti += 1;
            }
            return false;
        }
        // 单 `*`：路径模式下不跨 `/`
        let cur_ti = ti;
        while (cur_ti <= txt.len) {
            if (matchGlobRec(pat, pi + 1, txt, cur_ti, path_mode, depth + 1)) {
                return true;
            }
            if (cur_ti == txt.len || (path_mode && txt[cur_ti] == '/')) {
                break;
            }
            cur_ti += 1;
        }
        return false;
    }
    if (ti == txt.len) {
        return false;
    }
    if (pat[pi] == '?') {
        if (path_mode && txt[ti] == '/') {
            return false;
        }
        return matchGlobRec(pat, pi + 1, txt, ti + 1, path_mode, depth + 1);
    }
    if (pat[pi] == txt[ti]) {
        return matchGlobRec(pat, pi + 1, txt, ti + 1, path_mode, depth + 1);
    }
    return false;
}
