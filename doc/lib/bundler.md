# CP 标准库：模块打包、交叉目标、单文件图与依赖图引擎手册 (`cap:bundler`)

> 参考来源：`/data/dev/bun/docs/bundler/index.mdx`、`/data/dev/bun/docs/bundler/executables.mdx`、`/data/dev/bun/src/bundler/` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:bundler` 是 CP 语言官方内置的模块打包器（Bundler）核心模型标准库，提炼自 Bun 原生 Bundler V2、Chunk 拓扑调度、`CompileTarget` 交叉编译目标解析与 `StandaloneModuleGraph` 单文件可执行虚拟文件系统（`/data/dev/bun/src/bundler/`、`/data/dev/bun/src/options_types/CompileTarget.zig`、`/data/dev/bun/src/standalone_graph/StandaloneModuleGraph.zig`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                        CP Bundler V2 打包与单文件架构                    │
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 1. 打包与拓扑排序层    │  2. 交叉编译目标三元组层  │  3. 单文件可执行 VFS 层  │
│   BundleConfig       │    TargetOs / Arch / Abi│   StandaloneModuleGraph│
│   BundleV2           │    CompileTarget.parse  │     Header (CPSG0001)  │
│   ChunkOrder.sort    │      linux-x64-musl     │     Entry Table        │
│   ModuleGraph        │      bun-darwin-arm64   │     Payload Regions    │
│     topoSort/hasCycle│    exeExt/sharedLibExt  │     Footer Trailer     │
│     markReachable    │    isUnix / format      │   零拷贝 deserialize    │
└──────────────────────┴─────────────────────────┴────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/bundler/bundle.cp` | `// Source: /data/dev/bun/src/bundler/` | `BundleTarget`、`OutputFormat`、`ChunkKind`、`ChunkOrder`、`BundleConfig`、`BundleV2` 与 `ImportKind` / `ImportRecord` / `ModuleGraph`（拓扑排序、循环检测、树摇可达性） |
| `lib/bundler/target.cp` | `// Source: /data/dev/bun/src/options_types/CompileTarget.zig` | `TargetOs`、`TargetArch`、`TargetAbi`、`CompileTarget` 目标三元组解析、格式化与扩展名判定 |
| `lib/bundler/standalone.cp` | `// Source: /data/dev/bun/src/standalone_graph/StandaloneModuleGraph.zig` | `StandaloneModuleEntry`、`StandaloneModuleGraph` 二进制序列化与零拷贝越界防护反序列化 |
| `lib/bundler/metafile.cp` | `// Source: /data/dev/bun/src/bundler/linker_context/MetafileBuilder.cp` | `Metafile`、`MetafileInput`、`MetafileOutput`、`MetafileImport` JSON 生成与彩色树状体积分析表 |
| `lib/bundler/splitting.cp` | `// Source: /data/dev/bun/src/bundler/LinkerGraph.cp & Chunk.cp` | `CodeSplitter` 多入口 `u64` 位掩码代码分割、共享 Chunk 抽取、内容哈希与 `AssetManifest` |
| `lib/bundler/defines.cp` | `// Source: /data/dev/bun/src/bundler/defines.cp` | 零堆分配编译期 `--define` / `--drop` 常量替换表 `DefineTable`、`DefineValue`、`DefineFlags`、`CallUnwrapKind` 与点分属性链前缀剪枝 |
| `lib/bundler/index.cp` | `// Source: /data/dev/bun/src/bundler/` | `cap:bundler` 对外统一导出入口 |

---

## 二、 核心 API 与设计语义

### 1. `BundleV2`、`ChunkOrder` 与 `ModuleGraph` (`lib/bundler/bundle.cp`)

- **`BundleConfig` / `BundleV2` / `ChunkOrder`**：100% 保持原 API 兼容，支持 `defaultConfig()`、`productionBrowser(out_dir)`、`ChunkOrder.sort()`、`emitChunk()`、`findChunk()`、`compressionRatioPermille()`。
- **`ImportKind` & `ImportRecord`**：描述 AST 导入记录（`EntryPoint` / `ImportStmt` / `RequireCall` / `DynamicImport` / `CssAtImport` / `AssetUrl` / `Internal`）。
- **`ModuleGraph`**：
  - `addModule(path, is_entry, size_bytes)`：注册模块节点并去重返回稳定 `u32` ID；
  - `addImport(from_id, record)` / `addDependency(from_id, to_id, specifier, kind)`：注册有向依赖边；
  - `hasCycle()`：三色深度优先搜索循环依赖检测；
  - `topoSort(allocator)`：确定性依赖优先（Dependencies-First）拓扑排序，若存在环则返回 `error.CyclicDependency`；
  - `markReachable(entry_id)` / `markAllEntryPointsReachable()` / `isReachable(id)`：面向 Tree-Shaking 的死代码可达性标记。

### 2. 交叉编译目标三元组 (`lib/bundler/target.cp`)

- **`TargetOs`**：`Linux`、`Darwin`、`Windows`、`FreeBSD`、`Wasi`。
- **`TargetArch`**：`X64`、`Arm64`、`Riscv64`、`Wasm32`。
- **`TargetAbi`**：`Gnu`、`Musl`、`Msvc`、`None`。
- **`CompileTarget`**：
  - `parse(str)`：支持 `"linux-x64-musl"`、`"bun-darwin-arm64"`、`"windows-x86_64-msvc"`、`"wasi-wasm32"`、`"linux-x64-baseline"` 等格式并拦截非法组合（如 `darwin-musl`）；
  - `format(buf)` / `formatAlloc(allocator)`：输出规范化三元组；
  - `exeExt()`（`".exe"` / `".wasm"` / `""`）、`sharedLibExt()`（`".dll"` / `".dylib"` / `".wasm"` / `".so"`）、`isUnix()`。

### 3. 单文件可执行虚拟文件系统 (`lib/bundler/standalone.cp`)

- **二进制布局**：`[32B Header ("CPSG0001")] + [N * 32B Entry Descriptors] + [Payload] + [40B Footer Trailer ("\n---- CP! -----\n")]`。
- **`serialize(allocator)`**：生成自包含二进制映像，适合直接附加到原生可执行文件尾部。
- **`deserialize(allocator, bytes)`**：零拷贝反序列化（模块路径、内容与 SourceMap 切片直接指向原始字节缓冲），严格校验 Header Magic、Footer Trailer Magic、总字节长度、入口 ID 合法性及每条记录偏移边界，防止畸形输入越界读取。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    CompileTarget,
    StandaloneModuleGraph,
    StandaloneModuleEntry,
    ModuleGraph,
    ImportKind,
} from "cap:bundler";

pub fn demoBundlerDeep(): !void {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // 1. 交叉编译目标解析
    const tgt = CompileTarget.parse("bun-darwin-arm64")?;
    const is_unix = tgt.isUnix();          // true
    const dylib = tgt.sharedLibExt();      // ".dylib"
    * = is_unix;
    * = dylib;

    // 2. ModuleGraph 拓扑排序与树摇可达性
    let mg = ModuleGraph.init(alloc);
    const entry = mg.addModule("src/main.cp", true, 512)?;
    const util = mg.addModule("src/util.cp", false, 256)?;
    const dead = mg.addModule("src/dead.cp", false, 128)?;
    mg.addDependency(entry, util, "./util", ImportKind.ImportStmt)?;
    const order = mg.topoSort(alloc)?;     // [util, entry, dead]
    mg.markAllEntryPointsReachable();
    // mg.isReachable(util) == true, mg.isReachable(dead) == false
    * = order;
    * = dead;

    // 3. StandaloneModuleGraph 序列化与零拷贝反序列化
    let sg = StandaloneModuleGraph.init(alloc);
    * = sg.addModule(StandaloneModuleEntry{
        path: "/$cpfs/main.js",
        contents: "console.log('hello');",
        is_entry: true,
    })?;
    const blob = sg.serialize(alloc)?;
    let decoded = StandaloneModuleGraph.deserialize(alloc, blob)?;
    const content = decoded.findContent("/$cpfs/main.js");
    * = content;
}
```

### 4. Metafile 构建分析图与多入口代码分割引擎 (`lib/bundler/metafile.cp` & `lib/bundler/splitting.cp`)

- **`Metafile`**：esbuild / Bun 兼容的输入输出依赖图分析模型，支持 `addInput` / `addInputImport` / `addOutput` / `addOutputInput` / `addOutputImport` / `addOutputExport` / `setOutputCssBundle`，通过 `toJsonAlloc(alloc)` 导出标准 JSON，并通过 `formatAnalysisTable(alloc, use_color)` 输出按 `bytesInOutput` 降序排列的彩色体积占比树状报告。
- **`CodeSplitter` & `AssetManifest`**：基于 `ModuleGraph` 对最多 64 个入口点执行 `u64` 可达位掩码遍历，自动剔除死模块、将多入口公共依赖合并抽取为 `chunk-<hash>.<ext>` 共享 Chunk、计算确定性 8 位 Hex 内容哈希并构建跨 Chunk 导入边与 `AssetManifest`。

---

## 四、 上游行为契约、边界与安全注意事项

1. **交叉编译目标三元组边界（`CompileTarget.parse`）**：
   - 对齐 Bun `--compile --target` 规范，支持 `bun-linux-x64`、`linux-x64-musl`、`linux-x64-baseline`（兼容旧 CPU）、`bun-darwin-arm64`、`windows-x86_64-msvc`、`wasi-wasm32` 等组合；
   - 自动拦截非法 OS/ABI 组合（如 `darwin-musl`、`windows-gnu` 不兼容变体等）并返回错误，避免产出不可运行的交叉目标配置。
2. **依赖图环路与树摇边界（`ModuleGraph` & `CodeSplitter`）**：
   - `topoSort` 严格执行依赖优先（Dependencies-First）排序；当图中存在循环依赖时拒绝静默输出，确定性返回 `error.CyclicDependency`（可先用 `hasCycle()` 预检）；
   - `CodeSplitter` 使用单字 `u64` 入口可达位掩码实现零分配集合交并运算，单次分割支持最多 **64** 个入口点，超限时应分批构建或合并入口。
3. **单文件虚拟文件系统反序列化防越界（`StandaloneModuleGraph.deserialize`）**：
   - 反序列化完全零拷贝（模块路径、源码与 SourceMap 切片直接借用原始字节缓冲，调用方需保证底层缓冲生命周期长于 `StandaloneModuleGraph`）；
   - 严格校验 `CPSG0001` Header Magic、`"\n---- CP! -----\n"` 40B Footer Trailer、条目表总长、入口索引范围及每条 Payload 偏移区间，任何截断或畸形映像均安全报错退出。
