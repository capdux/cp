// Source: Absorbed from /data/dev/bun/src/install/lockfile/Tree.cp | Synced: 2026-11-22T13:15:00Z
// Pure-CP zero-allocation Node Modules Hoisting Tree builder & path formatter.
// Faithfully models Bun's Lockfile.Tree hoisting semantics:
// - BFS placement with ancestor hoisting, conflict nesting, and bundled hoist-root isolation
// - Optional peer dependency deduplication & deferred resolution replacement
// - Resolvable vs Filter build modes (disabled OS/CPU packages & --omit=dev/peer/optional)
// - Relative path & depth formatting in both `node_modules` and `pkg_path` styles

/// 依赖提升树支持的最大子树节点数（零堆固定上限）
pub const HOIST_MAX_TREES: usize = 64;
/// 依赖提升树支持的最大包数
pub const HOIST_MAX_PACKAGES: usize = 64;
/// 依赖提升树支持的最大依赖边数
pub const HOIST_MAX_DEPS: usize = 256;
/// 每个子树节点最多直接放置的依赖数
pub const HOIST_MAX_DEPS_PER_TREE: usize = 64;
/// 嵌套 `node_modules` 最大深度上限（防异常环路）
pub const HOIST_MAX_DEPTH: usize = 32;

/// 无效节点/包 ID 哨兵值（对齐 Bun `invalid_id` / `invalid_package_id`）
pub const HOIST_INVALID_ID: u32 = 0xffffffff;
/// 根依赖哨兵 ID（对齐 Bun `root_dep_id`）
pub const HOIST_ROOT_DEP_ID: u32 = 0xfffffffe;

/// 依赖行为位标志（对齐 Bun `Dependency.Behavior` 与 `Resolution.Tag`）
pub const HOIST_DEP_NORMAL: u8 = 0;
pub const HOIST_DEP_DEV: u8 = 1 << 0;
pub const HOIST_DEP_OPTIONAL: u8 = 1 << 1;
pub const HOIST_DEP_PEER: u8 = 1 << 2;
pub const HOIST_DEP_BUNDLED: u8 = 1 << 3;
pub const HOIST_DEP_FOLDER: u8 = 1 << 4;
pub const HOIST_DEP_WORKSPACE_ROOT: u8 = 1 << 5;

/// 依赖提升树错误集
pub err HoistTreeError {
    CapacityExceeded,
    DependencyLoop,
    InvalidPackageId,
    InvalidTreeId,
    InvalidDepId,
    BufferTooSmall,
    EmptyName,
    MaxDepthExceeded,
}

/// 依赖树路径输出风格（对齐 Bun `Tree.IteratorPathStyle`）
pub enum HoistPathStyle: u8 {
    /// 形如 `node_modules` 或 `node_modules/a/node_modules/b/node_modules`
    NodeModules = 0,
    /// 形如 `""` 或 `a/b`
    PkgPath = 1,
}

/// 依赖树构建模式（对齐 Bun `Tree.BuilderMethod`）
pub enum HoistBuildMode: u8 {
    /// 保留所有依赖以便配置变更时仍可解析（用于写入 lockfile）
    Resolvable = 0,
    /// 过滤禁用包与省略依赖类型，实现更激进的提升（用于实际安装）
    Filter = 1,
}

/// 单条依赖的提升决策种类（对齐 Bun `HoistDependencyResult`）
pub enum HoistDecisionKind: u8 {
    Hoisted = 0,
    Placed = 1,
    DependencyLoop = 2,
    ResolveOptionalPeer = 3,
    ResolveReplace = 4,
    ResolveLater = 5,
}

/// 依赖过滤省略策略（对应 `--omit=dev/peer/optional`）
pub struct HoistOmitPolicy {
    omit_dev: bool = false,
    omit_optional: bool = false,
    omit_peer: bool = false,
}

/// 单条依赖的提升判定结果
pub struct HoistDecision {
    kind: HoistDecisionKind = .Hoisted,
    tree_id: u32 = HOIST_INVALID_ID,
    pkg_id: u32 = HOIST_INVALID_ID,
    dep_id: u32 = HOIST_INVALID_ID,
    bundled: bool = false,
}

/// 相对路径格式化结果
pub struct HoistPathResult {
    path_len: usize = 0,
    depth: usize = 0,
}

/// 参与提升计算的包记录
pub struct HoistPkgRecord {
    id: u32 = 0,
    name: []const u8 = "",
    version: []const u8 = "",
    disabled: bool = false,
}

/// 参与提升计算的依赖边记录
pub struct HoistDepRecord {
    dep_id: u32 = 0,
    parent_pkg_id: u32 = 0,
    name: []const u8 = "",
    resolved_pkg_id: u32 = HOIST_INVALID_ID,
    flags: u8 = HOIST_DEP_NORMAL,
    peer_compatible: bool = false,
}

/// 提升树中的单个目录节点（每个节点对应一层 `node_modules` 目录）
pub struct HoistNode {
    id: u32 = 0,
    parent: u32 = HOIST_INVALID_ID,
    dep_id: u32 = HOIST_ROOT_DEP_ID,
    folder_name: []const u8 = "",
    dep_count: usize = 0,
    deps: [HOIST_MAX_DEPS_PER_TREE]u32 = [0; HOIST_MAX_DEPS_PER_TREE],
}

/// BFS 展开队列项（对齐 Bun `Tree.FillItem`）
struct HoistQueueItem {
    tree_id: u32 = 0,
    dep_id: u32 = HOIST_ROOT_DEP_ID,
    hoist_root_id: u32 = HOIST_INVALID_ID,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    for (i of 0..a.len) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn copySlice(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) {
        return err.BufferTooSmall;
    }
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn isDepOptionalPeer(flags: u8): bool {
    return (flags & HOIST_DEP_OPTIONAL) != 0 && (flags & HOIST_DEP_PEER) != 0;
}

fn isDepFilteredByOmit(flags: u8, omit: HoistOmitPolicy): bool {
    if (omit.omit_dev && (flags & HOIST_DEP_DEV) != 0) {
        return true;
    }
    if (omit.omit_optional && (flags & HOIST_DEP_OPTIONAL) != 0) {
        return true;
    }
    if (omit.omit_peer && (flags & HOIST_DEP_PEER) != 0) {
        return true;
    }
    return false;
}

/// 零堆分配 Node Modules 依赖提升树构建器（对齐 Bun `Lockfile.Tree.Builder`）
pub struct HoistTreeBuilder {
    pkgs: [HOIST_MAX_PACKAGES]HoistPkgRecord = [HoistPkgRecord{}; HOIST_MAX_PACKAGES],
    pkg_count: usize = 0,

    deps: [HOIST_MAX_DEPS]HoistDepRecord = [HoistDepRecord{}; HOIST_MAX_DEPS],
    resolutions: [HOIST_MAX_DEPS]u32 = [HOIST_INVALID_ID; HOIST_MAX_DEPS],
    dep_count: usize = 0,

    nodes: [HOIST_MAX_TREES]HoistNode = [HoistNode{}; HOIST_MAX_TREES],
    node_count: usize = 0,

    pending_peer_deps: [HOIST_MAX_DEPS]u32 = [HOIST_INVALID_ID; HOIST_MAX_DEPS],
    pending_peer_count: usize = 0,

    /// 初始化空构建器（pkg_id = 0 保留给项目根包，须先通过 `addPackage` 注册）
    static fn init(): HoistTreeBuilder {
        return HoistTreeBuilder{};
    }

    /// 注册一个包节点并返回其 `pkg_id`（首个注册的包 `id == 0` 作为项目根）
    fn addPackage(*_, name: []const u8, version: []const u8, disabled: bool): u32! {
        if (name.len == 0) {
            return err.EmptyName;
        }
        if (_.pkg_count >= HOIST_MAX_PACKAGES) {
            return err.CapacityExceeded;
        }
        const id = _.pkg_count as u32;
        _.pkgs[_.pkg_count] = HoistPkgRecord{
            id: id,
            name: name,
            version: version,
            disabled: disabled,
        };
        _.pkg_count += 1;
        return id;
    }

    /// 为指定父包添加一条依赖边，返回 `dep_id`
    fn addDependency(
        *_,
        parent_pkg_id: u32,
        name: []const u8,
        resolved_pkg_id: u32,
        flags: u8,
        peer_compatible: bool,
    ): u32! {
        if (name.len == 0) {
            return err.EmptyName;
        }
        if ((parent_pkg_id as usize) >= _.pkg_count) {
            return err.InvalidPackageId;
        }
        if (resolved_pkg_id != HOIST_INVALID_ID && (resolved_pkg_id as usize) >= _.pkg_count) {
            return err.InvalidPackageId;
        }
        if (_.dep_count >= HOIST_MAX_DEPS) {
            return err.CapacityExceeded;
        }
        const dep_id = _.dep_count as u32;
        _.deps[_.dep_count] = HoistDepRecord{
            dep_id: dep_id,
            parent_pkg_id: parent_pkg_id,
            name: name,
            resolved_pkg_id: resolved_pkg_id,
            flags: flags,
            peer_compatible: peer_compatible,
        };
        _.resolutions[_.dep_count] = resolved_pkg_id;
        _.dep_count += 1;
        return dep_id;
    }

    /// 检查某包是否具有出边依赖
    fn hasDependencies(pkg_id: u32): bool {
        for (i of 0.._.dep_count) {
            if (_.deps[i].parent_pkg_id == pkg_id) {
                return true;
            }
        }
        return false;
    }

    /// 将所有同名待解析 optional peer 回填为 `resolved_pkg_id`
    fn resolvePendingPeers(*_, dep_name: []const u8, resolved_pkg_id: u32) {
        let write_idx: usize = 0;
        for (i of 0.._.pending_peer_count) {
            const p_dep_id = _.pending_peer_deps[i];
            if (p_dep_id != HOIST_INVALID_ID && (p_dep_id as usize) < _.dep_count) {
                if (bytesEql(_.deps[p_dep_id as usize].name, dep_name)) {
                    _.resolutions[p_dep_id as usize] = resolved_pkg_id;
                    continue;
                }
            }
            _.pending_peer_deps[write_idx] = p_dep_id;
            write_idx += 1;
        }
        _.pending_peer_count = write_idx;
    }

    /// 核心单步提升判定（对齐 Bun `Tree.hoistDependency`）：
    /// 从当前子树向上遍历至根（或 `hoist_root_id`），返回去重/提升/放置/回填/环路决策。
    fn hoistDependency(
        start_tree_id: u32,
        hoist_root_id: u32,
        pkg_id: u32,
        dep: HoistDepRecord,
    ): HoistDecision! {
        let cur_id = start_tree_id;
        let as_defined = true;
        let last_valid_tree = start_tree_id;
        let depth_guard: usize = 0;

        while (cur_id != HOIST_INVALID_ID && (cur_id as usize) < _.node_count) {
            if (depth_guard >= HOIST_MAX_DEPTH) {
                return err.MaxDepthExceeded;
            }
            depth_guard += 1;

            const node = _.nodes[cur_id as usize];
            let conflict_here = false;

            for (i of 0..node.dep_count) {
                const existing_dep_id = node.deps[i];
                const existing_dep = _.deps[existing_dep_id as usize];
                if (!bytesEql(existing_dep.name, dep.name)) {
                    continue;
                }

                const existing_res = _.resolutions[existing_dep_id as usize];

                if (existing_res == HOIST_INVALID_ID && pkg_id == HOIST_INVALID_ID) {
                    return HoistDecision{ kind: .ResolveLater };
                }

                if (existing_res == HOIST_INVALID_ID) {
                    return HoistDecision{
                        kind: .ResolveReplace,
                        tree_id: cur_id,
                        dep_id: existing_dep_id,
                    };
                }

                if (pkg_id == HOIST_INVALID_ID) {
                    return HoistDecision{
                        kind: .ResolveOptionalPeer,
                        pkg_id: existing_res,
                    };
                }

                if (existing_res == pkg_id) {
                    return HoistDecision{ kind: .Hoisted };
                }

                if (as_defined) {
                    const existing_is_dev = (existing_dep.flags & HOIST_DEP_DEV) != 0;
                    const dep_is_dev = (dep.flags & HOIST_DEP_DEV) != 0;
                    if (existing_is_dev != dep_is_dev) {
                        return HoistDecision{ kind: .Hoisted };
                    }
                }

                if ((dep.flags & HOIST_DEP_PEER) != 0) {
                    if (dep.peer_compatible || (existing_dep.flags & HOIST_DEP_WORKSPACE_ROOT) != 0) {
                        return HoistDecision{ kind: .Hoisted };
                    }
                }

                if (as_defined && (existing_dep.flags & HOIST_DEP_PEER) == 0) {
                    return err.DependencyLoop;
                }

                // 在祖先层发生版本冲突：停止继续向上提升，退回至当前待放置目录
                conflict_here = true;
                break;
            }

            if (conflict_here) {
                return HoistDecision{
                    kind: .Placed,
                    tree_id: start_tree_id,
                    bundled: false,
                };
            }

            last_valid_tree = cur_id;
            if (node.parent == HOIST_INVALID_ID || cur_id == hoist_root_id) {
                break;
            }
            cur_id = node.parent;
            as_defined = false;
        }

        return HoistDecision{
            kind: .Placed,
            tree_id: last_valid_tree,
            bundled: false,
        };
    }

    /// 执行完整 BFS 依赖提升树构建，返回生成的有效子树数量
    fn build(*_, mode: HoistBuildMode, omit: HoistOmitPolicy): usize! {
        if (_.pkg_count == 0) {
            return err.InvalidPackageId;
        }

        _.node_count = 0;
        _.pending_peer_count = 0;
        for (i of 0.._.dep_count) {
            _.resolutions[i] = _.deps[i].resolved_pkg_id;
        }

        let queue: [HOIST_MAX_DEPS]HoistQueueItem = [HoistQueueItem{}; HOIST_MAX_DEPS];
        let q_head: usize = 0;
        let q_tail: usize = 0;

        queue[q_tail] = HoistQueueItem{
            tree_id: HOIST_INVALID_ID,
            dep_id: HOIST_ROOT_DEP_ID,
            hoist_root_id: HOIST_INVALID_ID,
        };
        q_tail += 1;

        while (q_head < q_tail) {
            const item = queue[q_head];
            q_head += 1;

            const parent_pkg_id: u32 = if (item.dep_id == HOIST_ROOT_DEP_ID)
                0
            else
                _.resolutions[item.dep_id as usize];

            if (parent_pkg_id == HOIST_INVALID_ID || (parent_pkg_id as usize) >= _.pkg_count) {
                continue;
            }
            if (!_.hasDependencies(parent_pkg_id)) {
                continue;
            }

            if (_.node_count >= HOIST_MAX_TREES) {
                return err.CapacityExceeded;
            }

            const next_id = _.node_count as u32;
            const folder_name: []const u8 = if (item.dep_id == HOIST_ROOT_DEP_ID)
                ""
            else
                _.deps[item.dep_id as usize].name;

            _.nodes[_.node_count] = HoistNode{
                id: next_id,
                parent: item.tree_id,
                dep_id: item.dep_id,
                folder_name: folder_name,
                dep_count: 0,
            };
            _.node_count += 1;

            for (dep_idx of 0.._.dep_count) {
                const dep = _.deps[dep_idx];
                if (dep.parent_pkg_id != parent_pkg_id) {
                    continue;
                }
                const dep_id = dep.dep_id;
                const pkg_id = _.resolutions[dep_idx];

                if (mode == .Filter) {
                    if (isDepFilteredByOmit(dep.flags, omit)) {
                        continue;
                    }
                    if (pkg_id == HOIST_INVALID_ID) {
                        continue;
                    }
                    if (_.pkgs[pkg_id as usize].disabled) {
                        continue;
                    }
                }

                let decision = HoistDecision{ kind: .Hoisted };

                if ((dep.flags & HOIST_DEP_BUNDLED) != 0) {
                    decision = HoistDecision{
                        kind: .Placed,
                        tree_id: next_id,
                        bundled: true,
                    };
                } else if (pkg_id == HOIST_INVALID_ID) {
                    if (isDepOptionalPeer(dep.flags)) {
                        decision = _.hoistDependency(next_id, item.hoist_root_id, pkg_id, dep)?;
                    } else {
                        continue;
                    }
                } else if ((dep.flags & HOIST_DEP_FOLDER) != 0) {
                    decision = HoistDecision{
                        kind: .Placed,
                        tree_id: next_id,
                        bundled: false,
                    };
                } else {
                    decision = _.hoistDependency(next_id, item.hoist_root_id, pkg_id, dep)?;
                }

                switch (decision.kind) {
                    .Hoisted, .DependencyLoop => {},
                    .ResolveOptionalPeer => {
                        _.resolutions[dep_idx] = decision.pkg_id;
                        _.resolvePendingPeers(dep.name, decision.pkg_id);
                    },
                    .ResolveReplace => {
                        _.resolutions[decision.dep_id as usize] = pkg_id;
                        _.resolvePendingPeers(dep.name, pkg_id);

                        const rep_tree = decision.tree_id as usize;
                        for (k of 0.._.nodes[rep_tree].dep_count) {
                            if (_.nodes[rep_tree].deps[k] == decision.dep_id) {
                                _.nodes[rep_tree].deps[k] = dep_id;
                            }
                        }
                        if (pkg_id != HOIST_INVALID_ID && _.hasDependencies(pkg_id)) {
                            if (q_tail >= HOIST_MAX_DEPS) {
                                return err.CapacityExceeded;
                            }
                            queue[q_tail] = HoistQueueItem{
                                tree_id: decision.tree_id,
                                dep_id: dep_id,
                                hoist_root_id: item.hoist_root_id,
                            };
                            q_tail += 1;
                        }
                    },
                    .ResolveLater => {
                        if (_.pending_peer_count >= HOIST_MAX_DEPS) {
                            return err.CapacityExceeded;
                        }
                        _.pending_peer_deps[_.pending_peer_count] = dep_id;
                        _.pending_peer_count += 1;
                    },
                    .Placed => {
                        const dest = decision.tree_id as usize;
                        if (_.nodes[dest].dep_count >= HOIST_MAX_DEPS_PER_TREE) {
                            return err.CapacityExceeded;
                        }
                        _.nodes[dest].deps[_.nodes[dest].dep_count] = dep_id;
                        _.nodes[dest].dep_count += 1;

                        if (pkg_id != HOIST_INVALID_ID && _.hasDependencies(pkg_id)) {
                            if (q_tail >= HOIST_MAX_DEPS) {
                                return err.CapacityExceeded;
                            }
                            queue[q_tail] = HoistQueueItem{
                                tree_id: decision.tree_id,
                                dep_id: dep_id,
                                hoist_root_id: if (decision.bundled) decision.tree_id else item.hoist_root_id,
                            };
                            q_tail += 1;
                        }
                    },
                }
            }

            // 若当前非根子树未保留任何直接依赖（全部被提升或过滤），弹出该空节点
            if (next_id > 0 && _.nodes[next_id as usize].dep_count == 0) {
                _.node_count -= 1;
            }
        }

        return _.node_count;
    }

    /// 返回当前构建出的子树节点总数
    fn treeCount(): usize {
        return _.node_count;
    }

    /// 获取指定子树节点快照
    fn treeAt(tree_id: u32): HoistNode! {
        if ((tree_id as usize) >= _.node_count) {
            return err.InvalidTreeId;
        }
        return _.nodes[tree_id as usize];
    }

    /// 获取指定子树下直接放置的依赖数
    fn treeDepCount(tree_id: u32): usize! {
        if ((tree_id as usize) >= _.node_count) {
            return err.InvalidTreeId;
        }
        return _.nodes[tree_id as usize].dep_count;
    }

    /// 获取指定子树下第 `idx` 条依赖的 `dep_id`
    fn treeDepAt(tree_id: u32, idx: usize): u32! {
        if ((tree_id as usize) >= _.node_count) {
            return err.InvalidTreeId;
        }
        if (idx >= _.nodes[tree_id as usize].dep_count) {
            return err.InvalidDepId;
        }
        return _.nodes[tree_id as usize].deps[idx];
    }

    /// 获取依赖边在提升后的最终解析包 ID（可观测 optional peer 回填结果）
    fn resolvedPkg(dep_id: u32): u32! {
        if ((dep_id as usize) >= _.dep_count) {
            return err.InvalidDepId;
        }
        return _.resolutions[dep_id as usize];
    }

    /// 格式化子树相对路径并返回目录深度（对齐 Bun `Tree.relativePathAndDepth`）
    /// - `NodeModules`: 根 -> `"node_modules"` (depth 0)，子树 -> `"node_modules/a/node_modules/b/node_modules"` (depth 2)
    /// - `PkgPath`: 根 -> `""` (depth 0)，子树 -> `"a/b"` (depth 2)
    fn formatRelativePath(tree_id: u32, style: HoistPathStyle, out: []u8): HoistPathResult! {
        if ((tree_id as usize) >= _.node_count) {
            return err.InvalidTreeId;
        }

        let written: usize = 0;
        if (style == .NodeModules) {
            written = copySlice(out, written, "node_modules")?;
        }

        let chain: [HOIST_MAX_DEPTH]u32 = [0; HOIST_MAX_DEPTH];
        let chain_len: usize = 0;
        let cur_id = tree_id;

        while (cur_id > 0 && (cur_id as usize) < _.node_count) {
            if (chain_len >= HOIST_MAX_DEPTH) {
                return err.MaxDepthExceeded;
            }
            chain[chain_len] = cur_id;
            chain_len += 1;
            const parent_id = _.nodes[cur_id as usize].parent;
            if (parent_id == HOIST_INVALID_ID) {
                break;
            }
            cur_id = parent_id;
        }

        let depth = chain_len;
        let rev = chain_len;
        while (rev > 0) {
            rev -= 1;
            const nid = chain[rev];
            const fname = _.nodes[nid as usize].folder_name;

            if (style == .PkgPath) {
                if (rev + 1 != depth) {
                    written = copySlice(out, written, "/")?;
                }
                written = copySlice(out, written, fname)?;
            } else {
                written = copySlice(out, written, "/")?;
                written = copySlice(out, written, fname)?;
                written = copySlice(out, written, "/node_modules")?;
            }
        }

        return HoistPathResult{
            path_len: written,
            depth: depth,
        };
    }

    /// 格式化某个包在指定子树中的安装路径（如 `"node_modules/a/node_modules/lodash"`）
    fn formatPackageInstallPath(tree_id: u32, pkg_name: []const u8, out: []u8): usize! {
        if (pkg_name.len == 0) {
            return err.EmptyName;
        }
        const base = _.formatRelativePath(tree_id, .NodeModules, out)?;
        let written = copySlice(out, base.path_len, "/")?;
        written = copySlice(out, written, pkg_name)?;
        return written;
    }
}
