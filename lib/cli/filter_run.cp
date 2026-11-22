// Source: Absorbed from /data/dev/bun/src/cli/filter_run.cp | Synced: 2026-11-22T10:21:35Z
// Ported from Bun: /data/dev/bun/src/cli/filter_run.cp & /data/dev/bun/docs/pm/filter.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation monorepo `--filter` / `--workspaces` script execution scheduler:
// topological dependency DAG, iterative 3-color cycle detection & cycle-break fallback,
// pre/main/post lifecycle chaining, parallel/sequential/topological modes,
// output tail-line elision (`elideScriptOutput`), exit-code aggregation, and box-drawing formatter.

/// 单个调度图支持的最大脚本节点数量
pub const FILTER_RUN_MAX_SCRIPTS: usize = 64;

/// 单个调度图支持的最大依赖边数量
pub const FILTER_RUN_MAX_EDGES: usize = 256;

/// 默认输出折叠保留行数（对齐 Bun `filter_run.cp` 默认值 10）
pub const FILTER_RUN_DEFAULT_ELIDE_LINES: usize = 10;

/// POSIX SIGINT 信号编号
pub const FILTER_RUN_SIGINT: u8 = 2;

/// `--filter` 脚本调度与格式化错误集
pub err FilterRunError {
    TooManyScripts,
    TooManyEdges,
    InvalidScriptIndex,
    EmptyScriptName,
    BufferTooSmall,
    InvalidStateTransition,
}

/// 生命周期阶段：`pre<script>`、`<script>`、`post<script>`
pub enum FilterScriptPhase: u8 {
    Pre = 0,
    Main = 1,
    Post = 2,
}

/// 调度模式：拓扑并行（默认）、纯并行或顺序执行
pub enum FilterRunMode: u8 {
    Topological = 0,
    Parallel = 1,
    Sequential = 2,
}

/// 脚本节点生命周期状态
pub enum FilterScriptStatus: u8 {
    Pending = 0,
    Running = 1,
    Exited = 2,
    Signaled = 3,
    Errored = 4,
    Aborted = 5,
}

/// 输出折叠结果：保留的末尾切片与被折叠的行数
pub struct FilterElideResult {
    content: []const u8 = "",
    elided_count: usize = 0,
}

/// 单个工作区脚本调度节点
pub struct FilterScriptNode {
    package_name: []const u8 = "",
    script_name: []const u8 = "",
    script_content: []const u8 = "",
    phase: FilterScriptPhase = FilterScriptPhase.Main,
    status: FilterScriptStatus = FilterScriptStatus.Pending,
    exit_code: u8 = 0,
    signal_code: u8 = 0,
    duration_ns: u64 = 0,
    remaining_deps: usize = 0,
    elide_lines: usize? = FILTER_RUN_DEFAULT_ELIDE_LINES,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendBytes(out: []u8, pos: *usize, src: []const u8): void! {
    if (pos.* + src.len > out.len) {
        return FilterRunError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn appendByte(out: []u8, pos: *usize, b: u8): void! {
    if (pos.* + 1 > out.len) {
        return FilterRunError.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

fn appendU64(out: []u8, pos: *usize, val_in: u64): void! {
    if (val_in == 0) {
        appendByte(out, pos, '0')?;
        return;
    }
    let rev: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val_in;
    while (v > 0) {
        rev[n] = ('0' as u8) + ((v % 10) as u8);
        v /= 10;
        n += 1;
    }
    if (pos.* + n > out.len) {
        return FilterRunError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = rev[n - 1 - i];
        i += 1;
    }
    pos.* += n;
}

/// 严格对齐 Bun `filter_run.elide`：
/// 1. 若末尾存在单个 `\n`，先剥离该尾部换行符；
/// 2. 若 `max_lines == null` 或 `max_lines == 0`，不折叠，返回全文且 `elided_count = 0`；
/// 3. 否则自后向前保留最多 `max_lines` 行，并统计之前被折叠的行数。
pub fn elideScriptOutput(data_in: []const u8, max_lines: usize?): FilterElideResult {
    let data = data_in;
    if (data.len == 0) {
        return FilterElideResult{ content: "", elided_count: 0 };
    }
    if (data[data.len - 1] == '\n') {
        data = data[0 .. data.len - 1];
    }
    let limit: usize = 0;
    if (ml := max_lines) {
        limit = ml;
    } else {
        return FilterElideResult{ content: data, elided_count: 0 };
    }
    if (limit == 0) {
        return FilterElideResult{ content: data, elided_count: 0 };
    }

    let i: usize = data.len;
    let lines: usize = 0;
    while (i > 0) {
        if (data[i - 1] == '\n') {
            lines += 1;
            if (lines >= limit) {
                break;
            }
        }
        i -= 1;
    }
    const content = if (i >= data.len) "" else data[i..];
    let elided: usize = 0;
    let j: usize = i;
    while (j > 0) {
        if (data[j - 1] == '\n') {
            elided += 1;
        }
        j -= 1;
    }
    return FilterElideResult{ content: content, elided_count: elided };
}

/// 生成生命周期脚本名称（`pre<script>` / `<script>` / `post<script>`）
pub fn formatLifecycleScriptName(script_name: []const u8, phase: FilterScriptPhase, out: []u8): usize! {
    if (script_name.len == 0) {
        return FilterRunError.EmptyScriptName;
    }
    let pos: usize = 0;
    switch (phase) {
        FilterScriptPhase.Pre => {
            appendBytes(out, &pos, "pre")?;
        },
        FilterScriptPhase.Main => {},
        FilterScriptPhase.Post => {
            appendBytes(out, &pos, "post")?;
        },
    }
    appendBytes(out, &pos, script_name)?;
    return pos;
}

/// 格式化单次脚本执行耗时（对齐 Bun `filter_run.redraw`：`> 1000 ms` 显示 2 位小数秒，否则整数毫秒）
pub fn formatFilterDuration(duration_ns: u64, out: []u8): usize! {
    let pos: usize = 0;
    if (duration_ns == 0) {
        appendBytes(out, &pos, "Done")?;
        return pos;
    }
    appendBytes(out, &pos, "Done in ")?;
    if (duration_ns > 1_000_000_000) {
        // 四舍五入到百分之一秒（10_000_000 ns）
        const centis = (duration_ns + 5_000_000) / 10_000_000;
        const secs = centis / 100;
        const frac = centis % 100;
        appendU64(out, &pos, secs)?;
        appendByte(out, &pos, '.')?;
        appendByte(out, &pos, ('0' as u8) + ((frac / 10) as u8))?;
        appendByte(out, &pos, ('0' as u8) + ((frac % 10) as u8))?;
        appendBytes(out, &pos, " s")?;
    } else {
        const ms = (duration_ns + 500_000) / 1_000_000;
        appendU64(out, &pos, ms)?;
        appendBytes(out, &pos, " ms")?;
    }
    return pos;
}

/// 格式化脚本底部状态栏文本（对齐 Bun `filter_run.redraw` 语义）
pub fn formatFilterStatusLine(node: FilterScriptNode, out: []u8): usize! {
    let pos: usize = 0;
    switch (node.status) {
        FilterScriptStatus.Pending => {
            appendBytes(out, &pos, "Waiting for ")?;
            appendU64(out, &pos, node.remaining_deps as u64)?;
            appendBytes(out, &pos, " other script(s)")?;
        },
        FilterScriptStatus.Running => {
            appendBytes(out, &pos, "Running...")?;
        },
        FilterScriptStatus.Exited => {
            if (node.exit_code == 0) {
                const dn = formatFilterDuration(node.duration_ns, out[pos..])?;
                pos += dn;
            } else {
                appendBytes(out, &pos, "Exited with code ")?;
                appendU64(out, &pos, node.exit_code as u64)?;
            }
        },
        FilterScriptStatus.Signaled => {
            if (node.signal_code == FILTER_RUN_SIGINT) {
                appendBytes(out, &pos, "Interrupted")?;
            } else {
                appendBytes(out, &pos, "Signaled with code ")?;
                appendU64(out, &pos, node.signal_code as u64)?;
            }
        },
        FilterScriptStatus.Errored => {
            appendBytes(out, &pos, "Error")?;
        },
        FilterScriptStatus.Aborted => {
            appendBytes(out, &pos, "Aborted")?;
        },
    }
    return pos;
}

/// 渲染单个脚本的终端折叠输出块（无 ANSI 转义，便于日志记录或叠加配色）
pub fn formatFilterOutputBlock(
    node: FilterScriptNode,
    raw_output: []const u8,
    is_abort: bool,
    out: []u8,
): usize! {
    let pos: usize = 0;
    appendBytes(out, &pos, node.package_name)?;
    appendByte(out, &pos, ' ')?;
    appendBytes(out, &pos, node.script_name)?;
    appendBytes(out, &pos, " $ ")?;
    appendBytes(out, &pos, node.script_content)?;
    appendByte(out, &pos, '\n')?;

    const max_lines: usize? = if (is_abort) null else node.elide_lines;
    const e = elideScriptOutput(raw_output, max_lines);
    if (e.elided_count > 0) {
        appendBytes(out, &pos, "│ [")?;
        appendU64(out, &pos, e.elided_count as u64)?;
        appendBytes(out, &pos, " lines elided]\n")?;
    }

    let rem = e.content;
    while (rem.len > 0) {
        let nl: usize = 0;
        let found = false;
        while (nl < rem.len) {
            if (rem[nl] == '\n') {
                found = true;
                break;
            }
            nl += 1;
        }
        appendBytes(out, &pos, "│ ")?;
        if (found) {
            appendBytes(out, &pos, rem[0 .. nl + 1])?;
            rem = rem[nl + 1 ..];
        } else {
            appendBytes(out, &pos, rem)?;
            appendByte(out, &pos, '\n')?;
            break;
        }
    }

    appendBytes(out, &pos, "└─ ")?;
    const sn = formatFilterStatusLine(node, out[pos..])?;
    pos += sn;
    appendByte(out, &pos, '\n')?;
    return pos;
}

/// 零堆分配 Monorepo 工作区脚本 DAG 调度器
pub struct FilterRunGraph {
    nodes: [FILTER_RUN_MAX_SCRIPTS]FilterScriptNode = [FilterScriptNode{}; FILTER_RUN_MAX_SCRIPTS],
    node_count: usize = 0,
    edge_from: [FILTER_RUN_MAX_EDGES]u16 = [0; FILTER_RUN_MAX_EDGES],
    edge_to: [FILTER_RUN_MAX_EDGES]u16 = [0; FILTER_RUN_MAX_EDGES],
    edge_count: usize = 0,
    cycle_detected: bool = false,
    aborted: bool = false,
    exit_on_error: bool = true,
    mode: FilterRunMode = FilterRunMode.Topological,

    static fn init(mode: FilterRunMode, exit_on_error: bool): FilterRunGraph {
        return FilterRunGraph{
            mode: mode,
            exit_on_error: exit_on_error,
        };
    }

    /// 注册一个待调度的工作区脚本节点，返回其索引
    fn addScript(
        *_,
        package_name: []const u8,
        script_name: []const u8,
        script_content: []const u8,
        phase: FilterScriptPhase,
    ): usize! {
        if (script_name.len == 0) {
            return FilterRunError.EmptyScriptName;
        }
        if (_.node_count >= FILTER_RUN_MAX_SCRIPTS) {
            return FilterRunError.TooManyScripts;
        }
        const idx = _.node_count;
        _.nodes[idx] = FilterScriptNode{
            package_name: package_name,
            script_name: script_name,
            script_content: script_content,
            phase: phase,
            status: FilterScriptStatus.Pending,
            exit_code: 0,
            signal_code: 0,
            duration_ns: 0,
            remaining_deps: 0,
            elide_lines: FILTER_RUN_DEFAULT_ELIDE_LINES,
        };
        _.node_count += 1;
        return idx;
    }

    /// 判断边 `dep_idx -> dependent_idx` 是否已存在
    fn hasEdge(dep_idx: usize, dependent_idx: usize): bool {
        let i: usize = 0;
        while (i < _.edge_count) {
            if ((_.edge_from[i] as usize) == dep_idx && (_.edge_to[i] as usize) == dependent_idx) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 添加有向依赖边：`dep_idx` 必须在 `dependent_idx` 之前完成（自动去重）
    fn addDependency(*_, dep_idx: usize, dependent_idx: usize): void! {
        if (dep_idx >= _.node_count || dependent_idx >= _.node_count) {
            return FilterRunError.InvalidScriptIndex;
        }
        if (_.hasEdge(dep_idx, dependent_idx)) {
            return;
        }
        if (_.edge_count >= FILTER_RUN_MAX_EDGES) {
            return FilterRunError.TooManyEdges;
        }
        _.edge_from[_.edge_count] = dep_idx as u16;
        _.edge_to[_.edge_count] = dependent_idx as u16;
        _.edge_count += 1;
        _.nodes[dependent_idx].remaining_deps += 1;
    }

    /// 按包名建立跨包依赖：`dep_package_name` 下所有脚本均先于 `dependent_package_name` 下所有脚本执行
    fn addPackageDependency(*_, dep_package_name: []const u8, dependent_package_name: []const u8): usize! {
        let added: usize = 0;
        let i: usize = 0;
        while (i < _.node_count) {
            if (bytesEql(_.nodes[i].package_name, dep_package_name)) {
                let j: usize = 0;
                while (j < _.node_count) {
                    if (bytesEql(_.nodes[j].package_name, dependent_package_name)) {
                        if (!_.hasEdge(i, j)) {
                            _.addDependency(i, j)?;
                            added += 1;
                        }
                    }
                    j += 1;
                }
            }
            i += 1;
        }
        return added;
    }

    /// 迭代式三色 DFS 环检测（0=未访问，1=访问中，2=已完成），零递归免疫深链栈溢出
    fn hasCycle(): bool {
        let color: [FILTER_RUN_MAX_SCRIPTS]u8 = [0; FILTER_RUN_MAX_SCRIPTS];
        let stack_node: [FILTER_RUN_MAX_SCRIPTS]u16 = [0; FILTER_RUN_MAX_SCRIPTS];
        let stack_edge: [FILTER_RUN_MAX_SCRIPTS]u16 = [0; FILTER_RUN_MAX_SCRIPTS];

        let start: usize = 0;
        while (start < _.node_count) {
            if (color[start] != 0) {
                start += 1;
                continue;
            }
            let top: usize = 0;
            stack_node[0] = start as u16;
            stack_edge[0] = 0;
            color[start] = 1;
            top = 1;

            while (top > 0) {
                const cur = stack_node[top - 1] as usize;
                let ei = stack_edge[top - 1] as usize;
                let pushed = false;
                while (ei < _.edge_count) {
                    const from_idx = _.edge_from[ei] as usize;
                    const to_idx = _.edge_to[ei] as usize;
                    ei += 1;
                    if (from_idx != cur) {
                        continue;
                    }
                    if (color[to_idx] == 1) {
                        return true;
                    }
                    if (color[to_idx] == 0) {
                        stack_edge[top - 1] = ei as u16;
                        color[to_idx] = 1;
                        stack_node[top] = to_idx as u16;
                        stack_edge[top] = 0;
                        top += 1;
                        pushed = true;
                        break;
                    }
                }
                if (!pushed) {
                    color[cur] = 2;
                    top -= 1;
                }
            }
            start += 1;
        }
        return false;
    }

    fn clearAllEdges(*_) {
        _.edge_count = 0;
        let i: usize = 0;
        while (i < _.node_count) {
            _.nodes[i].remaining_deps = 0;
            i += 1;
        }
    }

    /// 完成依赖解析：
    /// 1. 若为 `Parallel` 模式，清除跨包边；
    /// 2. 若为 `Topological` 模式且检测到环，记录 `cycle_detected = true` 并清除跨包边回退为无依赖并行（严格对齐 Bun）；
    /// 3. 始终串接同包相邻生命周期脚本（`pre` -> `main` -> `post`）；
    /// 4. 若为 `Sequential` 模式，按注册顺序全链串接。
    /// 返回是否检测到跨包依赖环。
    fn resolveDependencies(*_): bool! {
        _.cycle_detected = false;
        if (_.mode == FilterRunMode.Parallel) {
            _.clearAllEdges();
        } else if (_.mode == FilterRunMode.Sequential) {
            _.clearAllEdges();
            if (_.node_count > 1) {
                let i: usize = 0;
                while (i + 1 < _.node_count) {
                    _.addDependency(i, i + 1)?;
                    i += 1;
                }
            }
            return false;
        } else {
            if (_.hasCycle()) {
                _.cycle_detected = true;
                _.clearAllEdges();
            }
        }

        // 同包相邻脚本串行约束（pre -> main -> post）
        if (_.node_count > 1) {
            let i: usize = 0;
            while (i + 1 < _.node_count) {
                if (bytesEql(_.nodes[i].package_name, _.nodes[i + 1].package_name)) {
                    _.addDependency(i, i + 1)?;
                }
                i += 1;
            }
        }
        return _.cycle_detected;
    }

    /// 收集当前所有满足 `Pending && remaining_deps == 0` 的就绪脚本索引
    fn collectReady(out_indices: []usize): usize {
        if (_.aborted) {
            return 0;
        }
        if (_.mode == FilterRunMode.Sequential && _.runningCount() > 0) {
            return 0;
        }
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.node_count) {
            if (_.nodes[i].status == FilterScriptStatus.Pending && _.nodes[i].remaining_deps == 0) {
                if (count < out_indices.len) {
                    out_indices[count] = i;
                    count += 1;
                    if (_.mode == FilterRunMode.Sequential) {
                        break;
                    }
                }
            }
            i += 1;
        }
        return count;
    }

    /// 将指定就绪脚本标记为 `Running`
    fn markStarted(*_, idx: usize): void! {
        if (idx >= _.node_count) {
            return FilterRunError.InvalidScriptIndex;
        }
        if (_.nodes[idx].status != FilterScriptStatus.Pending || _.nodes[idx].remaining_deps != 0) {
            return FilterRunError.InvalidStateTransition;
        }
        _.nodes[idx].status = FilterScriptStatus.Running;
    }

    fn releaseDependents(*_, idx: usize) {
        let i: usize = 0;
        while (i < _.edge_count) {
            if ((_.edge_from[i] as usize) == idx) {
                const dep_to = _.edge_to[i] as usize;
                if (_.nodes[dep_to].remaining_deps > 0) {
                    _.nodes[dep_to].remaining_deps -= 1;
                }
            }
            i += 1;
        }
    }

    /// 标记脚本正常/异常退出，释放下游依赖；若非零退出且 `exit_on_error == true` 则触发全局中止
    fn markExited(*_, idx: usize, exit_code: u8, duration_ns: u64): void! {
        if (idx >= _.node_count) {
            return FilterRunError.InvalidScriptIndex;
        }
        if (_.nodes[idx].status != FilterScriptStatus.Running) {
            return FilterRunError.InvalidStateTransition;
        }
        _.nodes[idx].status = FilterScriptStatus.Exited;
        _.nodes[idx].exit_code = exit_code;
        _.nodes[idx].duration_ns = duration_ns;
        _.releaseDependents(idx);
        if (exit_code != 0 && _.exit_on_error) {
            _.abort();
        }
    }

    /// 标记脚本被信号终止
    fn markSignaled(*_, idx: usize, signal_code: u8): void! {
        if (idx >= _.node_count) {
            return FilterRunError.InvalidScriptIndex;
        }
        if (_.nodes[idx].status != FilterScriptStatus.Running) {
            return FilterRunError.InvalidStateTransition;
        }
        _.nodes[idx].status = FilterScriptStatus.Signaled;
        _.nodes[idx].signal_code = signal_code;
        _.releaseDependents(idx);
        if (_.exit_on_error) {
            _.abort();
        }
    }

    /// 标记脚本启动或 I/O 失败
    fn markErrored(*_, idx: usize): void! {
        if (idx >= _.node_count) {
            return FilterRunError.InvalidScriptIndex;
        }
        if (_.nodes[idx].status != FilterScriptStatus.Running) {
            return FilterRunError.InvalidStateTransition;
        }
        _.nodes[idx].status = FilterScriptStatus.Errored;
        _.releaseDependents(idx);
        if (_.exit_on_error) {
            _.abort();
        }
    }

    /// 触发中止：将所有尚未开始的 `Pending` 脚本标记为 `Aborted`
    fn abort(*_) {
        _.aborted = true;
        let i: usize = 0;
        while (i < _.node_count) {
            if (_.nodes[i].status == FilterScriptStatus.Pending) {
                _.nodes[i].status = FilterScriptStatus.Aborted;
            }
            i += 1;
        }
    }

    fn runningCount(): usize {
        let c: usize = 0;
        let i: usize = 0;
        while (i < _.node_count) {
            if (_.nodes[i].status == FilterScriptStatus.Running) {
                c += 1;
            }
            i += 1;
        }
        return c;
    }

    fn pendingCount(): usize {
        let c: usize = 0;
        let i: usize = 0;
        while (i < _.node_count) {
            if (_.nodes[i].status == FilterScriptStatus.Pending) {
                c += 1;
            }
            i += 1;
        }
        return c;
    }

    fn completedCount(): usize {
        let c: usize = 0;
        let i: usize = 0;
        while (i < _.node_count) {
            const st = _.nodes[i].status;
            if (st != FilterScriptStatus.Pending && st != FilterScriptStatus.Running) {
                c += 1;
            }
            i += 1;
        }
        return c;
    }

    /// 是否所有调度任务均已结束
    fn isDone(): bool {
        if (_.runningCount() > 0) {
            return false;
        }
        if (_.aborted) {
            return true;
        }
        return _.pendingCount() == 0;
    }

    /// 聚合最终退出码（对齐 Bun `State.finalize`）
    fn finalizeExitCode(): u8 {
        let i: usize = 0;
        while (i < _.node_count) {
            const node = _.nodes[i];
            switch (node.status) {
                FilterScriptStatus.Exited => {
                    if (node.exit_code != 0) {
                        return node.exit_code;
                    }
                },
                FilterScriptStatus.Signaled => {
                    if (node.signal_code == 0) {
                        return 1;
                    }
                    const code: u16 = (128 as u16) + (node.signal_code as u16);
                    return if (code > 255) 255 else (code as u8);
                },
                FilterScriptStatus.Errored => {
                    return 1;
                },
                FilterScriptStatus.Pending => {},
                FilterScriptStatus.Running => {},
                FilterScriptStatus.Aborted => {},
            }
            i += 1;
        }
        return 0;
    }
}
