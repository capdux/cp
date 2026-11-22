# CP 标准库：纯 CP 生产测试框架基础设施 (`cap:test_runner`)

> 参考来源：`/data/dev/bun/docs/test/{configuration,runtime-behavior}.mdx`, `/data/dev/bun/src/test_runner/` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:test_runner` 是零 JSC 耦合的纯 CP 生产级测试套件编排、快照管理、值格式化与 CI 报告引擎，完整直迁自 Bun `src/test_runner/` 核心无 JS 引擎逻辑（`Order.cp`、`Collection.cp`、`Execution.cp`、`snapshot.cp`、`pretty_format.cp`、`bun_test.cp`）。

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│                     cap:test_runner 测试基础设施管线                          │
├────────────────────┬─────────────────────────┬───────────────────────────────┤
│ 1. 收集与排序层      │  2. 执行与生命周期层     │  3. 快照与 CI 报告层           │
│   TestRunner       │    describe 树形嵌套     │    SnapshotStore (.snap v1)   │
│   TestFilter /Only │    beforeAll/beforeEach │      cap:patch DiffFormatter  │
│   TestOrder (PRNG) │    afterEach/afterAll   │    PrettyFormat               │
│     Xoshiro256++   │    TestRunSummary 统计   │    TestReporter (TAP/JUnit/JSON)│
└────────────────────┴─────────────────────────┴───────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/test_runner/order.cp` | `// Source: /data/dev/bun/src/test_runner/Order.cp` | `TestOrderMode`、`Prng64`（SplitMix64 + Xoshiro256++）、`TestOrder.shuffleIndices` 确定性洗牌 |
| `lib/test_runner/suite.cp` | `// Source: /data/dev/bun/src/test_runner/Collection.cp & .../Execution.cp` | `TestStatus`、`TestFilter`、`HookKind`、`TestCase`、`TestGroup`、`TestContext`、`TestRunner` |
| `lib/test_runner/snapshot.cp` | `// Source: /data/dev/bun/src/test_runner/snapshot.cp` | `SnapshotStore` `.snap` v1 读写、`Check`/`Update`/`CreateNew` 与 `cap:patch` 彩色 Diff |
| `lib/test_runner/reporter.cp` | `// Source: /data/dev/bun/src/test_runner/pretty_format.cp & .../bun_test.cp` | `PrettyFormat` 美化输出与 `TestReporter`（TAP v13 / JUnit XML / JSON） |
| `lib/test_runner/index.cp` | `// Source: /data/dev/bun/src/test_runner/` | `cap:test_runner` 对外统一导出入口 |

---

## 二、 核心 API 与设计语义

### 1. 确定性测试排序与种子伪随机 (`order.cp`)

- `TestOrderMode`：`Declaration`（声明序）、`Random`（确定性随机序）、`Reverse`（逆序）。
- `Prng64`：基于 SplitMix64 种子扩展与 Xoshiro256++ 状态步进的 64 位可复现伪随机数生成器；`deriveSeed(base_seed, salt)` 为不同套件派生独立且可复现的子种子。
- `TestOrder.shuffleIndices(indices, seed)`：使用无偏拒绝采样的确定性 Fisher-Yates 洗牌算法，保证相同 `seed` 在任意平台得到完全一致的测试序；`formatSeedBanner` 输出可复现种子提示。

### 2. 测试套件树、钩子与过滤引擎 (`suite.cp`)

- `TestStatus`：`Pending`、`Passed`、`Failed`、`Skipped`、`Todo`、`TimedOut`。
- `TestFilter`：支持 `name_pattern` 子串过滤、`tag_pattern` 标签过滤以及 `.only` 独占模式。
- `HookKind`：`BeforeAll`、`BeforeEach`、`AfterEach`、`AfterAll`。
- `TestRunner`：
  - 支持 `describe(name, builder)` / `describeSkip` / `describeOnly` 与 `beginGroup` / `endGroup` 构建任意深度的树形嵌套套件。
  - 严格执行生命周期钩子嵌套顺序：`beforeAll`（进入分组时）-> `beforeEach`（由外向内）-> 测试体 -> `afterEach`（由内向外栈式回退）-> `afterAll`（退出分组时）。
  - 传入 `TestContext`（支持 `expectTrue` / `expectEq` / `expectStrEq` / `fail` / `failDiff` / `user_data`），精确捕获失败信息、断言次数与 `monotonicNs()` 纳秒级耗时，汇总输出 `TestRunSummary`。

### 3. `.snap` 快照引擎 (`snapshot.cp`)

- `SnapshotStore`：
  - 使用 Bun 兼容格式（首行 `// Cap Snapshot v1, ...` + ``exports[`name`] = `...`;``），对反斜杠 `\`、反引号 `` ` `` 与模板插值 `${` 执行转义与无损反转义。
  - `load(path)` / `save(path)`：支持真实磁盘 `.snap` 文件读取、创建与原子写回。
  - `matchSnapshot(name, actual, update_mode)`：支持 `SnapshotUpdateMode.Check` / `Update` / `CreateNew`；当快照不匹配时，直接调用 `cap:patch` `DiffFormatter` 生成带行号与 ANSI 字符级高亮的差异报告。

### 4. 格式化器与多格式 CI 报告 (`reporter.cp`)

- `PrettyFormat`：支持字符串、整数、浮点、布尔、切片及键值对象（`PrettyNode` / `PrettyField` 与泛型 `formatValue`）的缩进美化输出。
- `TestReporter`：
  - **TAP v13**（`formatTap`）：输出 `TAP version 13`、`1..N`、`ok`/`not ok`、`# SKIP`、`# TODO` 及 YAML 诊断块。
  - **JUnit XML**（`formatJUnit`）：输出完整 `<testsuites>` / `<testsuite>` / `<testcase>` / `<failure>` / `<skipped />`，并对 `&`、`<`、`>`、`"`、`'` 执行完整 XML 实体转义。
  - **JSON Report**（`formatJson`）：输出包含 `seed`、`order`、`summary` 与 `tests` 明细的标准 JSON 报告。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    TestOrderMode,
    TestRunner,
    TestContext,
    SnapshotStore,
    SnapshotUpdateMode,
    TestReporter,
    ReportFormat,
} from "cap:test_runner";

fn onPass(ctx: *TestContext) {
    * = ctx.expectEq(2 + 2, 4);
}

pub fn demoTestRunner(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    let runner = TestRunner.init(alloc);
    defer runner.deinit();
    runner.setOrderMode(TestOrderMode.Random, 20261120);
    runner.addTest("basic math", onPass)?;

    const summary = runner.run()?;
    const junit = TestReporter.format(alloc, summary, ReportFormat.JUnit, "demo-suite")?;
    * = junit;

    let snaps = SnapshotStore.init(alloc);
    defer snaps.deinit();
    const m = snaps.matchSnapshot("greeting 1", "hello cp", SnapshotUpdateMode.CreateNew)?;
    * = m.matched;
}
```

---

## 上游 Bun Test Runner 行为契约与边界注意事项

1. **确定性随机洗牌与复现（`TestOrderMode.Random` & `Prng64`）**：
   - 参考 `/data/dev/bun/docs/test/runtime-behavior.mdx`：随机乱序执行用于暴露测试用例间的隐式共享状态依赖；通过记录 `TestRunner.setOrderMode(TestOrderMode.Random, seed)` 中的 `seed`（SplitMix64 + Xoshiro256++），CI 失败时可 100% 复现相同的执行顺序。
2. **嵌套生命周期钩子顺序（`HookKind`）**：
   - 嵌套 `TestGroup` 严格遵循 `外层 beforeAll -> 外层 beforeEach -> 内层 beforeEach -> 用例 -> 内层 afterEach -> 外层 afterEach -> 外层 afterAll` 执行，且即使单个用例失败仍保证执行对应的 `afterEach` / `afterAll` 清理钩子。
3. **快照更新策略（`SnapshotUpdateMode`）**：
   - CI 流水线应始终使用 `SnapshotUpdateMode.Check`（严禁静默写入新快照）；本地显式更新时切换至 `Update` 或 `CreateNew`，不匹配时自动联动 `cap:patch` 的 `DiffFormatter` 输出彩色差异。
4. **CI 报告格式（`TestReporter`）**：
   - 原生支持 `TAP` v13、`JUnit` XML 与 `JSON` 三种标准格式，可直接对接 GitHub Actions / GitLab CI 测试结果归档。
5. **并行测试 IPC 二进制帧编解码（`lib/test_runner/ipcFrame.cp`，吸收自 `/data/dev/bun/src/cli/test/parallel/Frame.cp`）**：
   - 线协议格式：`[u32 LE payload_len][u8 kind][payload]`，嵌入式字符串为 `[u32 LE len][bytes]`，强制 64 MiB (`IPC_FRAME_MAX_PAYLOAD`) 单帧载荷上限，防止 `5 + len` 整数溢出与异常大包内存膨胀。
   - 核心符号：`IPC_FRAME_HEADER_SIZE` (`5`)、`IPC_FRAME_MAX_PAYLOAD`、`IPC_TRUNC_MARKER`、`IpcFrameError`、`IpcFrameKind`（`ready` / `file_start` / `test_done` / `file_done` / `repeat_bufs` / `junit_file` / `coverage_file` / `run` / `shutdown`）、`IpcDecodedFrame`、`IpcPayloadReader`、`IpcFrameWriter`、`decodeIpcFrame`。

```typescript
import { IpcFrameKind, IpcFrameWriter, IpcPayloadReader, decodeIpcFrame } from "cap:test_runner";

let buf: [128]u8 = [0; 128];
let w = IpcFrameWriter.init(buf[0..]);
* = w.begin(IpcFrameKind.run)?;
* = w.writeU32(7)?;
* = w.writeStr("test/math.test.cp")?;
const wire = w.finish()?;

const frame = decodeIpcFrame(wire)?;
let r = IpcPayloadReader.init(frame.payload);
const file_idx = r.readU32Checked()?; // 7
const path = r.readStrChecked()?;     // "test/math.test.cp"
```
6. **并行工作窃取区间调度器（`lib/test_runner/workSteal.cp`，吸收自 `/data/dev/bun/src/cli/test/parallel/FileRange.cp`）**：
   - 常量与类型：`WORK_STEAL_MAX_WORKERS` (`64`)、`FileRange`（`init` / `len` / `isEmpty` / `contains` / `popFront` / `popBack` / `stealBackHalf`）、`WorkStealPool`（`partition` / `rangeOf` / `totalRemaining` / `stealFromBusiest` / `nextFor`）。
   - 本地派发从区间头部拉取（cache-hot 局部性），窃取从最忙同伴尾部切分后半段连续区间，兼顾负载均衡与目录局部性。

```typescript
import { WorkStealPool } from "cap:test_runner";

let pool = WorkStealPool.partition(10, 4); // [0..3), [3..6), [6..8), [8..10)
const first = pool.nextFor(0) ?? 0;        // 0 (worker 0 pops locally)
```
7. **并行 Worker JUnit XML 与 LCOV 覆盖率聚合器（`lib/test_runner/aggregate.cp`，吸收自 `/data/dev/bun/src/cli/test/parallel/aggregate.cp`）**：
   - 核心符号：`JUnitTotals`、`JUnitMergeResult`、`CoverageThreshold`、`MergedCoverageFile`、`CoverageAggregator`、`parseJUnitAttrU32`、`appendXmlEscaped`、`mergeJUnitFragments`。
   - **JUnit 片段合并（`mergeJUnitFragments`）**：提取各 Worker 生成的 `<testsuites ...>` 头部 `tests`/`failures`/`skipped` 计数与内层 `<testsuite>`，并为异常中止的测试文件合成 XML 转义后的失败用例，确保外层 `<testsuites>` 总数与内层严格相等（通过 GitLab CI / Jenkins / GitHub Actions Schema 校验）。
   - **LCOV 覆盖率聚合（`CoverageAggregator`）**：支持增量 `ingestLcov`（`SF:` 归并、`DA:line,hits` 饱和累加 `+|`、`FNF:`/`FNH:` 取最大值）、`formatUncoveredRanges`（连续未覆盖行折叠为 `"3,5-8,12"`）、`renderMergedLcov`（文件路径字典序 + 行号升序确定性输出 `lcov.info`）以及 `evaluateThresholds`（按函数/行阈值门禁判定）。

```typescript
import { CoverageAggregator, CoverageThreshold, mergeJUnitFragments } from "cap:test_runner";

pub fn demoAggregate(allocator: std.mem.Allocator): ! {
    let agg = CoverageAggregator.init(allocator);
    defer agg.deinit();
    agg.ingestLcov("SF:src/math.cp\nFNF:2\nFNH:2\nDA:1,3\nDA:2,0\nDA:3,0\nend_of_record\n")?;
    const ok = agg.evaluateThresholds(CoverageThreshold{ functions: 0.9, lines: 0.5 });
    * = ok;
    const merged_lcov = agg.renderMergedLcov(allocator)?;
    defer allocator.free(merged_lcov);
}
```
8. **测试文件发现、过滤与目录剪枝引擎（`lib/test_runner/discovery.cp`，吸收自 `/data/dev/bun/src/cli/test/Scanner.cp`）**：
   - 常量与函数：`TEST_SUFFIX_DOT_TEST` (`.test`)、`TEST_SUFFIX_UNDER_TEST` (`_test`)、`TEST_SUFFIX_DOT_SPEC` (`.spec`)、`TEST_SUFFIX_UNDER_SPEC` (`_spec`)、`TEST_DEFAULT_IGNORE_DIR` (`node_modules`)、`TEST_DISCOVERY_MAX_PATH` (`4096`)、`isSupportedTestExt`、`hasBunTestSuffix`、`hasTestNameSuffix`、`isExplicitPathArg`、`shouldSkipDirectory`、`matchesDiscoveryGlob`；
   - 核心类型：`TestDiscoveryFilter`（`couldBeTestFile` / `doesAbsolutePathMatchFilter` / `doesPathMatchFilter` / `matchesPathIgnorePattern` / `shouldPruneDirectory` / `shouldRunFile`）。

```typescript
import { TestDiscoveryFilter, isExplicitPathArg } from "cap:test_runner";

const excludes: [1][]const u8 = ["fixtures"];
const filters: [1][]const u8 = ["utils"];
const ignores: [1][]const u8 = ["vendor/**"];
const disc = TestDiscoveryFilter{
    exclusion_names: excludes[0..],
    filter_names: filters[0..],
    path_ignore_patterns: ignores[0..],
    include_cp_prefix: true,
};
const run_it = disc.shouldRunFile("/repo/src/utils/str.test.cp", "src/utils/str.test.cp", true); // true
const prune_vendor = disc.shouldPruneDirectory("vendor", "vendor"); // true
const explicit = isExplicitPathArg("./test/specific.test.cp"); // true
* = run_it;
* = prune_vendor;
* = explicit;
```
9. **并行测试协调器状态机（`lib/test_runner/coordinator.cp`，吸收自 `/data/dev/bun/src/cli/test/parallel/Coordinator.cp`）**：
   - 常量与类型：`COORDINATOR_MAX_WORKERS` (`64`)、`COORDINATOR_MAX_CRASHED_FILES` (`64`)、`COORDINATOR_DEFAULT_SCALE_UP_MS` (`150`)、`CoordinatorError`、`WorkerSlotState`、`WorkerSlot`、`CoordinatorConfig`、`DispatchDecisionKind`、`DispatchDecision`、`ParallelCoordinator`（`init` / `isDone` / `hasUndispatchedFiles` / `spawnNextWorker` / `shouldScaleUp` / `applyScaleUp` / `assignWork` / `bailOut` / `onTestResult` / `onFileDone` / `onWorkerExited` / `shouldEmitHeader` / `crashedFileAt`）。

```typescript
import { ParallelCoordinator, CoordinatorConfig } from "cap:test_runner";

let coord = ParallelCoordinator.init(CoordinatorConfig{
    file_count: 8,
    parallel_limit: 4,
    scale_up_after_ms: 150,
    bail_threshold: 1,
});
const w0 = coord.spawnNextWorker() ?? 0;
const d0 = coord.assignWork(w0, 1000)?;
// 当 w0 持续忙碌超过 150ms 后，applyScaleUp(1200) 自动拉起剩余 3 个 Worker
const added = coord.applyScaleUp(1200); // 3
* = d0;
* = added;
```
10. **并行测试双向背压 IPC 通道（`lib/test_runner/ipcChan.cp`，吸收自 `/data/dev/bun/src/cli/test/parallel/Channel.cp`）**：
   - 常量与类型：`IPC_CHANNEL_BUF_CAP` (`8192`)、`IpcChannelError`、`IpcChannelCloseReason`、`IpcChannelPollStatus`、`IpcChannelPollResult`、`IpcChannel`（`init` / `isAttached` / `isDone` / `isProtocolError` / `hasPendingWrites` / `queueSend` / `peekTxChunk` / `advanceTx` / `submitInflight` / `completeInflight` / `ingest` / `pollFrame` / `onPeerEof` / `close`）。
   - 同时支持 POSIX 部分写入背压队列与 Windows/异步 `tx_buf → inflight_buf` 双缓冲切换；入站支持任意分片重组、粘包连续解帧、未知 `IpcFrameKind` 前向跳过，以及 `> 64 MiB` 畸形帧触发 `ProtocolError`（保持 `isAttached() == true` 以与对端 `PeerEof` 精准区分）。

```cp
import {
    IpcChannel,
    IpcChannelPollStatus,
    IpcFrameWriter,
    IpcFrameKind,
} from "cap:test_runner";

let wire: [256]u8 = [0; 256];
let w = IpcFrameWriter.init(wire[0..]);
w.begin(IpcFrameKind.ready)?;
w.writeU32(42)?;
const frame = w.finish()?;

let ch = IpcChannel.init(true);
* = ch.ingest(frame[0..3])?;
* = ch.ingest(frame[3..])?;
let payload: [64]u8 = [0; 64];
const res = ch.pollFrame(payload[0..])?;
// res.status == IpcChannelPollStatus.Frame, res.kind == IpcFrameKind.ready, res.payload_len == 4
* = res;
```

### 上游 Bun Test Discovery、Reporters 与 Code Coverage 生产最佳实践（吸收自 `/data/dev/bun/docs/test/{discovery,reporters,code-coverage,runtime-behavior}.mdx` 与 `/data/dev/bun/docs/guides/test/{bail,timeout}.mdx`）

1. **测试文件发现与过滤契约（参考 `/data/dev/bun/docs/test/discovery.mdx`）**：
   - **默认匹配模式**：递归扫描 `*.test.{js|jsx|ts|tsx|cp}`、`*_test.*`、`*.spec.*`、`*_spec.*`（CP 工程默认兼容 `test_*.cp` 前缀约定），自动忽略 `node_modules` 与隐藏目录（`.` 开头）。
   - **位置参数过滤 vs 显式路径**：位置过滤参数按**子串匹配**工作（非 glob），如 `utils` 可命中 `src/utils/string.test.ts` 与 `lib/utils/array_test.js`；若需运行精确文件，路径必须以 `./`、`../` 或 `/` 开头（`isExplicitPathArg`）。
   - **目录剪枝防误杀原则**：`path_ignore_patterns` 仅对包含 `**` 的模式（如 `vendor/**`）追加末尾 `/` 尝试提前剪枝整棵子树；单星号模式（如 `vendor/*`）不跨目录边界，因此绝不误剪枝子目录。
2. **多形态 CI 报告协同**：
   - 终端交互与大套件推荐精简进度输出（如 dots 模式，仅打印 `.` / `F` 减少日志刷屏）；无颜色终端环境自动降级为纯 ASCII（`(pass)` / `(fail)`）。
   - CI 流水线（GitLab CI / Jenkins / GitHub Actions）可同时输出控制台摘要与 JUnit XML 文件，并在并发执行时通过 `mergeJUnitFragments` 汇总多 Worker 产物。
3. **代码覆盖率阈值门禁与持久化报告**：
   - 推荐在 CI 中配置分层覆盖率阈值（`functions` 与 `lines` 可分别设定，如 `0.9`），任一文件低于阈值时触发门禁失败；
   - 通过 `CoverageAggregator.renderMergedLcov` 输出标准 `lcov.info`，可直接对接 Codecov、Coveralls、VS Code Coverage Gutters 等生态工具。
4. **并行调度、快速熔断（`--bail`）与超时控制（吸收自 `/data/dev/bun/docs/guides/test/{bail,timeout}.mdx`）**：
   - **懒扩容与工作窃取**：协调器初始仅拉起 1 个 Worker，避免极速小套件产生进程启动开销；仅当所有存活 Worker 均持续忙碌超过 `scale_up_after_ms`（默认 `150ms`）且仍有未派发文件时，才一次性扩容至 `parallel_limit`，随后通过 `WorkStealPool` 从剩余量最大槽位窃取后半段以保持目录局部性。
   - **CI 快速熔断（`--bail` / `--bail=N`）**：设置 `bail_threshold = 1`（单次失败即中止）或 `bail_threshold = N`（累计 N 次失败后中止），触发后立即停止派发新文件并优雅关闭空闲 Worker，显著缩短 CI 失败反馈周期；默认单测超时基线为 `5000ms`（可通过 `--timeout` 调整）。
5. **运行时环境一致性、超时边界与未处理异常门禁（吸收自 `/data/dev/bun/docs/test/runtime-behavior.mdx`）**：
   - **确定性环境变量**：测试运行器默认注入 `NODE_ENV=test`（除非外部环境或 `.env` 显式覆盖）并固定时区为 `TZ=Etc/UTC`，避免跨机器日期/时区差异引发偶发失败；
   - **分级超时配置**：支持全局 `--timeout`、单测第三参数覆盖，以及传入 `0`/`Infinity` 禁用超时；
   - **测试间未处理异常计入退出码**：即便所有 `test(...)` 用例均通过，测试间隙发生的未捕获异常或未处理异步拒绝仍会导致最终退出码非零（等于未处理错误数），防止异步泄漏被静默掩盖。
6. **并行 Worker 进程槽位、双门槛回收与 `concurrentTestGlob` 选择性并发（`lib/test_runner/workerProc.cp`，吸收自 `/data/dev/bun/src/cli/test/parallel/Worker.cp` & `/data/dev/bun/docs/guides/test/concurrent-test-glob.mdx`）**：
   - **导出符号**：`WORKER_PROC_CAPTURE_CAP` (`2048`)、`WorkerProcError`、`WorkerPipeRole`、`WorkerExitKind`、`WorkerExitStatus`、`WorkerLifecyclePhase`、`WorkerIpcAction`、`WorkerSpawnPolicy`、`WorkerProcessSlot`（`init`/`beginStart`/`commitStart`/`abortStart`/`dispatchFile`/`completeInflight`/`requestShutdown`/`onPipeChunk`/`onPipeDone`/`capturedSlice`/`takeCaptured`/`onProcessExit`/`onChannelDone`/`canReap`/`reap`）、`matchesConcurrentTestGlob`；
   - **双门槛 Reap 屏障与原子捕获**：回收 Worker 必须同时满足 `exit_status != null && ipc_done == true`，确保内核管道缓冲区中的尾部 `test_done`/JUnit/Coverage 帧先被完整解码；每文件 stdout/stderr 合并暂存于 `captured` 并在 `file_done` 时原子刷出，彻底消除并发测试日志交错；
   - **启动回滚与畸形帧防护**：`abortStart()` 原子复位槽位而不触发 `onProcessExit`，避免 `EMFILE` 场景下递归拉起进程风暴；IPC 结束时若仍处于 `still_attached` 则返回 `WorkerIpcAction.KillAndReap`；
   - **选择性并发最佳实践**：默认顺序执行以保护共享状态测试，可通过 `concurrentTestGlob = "**/concurrent-*.test.ts"`（`matchesConcurrentTestGlob`）仅对独立无状态集成测试开启并发。

```cp
import {
    WorkerProcessSlot,
    WorkerExitStatus,
    WorkerIpcAction,
    matchesConcurrentTestGlob,
} from "cap:test_runner";

let w = WorkerProcessSlot.init(0);
* = w.beginStart()?;
* = w.commitStart(true)?;
* = w.dispatchFile(3, 1000)?;
* = w.onPipeChunk(.Stdout, "ok\n");
* = w.completeInflight();
* = w.requestShutdown()?;
const ready1 = w.onProcessExit(WorkerExitStatus.exited(0)); // false (IPC still open)
const act = w.onChannelDone(false); // WorkerIpcAction.ReadyToReap
const reaped = w.reap();            // true
const is_par = matchesConcurrentTestGlob("**/concurrent-*.test.ts", "tests/integration/concurrent-api.test.ts");
* = ready1; * = act; * = reaped; * = is_par;
```
