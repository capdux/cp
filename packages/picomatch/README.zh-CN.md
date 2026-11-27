[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `picomatch`

面向 CP 语言的零分配 Glob 路径匹配器与静态目录前缀扫描器——完整对标 **npm `picomatch` v4**，支持 `**` 递归通配、字符类（`[a-z]`、`[!0-9]`）、花括号展开（`{a,b}`）、点文件保护与前导取反（`!`）。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `npm:picomatch` ([micromatch/picomatch](https://github.com/micromatch/picomatch), v4 对齐) |
| **CP 包名与版本** | `picomatch@1.0.0` (`MIT`) |
| **依赖** | `normalize-path@1.0.0`、`is-extglob@1.0.0` |

保持原包名导入，零改名成本直接迁移：

```ts
// TypeScript (npm picomatch)
import picomatch from "picomatch";
const isMatch = picomatch("src/**/codegen/*.ts");
isMatch("src/a/b/codegen/emit.ts"); // true
```

```cp
// CP — 保持原包名，零堆分配直接匹配
import { isMatch } from "picomatch";

const ok = isMatch("src/a/b/codegen/emit.cp", "src/**/codegen/*.cp"); // true
```

## ReleaseFast 实测性能亮点

在 Intel Xeon Platinum 8175M 平台下使用 `cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm` 实测（详见 [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)）：

| 操作 | 测试载荷 | 延迟 | 吞吐量 | 核心优化架构 |
| :--- | :--- | --: | --: | :--- |
| `isMatch` | 47 B 路径匹配 `src/**/codegen/*.cp` | **343.42 ns/op** | **2,911,885 ops/s** | 16-lane SIMD 分段扫描、零堆分配 |

- **零堆分配**：无需正则编译或 AST 堆分配，全流程在栈上完成带深度保护的分段匹配。
- **16-lane SIMD 分段扫描**：依托 `cap:ascii` 快速定位路径分隔符与字面段。

## 实战示例

### 1. 基础 Glob 匹配（Globstar、花括号、字符类与取反）

```cp
import { isMatch } from "picomatch";

fn demoBasic() {
    const a = isMatch("src/core/codegen/emit.cp", "src/**/codegen/*.cp"); // true
    const b = isMatch("app.tsx", "*.{ts,tsx,cp}");                         // true
    const c = isMatch("file2.log", "file[0-9].log");                       // true
    const d = isMatch("src/main.cp", "!*.md");                             // true
    const hidden = isMatch(".env", "*");                                   // false（默认保护点文件）
    _ = a; _ = b; _ = c; _ = d; _ = hidden;
}
```

### 2. 自定义 `PicoOptions` 与多模式匹配（`isMatchAny`）

```cp
import { PicoOptions, isMatchWithOptions, isMatchAny } from "picomatch";

fn demoOptions() {
    let opts = PicoOptions.defaults();
    opts.dot = true;     // 允许匹配 .env / .github 等隐藏点文件
    opts.nocase = true;  // 忽略 ASCII 大小写

    const matched_dot = isMatchWithOptions(".github/workflows/ci.YML", "**/*.yml", opts); // true

    const patterns = [_]string{ "src/**/*.cp", "lib/**/*.cp", "cap.json" };
    const any_hit = isMatchAny("lib/net/socket.cp", patterns[0..], PicoOptions.defaults()); // true
    _ = matched_dot; _ = any_hit;
}
```

### 3. 进阶边界场景：静态前缀提取（`scanBase`）与 `basename`/`noglobstar` 模式

```cp
import { PicoOptions, isMatchWithOptions, scanBase } from "picomatch";

fn demoEdgeCases() {
    // 提取首个通配符之前的静态目录前缀，大幅缩小文件系统遍历范围
    let base_buf: [128]u8 = undefined;
    const n = scanBase("packages/net/src/**/*.cp", base_buf[0..]);
    // base_buf[0..n] == "packages/net/src"

    // 仅匹配路径末尾文件名
    let opts = PicoOptions.defaults();
    opts.basename = true;
    const hit = isMatchWithOptions("a/b/c/README.md", "*.md", opts); // true

    // 禁用 globstar（** 按单层 * 处理）
    let no_gs = PicoOptions.defaults();
    no_gs.noglobstar = true;
    const flat = isMatchWithOptions("a/b/c.cp", "**/*.cp", no_gs); // false
    _ = n; _ = hit; _ = flat;
}
```

## 核心 API 速览

- `PicoOptions`（`dot`、`nocase`、`basename`、`noglobstar`、`PicoOptions.defaults()`）
- `isMatch(path, pattern)`
- `isMatchWithOptions(path, pattern, opts)`
- `isMatchAny(path, patterns, opts)`
- `scanBase(pattern, out)`

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
