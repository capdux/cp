# `cap:junit` — JUnit XML CI 测试报告零分配解析与汇总库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:obs`，导入请使用 `import { ... } from "cap:obs"`。

`cap:junit` 提供纯 CP、零堆分配的 JUnit XML 测试报告解析与汇总能力，兼容 Maven Surefire、pytest、Jest、Go (`gotestsum`) 与 Cargo (`cargo-nextest`) 等 CI 工具生成的 `<testsuites>` / `<testsuite>` / `<testcase>` / `<failure>` / `<error>` / `<skipped>` 报告。

## 导入方式

```cp
import {
    JunitOutcome,
    JunitSuite,
    JunitCase,
    JunitSummary,
    JunitSuiteIter,
    JunitCaseIter,
    isJunitXml,
    summarizeJunit,
} from "cap:obs";
```

## 枚举与数据结构

### `JunitOutcome: u8`
- `passed`, `failed`, `errored`, `skipped`

### `JunitSuite`
- `name: []const u8`, `tests`, `failures`, `errors`, `skipped: u32`, `time_sec: f64`, `timestamp: []const u8`, `raw_xml: []const u8`
- 方法 `cases(): JunitCaseIter` — 零分配迭代该套件内全部 `<testcase>`

### `JunitCase`
- `name`, `classname: []const u8`, `time_sec: f64`, `outcome: JunitOutcome`
- `message`, `fail_type: []const u8`（来自 `<failure>`/`<error>`/`<skipped>` 属性）
- `detail_text: []const u8`（失败/错误堆栈正文，自动剥离 `<![CDATA[...]]>`）

### `JunitSummary`
- `suite_count`, `total`, `passed`, `failed`, `errored`, `skipped: u32`, `total_time_sec: f64`
- 方法 `allPassed(): bool` — 无 `failed` 且无 `errored` 返回 `true`

## 核心函数与迭代器

- `isJunitXml(data: []const u8): bool` — 跳过可选 UTF-8 BOM、XML 声明与注释，检测 `<testsuites` 或 `<testsuite`
- `JunitSuiteIter.init(data)` / `.next()` — 迭代全部 `<testsuite>`（兼容外层 `<testsuites>`、单顶层与自闭合空套件）
- `JunitCaseIter.init(suite_body)` / `.next()` — 迭代 `<testcase>`（支持自闭合通过用例与完整子节点用例）
- `summarizeJunit(data): JunitSummary` — 单遍汇总统计

## 快速示例

```cp
import { JunitSuiteIter, summarizeJunit } from "cap:obs";

fn checkCiReport(xml_bytes: []const u8) {
    const summary = summarizeJunit(xml_bytes);
    if (!summary.allPassed()) {
        let sit = JunitSuiteIter.init(xml_bytes);
        while (suite := sit.next()) {
            let cit = suite.cases();
            while (tc := cit.next()) {
                * = tc.detail_text;
            }
        }
    }
}
```
