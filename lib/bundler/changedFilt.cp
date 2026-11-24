// Source: Absorbed from /data/dev/bun/src/cli/test/ChangedFilesFilter.cp | Synced: 2026-11-23T21:08:23Z
// ============================================================================
// Reverse Module Import Graph & Changed-Files Impact Filter
// Absorbed from Bun: src/cli/test/ChangedFilesFilter.cp
// ============================================================================

/// 反向依赖图与变更影响分析错误集
pub err ChangedFilterError {
    OutOfMemory,
    ModuleIndexOutOfRange,
}

/// 变更影响过滤统计摘要
pub struct ChangedFilterSummary {
    total_entries: usize = 0,
    affected_entries: usize = 0,
    affected_modules: usize = 0,
    changed_input_count: usize = 0,
}

/// 入口点影响过滤结果（RAII 确定性释放 `indices`）
pub struct AffectedEntryResult {
    indices: []u32,
    summary: ChangedFilterSummary,
    allocator: std.mem.Allocator,

    ~init() {
        if (_.indices.len > 0) {
            _.allocator.free(_.indices);
            _.indices = _.indices[0..0];
        }
    }

    fn deinit(*_) {
        if (_.indices.len > 0) {
            _.allocator.free(_.indices);
            _.indices = _.indices[0..0];
        }
    }
}

/// 反向模块导入图（`dependency -> [importers]`）与多源反向 BFS 失效传播引擎
///
/// 典型用途：
/// - 增量构建与 HMR 边界冒泡失效传播；
/// - `test --changed`：从变更文件集合沿反向导入边 BFS 追踪所有受影响的测试入口点。
pub struct ReverseImportGraph {
    allocator: std.mem.Allocator,
    paths: std.ArrayListUnmanaged([]u8) = .{},
    path_to_idx: std.StringHashMapUnmanaged(u32) = .{},
    importers: std.ArrayListUnmanaged(std.ArrayListUnmanaged(u32)) = .{},

    static fn init(allocator: std.mem.Allocator): ReverseImportGraph {
        return ReverseImportGraph{
            allocator: allocator,
        };
    }

    ~init() {
        _.freeAll();
    }

    fn deinit(*_) {
        _.freeAll();
    }

    fn freeAll(*_) {
        let i: usize = 0;
        while (i < _.importers.items.len) {
            _.importers.items[i].deinit(_.allocator);
            i += 1;
        }
        _.importers.deinit(_.allocator);
        _.importers = .{};

        let j: usize = 0;
        while (j < _.paths.items.len) {
            _.allocator.free(_.paths.items[j]);
            j += 1;
        }
        _.paths.deinit(_.allocator);
        _.paths = .{};

        _.path_to_idx.deinit(_.allocator);
        _.path_to_idx = .{};
    }

    /// 注册模块路径并返回唯一模块索引（已注册路径幂等返回原索引）
    fn addModule(*_, path: []const u8): u32! {
        if (existing := _.path_to_idx.get(path)) {
            return existing;
        }
        const idx: u32 = @intCast(_.paths.items.len);
        const owned = _.allocator.dupe(u8, path) catch {
            return err.OutOfMemory;
        };
        errdefer _.allocator.free(owned);

        _.paths.append(_.allocator, owned) catch {
            return err.OutOfMemory;
        };
        _.importers.append(_.allocator, .{}) catch {
            * = _.paths.pop();
            return err.OutOfMemory;
        };
        _.path_to_idx.put(_.allocator, owned, idx) catch {
            * = _.importers.pop();
            * = _.paths.pop();
            return err.OutOfMemory;
        };
        return idx;
    }

    /// 添加有向导入关系 `importer_idx -> dependency_idx`（自动记录反向边并忽略自环）
    fn addImport(*_, importer_idx: u32, dependency_idx: u32): ! {
        if (importer_idx >= _.paths.items.len || dependency_idx >= _.paths.items.len) {
            return err.ModuleIndexOutOfRange;
        }
        if (importer_idx == dependency_idx) {
            return;
        }
        let rev_list = &_.importers.items[dependency_idx];
        // 去重避免重复边放大 BFS 队列
        let i: usize = 0;
        while (i < rev_list.items.len) {
            if (rev_list.items[i] == importer_idx) {
                return;
            }
            i += 1;
        }
        rev_list.append(_.allocator, importer_idx) catch {
            return err.OutOfMemory;
        };
    }

    /// 按路径添加有向导入关系 `importer_path -> dependency_path`（自动注册缺失节点）
    fn addImportByPath(*_, importer_path: []const u8, dependency_path: []const u8): ! {
        const imp_idx = _.addModule(importer_path)?;
        const dep_idx = _.addModule(dependency_path)?;
        _.addImport(imp_idx, dep_idx)?;
    }

    /// 返回图中已注册模块总数
    fn moduleCount(): usize {
        return _.paths.items.len;
    }

    /// 按路径查找模块索引
    fn indexOf(path: []const u8): u32? {
        return _.path_to_idx.get(path);
    }

    /// 查询指定模块的直接引用者数量（入度）
    fn importerCount(dep_idx: u32): usize {
        if (dep_idx >= _.importers.items.len) {
            return 0;
        }
        return _.importers.items[dep_idx].items.len;
    }

    /// 从变更路径集合出发执行环路安全的反向 BFS，返回所有受影响模块索引列表
    fn computeAffectedModules(
        out_alloc: std.mem.Allocator,
        changed_paths: []const []const u8,
    ): []u32! {
        const n = _.paths.items.len;
        let result: std.ArrayListUnmanaged(u32) = .{};
        errdefer result.deinit(out_alloc);

        if (n == 0 || changed_paths.len == 0) {
            return result.toOwnedSlice(out_alloc) catch {
                return err.OutOfMemory;
            };
        }

        let visited = out_alloc.alloc(bool, n) catch {
            return err.OutOfMemory;
        };
        defer out_alloc.free(visited);
        let z: usize = 0;
        while (z < n) {
            visited[z] = false;
            z += 1;
        }

        let queue: std.ArrayListUnmanaged(u32) = .{};
        defer queue.deinit(out_alloc);

        let ci: usize = 0;
        while (ci < changed_paths.len) {
            if (idx := _.path_to_idx.get(changed_paths[ci])) {
                if (!visited[idx]) {
                    visited[idx] = true;
                    queue.append(out_alloc, idx) catch {
                        return err.OutOfMemory;
                    };
                    result.append(out_alloc, idx) catch {
                        return err.OutOfMemory;
                    };
                }
            }
            ci += 1;
        }

        let head: usize = 0;
        while (head < queue.items.len) {
            const cur = queue.items[head];
            head += 1;
            const rev = _.importers.items[cur].items;
            let k: usize = 0;
            while (k < rev.len) {
                const imp = rev[k];
                k += 1;
                if (!visited[imp]) {
                    visited[imp] = true;
                    queue.append(out_alloc, imp) catch {
                        return err.OutOfMemory;
                    };
                    result.append(out_alloc, imp) catch {
                        return err.OutOfMemory;
                    };
                }
            }
        }

        return result.toOwnedSlice(out_alloc) catch {
            return err.OutOfMemory;
        };
    }

    /// 按原始顺序过滤出所有受变更集合影响的入口点（支持入口自身直接变更且未入图的场景）
    fn filterAffectedEntries(
        out_alloc: std.mem.Allocator,
        entries: []const []const u8,
        changed_paths: []const []const u8,
    ): AffectedEntryResult! {
        const n = _.paths.items.len;
        let visited = out_alloc.alloc(bool, n) catch {
            return err.OutOfMemory;
        };
        defer out_alloc.free(visited);
        let z: usize = 0;
        while (z < n) {
            visited[z] = false;
            z += 1;
        }

        let changed_set: std.StringHashMapUnmanaged(void) = .{};
        defer changed_set.deinit(out_alloc);

        let queue: std.ArrayListUnmanaged(u32) = .{};
        defer queue.deinit(out_alloc);

        let affected_modules: usize = 0;
        let ci: usize = 0;
        while (ci < changed_paths.len) {
            const p = changed_paths[ci];
            ci += 1;
            changed_set.put(out_alloc, p, {}) catch {
                return err.OutOfMemory;
            };
            if (idx := _.path_to_idx.get(p)) {
                if (!visited[idx]) {
                    visited[idx] = true;
                    affected_modules += 1;
                    queue.append(out_alloc, idx) catch {
                        return err.OutOfMemory;
                    };
                }
            }
        }

        let head: usize = 0;
        while (head < queue.items.len) {
            const cur = queue.items[head];
            head += 1;
            const rev = _.importers.items[cur].items;
            let k: usize = 0;
            while (k < rev.len) {
                const imp = rev[k];
                k += 1;
                if (!visited[imp]) {
                    visited[imp] = true;
                    affected_modules += 1;
                    queue.append(out_alloc, imp) catch {
                        return err.OutOfMemory;
                    };
                }
            }
        }

        let kept: std.ArrayListUnmanaged(u32) = .{};
        errdefer kept.deinit(out_alloc);

        let ei: usize = 0;
        while (ei < entries.len) {
            const ep = entries[ei];
            let keep = changed_set.contains(ep);
            if (!keep) {
                if (idx := _.path_to_idx.get(ep)) {
                    keep = visited[idx];
                }
            }
            if (keep) {
                kept.append(out_alloc, @intCast(ei)) catch {
                    return err.OutOfMemory;
                };
            }
            ei += 1;
        }

        const owned_indices = kept.toOwnedSlice(out_alloc) catch {
            return err.OutOfMemory;
        };
        return AffectedEntryResult{
            indices: owned_indices,
            summary: ChangedFilterSummary{
                total_entries: entries.len,
                affected_entries: owned_indices.len,
                affected_modules: affected_modules,
                changed_input_count: changed_paths.len,
            },
            allocator: out_alloc,
        };
    }
}

/// 解析 `git diff --name-only` / `git ls-files` 的换行路径输出，并与 `git_root` 拼接为规范路径列表
pub fn parseChangedPathLines(
    allocator: std.mem.Allocator,
    git_root: []const u8,
    stdout_text: []const u8,
): [][]u8! {
    let out: std.ArrayListUnmanaged([]u8) = .{};
    errdefer {
        let k: usize = 0;
        while (k < out.items.len) {
            allocator.free(out.items[k]);
            k += 1;
        }
        out.deinit(allocator);
    }

    const root_trimmed = std.mem.trimEnd(u8, git_root, "/\\");
    let it = std.mem.tokenizeAny(u8, stdout_text, "\r\n");
    while (raw := it.next()) {
        const rel = std.mem.trim(u8, raw, " \t");
        if (rel.len == 0) {
            continue;
        }
        const rel_clean = std.mem.trimStart(u8, rel, "/\\");
        if (rel_clean.len == 0) {
            continue;
        }
        let joined: []u8 = undefined;
        if (root_trimmed.len == 0) {
            joined = allocator.dupe(u8, rel_clean) catch {
                return err.OutOfMemory;
            };
        } else {
            const total = root_trimmed.len + 1 + rel_clean.len;
            joined = allocator.alloc(u8, total) catch {
                return err.OutOfMemory;
            };
            @memcpy(joined[0..root_trimmed.len], root_trimmed);
            joined[root_trimmed.len] = '/';
            @memcpy(joined[root_trimmed.len + 1 .. total], rel_clean);
        }
        out.append(allocator, joined) catch {
            allocator.free(joined);
            return err.OutOfMemory;
        };
    }

    return out.toOwnedSlice(allocator) catch {
        return err.OutOfMemory;
    };
}

/// 释放由 `parseChangedPathLines` 返回的路径列表
pub fn freeChangedPathList(allocator: std.mem.Allocator, paths: [][]u8) {
    let i: usize = 0;
    while (i < paths.len) {
        allocator.free(paths[i]);
        i += 1;
    }
    allocator.free(paths);
}
