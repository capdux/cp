// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/findImportedCSSFilesInJSOrder.cp | Synced: 2026-11-22T05:26:00Z
// ============================================================================
// Bundler CSS Import Ordering in JavaScript DFS Postorder
// Absorbed from Bun: src/bundler/linker_context/findImportedCSSFilesInJSOrder.cp
// ============================================================================

/// CSS 导入顺序图构建与遍历错误集
pub err CssOrderError {
    OutOfMemory,
    SourceIndexOutOfRange,
}

/// 模块加载器分类（区分 JS/TS 模块、CSS 样式表与其他资源）
pub enum CssOrderLoaderKind: u8 {
    js,
    css,
    other,

    fn isCss(): bool {
        return _ == CssOrderLoaderKind.css;
    }
}

struct PostorderFrame {
    node: u32,
    next_edge: usize,
    is_css: bool,
}

/// 模块导入顺序图与 JS 深度优先后序（DFS Postorder）CSS 收集器
///
/// 遵循 ESM / Bun 静态求值语义：
/// 若 `A` 依次导入 `B`、`C`，且 `B`、`C` 均导入 `D`（菱形依赖），
/// 则深度优先后序遍历顺序为 `D -> B -> C -> A`，确保基础样式 `D` 始终先于上层样式 `B`/`C` 生效。
/// 内部使用显式迭代栈 + `visited` 表，完全环路安全且无深层递归栈溢出风险。
pub struct CssOrderGraph {
    allocator: std.mem.Allocator,
    loaders: std.ArrayListUnmanaged(CssOrderLoaderKind) = .{},
    imports: std.ArrayListUnmanaged(std.ArrayListUnmanaged(u32)) = .{},

    static fn init(allocator: std.mem.Allocator): CssOrderGraph {
        return CssOrderGraph{
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
        while (i < _.imports.items.len) {
            _.imports.items[i].deinit(_.allocator);
            i += 1;
        }
        _.imports.deinit(_.allocator);
        _.imports = .{};

        _.loaders.deinit(_.allocator);
        _.loaders = .{};
    }

    /// 注册一个文件节点并返回其 `source_index`
    fn addFile(*_, loader: CssOrderLoaderKind): u32! {
        const idx: u32 = @intCast(_.loaders.items.len);
        _.loaders.append(_.allocator, loader) catch {
            return err.OutOfMemory;
        };
        _.imports.append(_.allocator, .{}) catch {
            * = _.loaders.pop();
            return err.OutOfMemory;
        };
        return idx;
    }

    /// 按源码声明顺序添加有向导入边 `from_idx -> to_idx`
    fn addImport(*_, from_idx: u32, to_idx: u32): ! {
        if (from_idx >= _.loaders.items.len || to_idx >= _.loaders.items.len) {
            return err.SourceIndexOutOfRange;
        }
        _.imports.items[from_idx].append(_.allocator, to_idx) catch {
            return err.OutOfMemory;
        };
    }

    /// 返回图中已注册文件总数
    fn fileCount(): usize {
        return _.loaders.items.len;
    }

    /// 按 JS 深度优先后序收集从 `entry_point` 可达的全部被导入 CSS 文件索引（不含入口自身）
    fn findImportedCssInJsOrder(
        out_alloc: std.mem.Allocator,
        entry_point: u32,
    ): []u32! {
        if (entry_point >= _.loaders.items.len) {
            return err.SourceIndexOutOfRange;
        }
        const entries: [1]u32 = [entry_point];
        return _.collectPostorder(out_alloc, entries[0..], true);
    }

    /// 按多入口共享 `visited` 位表收集全部被导入 CSS 文件索引（零重复、保持首次触达级联序）
    fn findImportedCssForEntries(
        out_alloc: std.mem.Allocator,
        entries: []const u32,
    ): []u32! {
        let i: usize = 0;
        while (i < entries.len) {
            if (entries[i] >= _.loaders.items.len) {
                return err.SourceIndexOutOfRange;
            }
            i += 1;
        }
        return _.collectPostorder(out_alloc, entries, true);
    }

    /// 返回从 `entry_point` 可达的全部模块索引的 JS 深度优先后序列表（含入口自身）
    fn findAllInJsPostorder(
        out_alloc: std.mem.Allocator,
        entry_point: u32,
    ): []u32! {
        if (entry_point >= _.loaders.items.len) {
            return err.SourceIndexOutOfRange;
        }
        const entries: [1]u32 = [entry_point];
        return _.collectPostorder(out_alloc, entries[0..], false);
    }

    fn collectPostorder(
        out_alloc: std.mem.Allocator,
        entries: []const u32,
        css_only: bool,
    ): []u32! {
        const n = _.loaders.items.len;
        let order: std.ArrayListUnmanaged(u32) = .{};
        errdefer order.deinit(out_alloc);

        if (n == 0 || entries.len == 0) {
            return order.toOwnedSlice(out_alloc) catch {
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

        let stack: std.ArrayListUnmanaged(PostorderFrame) = .{};
        defer stack.deinit(out_alloc);

        let ei: usize = 0;
        while (ei < entries.len) {
            const root = entries[ei];
            ei += 1;
            if (visited[root]) {
                continue;
            }
            visited[root] = true;
            // 对齐 Bun 原版：入口节点本身传 is_css = false（仅收集被导入的 CSS）
            stack.append(out_alloc, PostorderFrame{
                node: root,
                next_edge: 0,
                is_css: false,
            }) catch {
                return err.OutOfMemory;
            };

            while (stack.items.len > 0) {
                const top_idx = stack.items.len - 1;
                const cur_node = stack.items[top_idx].node;
                const edge_idx = stack.items[top_idx].next_edge;
                const edges = _.imports.items[cur_node].items;

                if (edge_idx < edges.len) {
                    stack.items[top_idx].next_edge = edge_idx + 1;
                    const dep = edges[edge_idx];
                    if (!visited[dep]) {
                        visited[dep] = true;
                        stack.append(out_alloc, PostorderFrame{
                            node: dep,
                            next_edge: 0,
                            is_css: _.loaders.items[dep].isCss(),
                        }) catch {
                            return err.OutOfMemory;
                        };
                    }
                } else {
                    const done = stack.pop() ?? break;
                    if (!css_only || done.is_css) {
                        order.append(out_alloc, done.node) catch {
                            return err.OutOfMemory;
                        };
                    }
                }
            }
        }

        return order.toOwnedSlice(out_alloc) catch {
            return err.OutOfMemory;
        };
    }
}
