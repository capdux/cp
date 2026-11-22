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
5. **并行测试 IPC 二进制帧编解码（`lib/test_runner/ipc_frame.cp`，吸收自 `/data/dev/bun/src/cli/test/parallel/Frame.cp`）**：
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
6. **并行工作窃取区间调度器（`lib/test_runner/work_stealing.cp`，吸收自 `/data/dev/bun/src/cli/test/parallel/FileRange.cp`）**：
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

### 上游 Bun Test Reporters 与 Code Coverage 生产最佳实践（吸收自 `/data/dev/bun/docs/test/{reporters,code-coverage}.mdx`）

1. **多形态 CI 报告协同**：
   - 终端交互与大套件推荐精简进度输出（如 dots 模式，仅打印 `.` / `F` 减少日志刷屏）；无颜色终端环境自动降级为纯 ASCII（`(pass)` / `(fail)`）。
   - CI 流水线（GitLab CI / Jenkins / GitHub Actions）可同时输出控制台摘要与 JUnit XML 文件，并在并发执行时通过 `mergeJUnitFragments` 汇总多 Worker 产物。
2. **代码覆盖率阈值门禁与持久化报告**：
   - 推荐在 CI 中配置分层覆盖率阈值（`functions` 与 `lines` 可分别设定，如 `0.9`），任一文件低于阈值时触发门禁失败；
   - 通过 `CoverageAggregator.renderMergedLcov` 输出标准 `lcov.info`，可直接对接 Codecov、Coveralls、VS Code Coverage Gutters 等生态工具。
