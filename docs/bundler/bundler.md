# CP 标准库：模块打包配置、Chunk 拓扑排序与管线状态手册 (`cap:bundler`)

## 一、 架构总览与设计准则

`cap:bundler` 是 CP 语言官方内置的模块打包器（Bundler）核心模型标准库，提炼自 Bun 原生 Bundler V2 与 Chunk 拓扑调度架构（`/data/dev/bun/src/bundler/bundle_v2.cp` 与 `Chunk.cp`）。模块采用零堆分配栈内联存储与确定性拓扑排序设计，提供打包目标配置（`BundleConfig`）、多格式产出枚举（`BundleTarget` / `OutputFormat`）、基于 `(distance, tie_breaker)` 的稳定 Chunk 排序器（`ChunkOrder`）以及打包会话状态机（`BundleV2`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                        CP Bundler V2 打包管线模型                        │
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 配置与目标平台层       │  确定性 Chunk 拓扑排序   │  BundleV2 产出与统计    │
│ BundleConfig         │  ChunkOrder             │  BundleV2 (32 栈槽)     │
│   target: Browser/   │    distance (入口距离)   │    addSourceBytes()    │
│     Node/Bun/CpNative│    tie_breaker (DFS 序) │    emitChunk()         │
│   format: Esm/Cjs/   │    compare / sort()     │    findChunk(name)     │
│     Iife             │  ChunkEntry / ChunkKind │    compressionRatio()  │
│   minify / splitting │    Entry / Shared / Asset│   markTopLevelAwait()  │
└──────────────────────┴─────────────────────────┴────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/bundler/bundle.cp` | `// Source: /data/dev/bun/src/bundler/` | `BundleTarget`、`OutputFormat`、`ChunkKind`、`ChunkOrder`（确定性排序）、`ChunkEntry`、`BundleConfig` 与 `BundleV2` 会话状态机 |
| `lib/bundler/index.cp` | `// Source: /data/dev/bun/src/bundler/` | `cap:bundler` 对外统一导出入口 |

---

## 二、 核心 API 与设计语义

### 1. 目标与格式枚举

- **`BundleTarget`**：`Browser (0)`、`Node (1)`、`Bun (2)`、`CpNative (3)`。
- **`OutputFormat`**：`Esm (0)`、`Cjs (1)`、`Iife (2)`。
- **`ChunkKind`**：`EntryPoint (0)`、`SharedChunk (1)`、`Asset (2)`。

### 2. 确定性 Chunk 拓扑排序 (`ChunkOrder`)

沿用 Bun `src/bundler/Chunk.cp` 的稳定排序策略：
- **主关键字 `distance`**：距离入口模块最近的依赖优先输出；
- **次关键字 `tie_breaker`**：当两个模块距入口等距时，以 DFS 遍历产生的稳定 `source_index` 打破平局，确保跨机器、多次构建产物字节级 100% 确定性（Deterministic Build）。
- `ChunkOrder.sort(items)`：原地稳定插入排序，无额外堆分配。

### 3. 配置与打包会话 (`BundleConfig` & `BundleV2`)

- **`BundleConfig`**：提供 `defaultConfig()`（面向 `CpNative` / `Esm`）与 `productionBrowser(out_dir)`（开启 `minify`、`sourcemap`、`splitting`）两种工厂预设。
- **`BundleV2`**：
  - `init(config)`：初始化打包会话；
  - `addSourceBytes(bytes)`：累计输入源码字节数；
  - `markTopLevelAwait()`：标记构建图中存在顶层 `await` 模块；
  - `emitChunk(entry)`：登记产出 Chunk（最多 32 槽栈内联存储，自动更新 `entry_count` 与 `total_output_bytes`）；
  - `chunkCount()` / `findChunk(name)`：只读查询产出数量与按文件名检索 Chunk 元数据；
  - `compressionRatioPermille()`：以千分比计算产出体积与源码体积之比（如压缩至 40% 返回 `400`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    BundleTarget,
    OutputFormat,
    ChunkKind,
    ChunkOrder,
    ChunkEntry,
    BundleConfig,
    BundleV2,
} from "cap:bundler";

pub fn demoBundler() {
    // 1. 构建面向浏览器生产环境的配置
    const cfg = BundleConfig.productionBrowser("dist/web");
    let bundler = BundleV2.init(cfg);

    // 2. 对模块图进行确定性 Chunk 拓扑排序
    let orders: [3]ChunkOrder = [
        ChunkOrder { source_index: 12, distance: 2, tie_breaker: 5 },
        ChunkOrder { source_index: 4, distance: 1, tie_breaker: 2 },
        ChunkOrder { source_index: 7, distance: 1, tie_breaker: 1 },
    ];
    ChunkOrder.sort(orders[0..3]);
    // 排序结果: source_index 7 (dist=1, tb=1) -> 4 (dist=1, tb=2) -> 12 (dist=2, tb=5)

    // 3. 记录输入源码与产出 Chunk
    bundler.addSourceBytes(20000);
    * = bundler.emitChunk(ChunkEntry {
        name: "index.js",
        kind: ChunkKind.EntryPoint,
        source_index: 7,
        size_bytes: 6000,
        is_executable: true,
    });
    * = bundler.emitChunk(ChunkEntry {
        name: "vendor.js",
        kind: ChunkKind.SharedChunk,
        source_index: 4,
        size_bytes: 2000,
        is_executable: false,
    });

    // compressionRatioPermille() == 400 (8000 / 20000 = 40.0%)
    const ratio = bundler.compressionRatioPermille();
    * = ratio;
}
```

---

## 四、 与 Bun / esbuild API 对照表

| 功能特性 | Bun (`Bun.build` / `src/bundler/`) | esbuild (`esbuild.build`) | CP 标准库 (`cap:bundler`) |
| :--- | :--- | :--- | :--- |
| **目标平台枚举** | `target: "browser" \| "bun" \| "node"` | `platform: "browser" \| "node"` | `BundleTarget` (`Browser` / `Node` / `Bun` / `CpNative`) |
| **输出模块格式** | `format: "esm" \| "cjs" \| "iife"` | `format: "esm" \| "cjs" \| "iife"` | `OutputFormat` (`Esm` / `Cjs` / `Iife`) |
| **稳定 Chunk 排序** | `ChunkOrder.sort` (`src/bundler/Chunk.cp`) | `chunkOrder` | `ChunkOrder.sort` / `ChunkOrder.compare` |
| **打包会话与产物清单** | `BundleV2` (`src/bundler/bundle_v2.cp`) | `BuildResult.metafile` | `BundleV2` (`emitChunk` / `findChunk` / `compressionRatioPermille`) |
