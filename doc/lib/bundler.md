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
| `lib/bundler/html_manifest.cp` | `// Source: /data/dev/bun/src/bundler/HTMLImportManifest.cp` | 零堆分配 HTML Import 清单构建器 `HtmlImportManifest`、`HtmlManifestFile`、`HtmlManifestLoader`、11 字符 URL-safe Base64 `encodeHashEtag` 与 RFC 8259 JSON 序列化 |
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
- **`OutputFileListBuilder` (`lib/bundler/output_file_list.cp`)**：吸收自 `/data/dev/bun/src/bundler/linker_context/OutputFileListBuilder.cp`，零堆分配三段式构建产物布局器（`[chunks | sourcemaps/bytecode/module_info | additional]`），保证 `outputIndexForChunk(chunk[i]) == i` 的 O(1) 零映射不变量，并在 `--compile` ESM 字节码与 standalone HTML 模式下精确预计算分区容量。
- **`entry_points.cp` (`lib/bundler/entry_points.cp`)**：吸收自 `/data/dev/bun/src/bundler/entry_points.cp`，零堆分配虚拟入口路径编解码器与框架引导代码生成器（`EntryPointError`、`EntryPointNamespace`、`MacroEntryId`、`isEntryPointPath`、`encodeEntryPointPath`、`decodeEntryPointPath`、`generateFallbackEntryCode`、`generateClientEntryCode`、`generateServerEntryCode`、`generateMacroEntryId`），支持 JS 字符串安全转义防注入与确定性 `macro://<16-hex>.js` 宏入口 ID 生成。

### 6. 虚拟入口点与宏 ID 生成示例（`entry_points.cp`）

```typescript
import { encodeEntryPointPath, decodeEntryPointPath, generateMacroEntryId } from "cap:bundler";

let pbuf: [128]u8 = undefined;
const virt = encodeEntryPointPath("/src/pages", "index", ".tsx", pbuf[0..])?; // "/src/pages/index.entry.tsx"
let spec_buf: [64]u8 = undefined;
const mid = generateMacroEntryId("/src/macros.ts", "buildTime", spec_buf[0..])?;
// spec_buf[0..mid.specifier_len] => "macro://<16-hex>.js"
```

### 5. 三段式 `OutputFileListBuilder` 示例

```typescript
import { ChunkDescriptor, OutputLayoutOptions, OutputFileListBuilder, OutputFileRecord } from "cap:bundler";

const chunks = [
    ChunkDescriptor{ content_kind: .javascript, has_external_sourcemap: true, is_js_like_loader: true },
    ChunkDescriptor{ content_kind: .css, has_external_sourcemap: true, is_js_like_loader: false },
];
const opts = OutputLayoutOptions{ external_source_maps: true, additional_output_files_count: 1 };
let list = OutputFileListBuilder.initFromLayout(opts, chunks[0..])?;
const idx0 = list.insertForChunk(OutputFileRecord{ path: "index.js", slot_kind: .chunk, byte_size: 2048 })?; // idx0 == 0
```

### 3.8 符号重命名器与最小化短名分配器（`lib/bundler/renamer.cp`，吸收自 `/data/dev/bun/src/js_printer/renamer.cp`）
- **常量与错误集**：`INLINE_NAME_CAP` (15)、`MAX_SCOPE_NAMES` (128)、`MAX_RESERVED_NAMES` (64)、`MAX_MINIFY_SLOTS` (128)、`DEFAULT_HEAD_ALPHABET`、`DEFAULT_TAIL_ALPHABET`、`RenamerError`
- **辅助与编码函数**：`isJsIdentStart`、`isJsIdentContinue`、`ensureValidJsIdentifier`、`isJsReservedWord`、`isCjsReservedName`、`encodeMinifiedName`、`encodeMinifiedNameWithAlphabet`（Base54 首字符 + Base64 后续字符，可与 `cap:char_freq` 频率表联动）
- **数据结构与分配器**：`SlotNamespace`（`default` / `label` / `private_name`）、`InlineName`（15B 栈上内联短名）、`NumberScopeRenamer`（作用域链冲突编号 `foo`/`foo2`/`foo3`）、`MinifySymbolSlot`、`MinifySlotAssigner`（按引用频率降序分配短名，自动跳过 JS 保留字/自定义保留名并遵守 JSX 首字母大写约束）

```typescript
import { NumberScopeRenamer, MinifySlotAssigner, encodeMinifiedName } from "cap:bundler";

let scope = NumberScopeRenamer.init();
let out: [32]u8 = [0; 32];
const n1 = scope.findUnusedName("foo", out[0..])?; // "foo"
const n2 = scope.findUnusedName("foo", out[0..])?; // "foo2"

let assigner = MinifySlotAssigner.init();
assigner.addReserved("a")?;
const s0 = assigner.addSlot(1, 100, false)?;
const s1 = assigner.addSlot(2, 50, true)?; // JSX 组件需大写首字母
assigner.assignByFrequency(.default)?;
// assigner.assignedName(s0) == "b", assigner.assignedName(s1) == "A"
```

### 3.9 Barrel 文件按需延迟加载优化引擎（`lib/bundler/barrel_imports.cp`，吸收自 `/data/dev/bun/src/bundler/barrel_imports.cp`）
- **常量与错误集**：`MAX_BARREL_REQUESTED_EXPORTS` (64)、`MAX_BARREL_REEXPORTS` (64)、`MAX_BARREL_IMPORT_RECORDS` (64)、`BarrelError`（`RequestedCapacityExceeded` / `ReExportCapacityExceeded` / `ImportRecordOutOfRange`）
- **数据模型与优化器**：`RequestedExportsKind`（`partial` / `all`）、`BarrelReExport`、`BarrelExportResolution`、`RequestedExportsSet`（跨调用去重与环路检测）、`BarrelEligibility`（`isEligible()` 纯重导出资格判定）、`BarrelPlanResult`（64 位 `active_mask` / `deferred_mask` 位图）、`BarrelWakeupResult`、`BarrelModule`（`addReExport` / `markExportStar` / `resolveExport` / `computePlan` / `requestExport` / `requestAll`，永不延迟 `export *` 记录并支持命名空间 `*` 下游全量传播）

```typescript
import { BarrelModule } from "cap:bundler";

let mod = BarrelModule.init(3)?;
mod.addReExport("Button", "Btn", 0, false)?;
mod.addReExport("Modal", "ModalDialog", 1, false)?;
mod.markExportStar(2)?; // export * 永不延迟

* = mod.requested.recordPartial("Button")?;
const plan = mod.computePlan(); // active: 0 & 2, deferred: 1
const wake = mod.requestExport("Modal")?; // newly_activated=true, propagate_alias="ModalDialog"
```

### 3.10 React Server Components 边界注册表（`lib/bundler/server_components.cp`，吸收自 `/data/dev/bun/src/js_parser/ast/ServerComponentBoundary.cp`）
- **常量与错误集**：`MAX_SCB_BOUNDARIES` (128)、`MAX_SCB_BITSET_WORDS` (16)、`MAX_SCB_BITSET_BITS` (1024)、`ScbError`（`DuplicateSourceIndex` / `BoundaryCapacityExceeded` / `SourceIndexOutOfRange`）
- **数据模型与注册表**：`UseDirective`（`none` / `use_client` / `use_server`，含 `parse` 与 `asStr`）、`ServerComponentBoundary`（`source_index` / `reference_source_index` / `ssr_source_index` / `hasSeparateSsr`）、`ScbBitSet`（1024 位零堆分配边界索引位图）、`ServerComponentBoundaryTable`（`put` / `putSharedSsr` / `getIndex` / `get` / `getReferenceSourceIndex` / `getSsrSourceIndex` / `getByReferenceIndex` / `countByDirective` / `buildBitSet`）

```typescript
import { UseDirective, ServerComponentBoundaryTable } from "cap:bundler";

const dir = UseDirective.parse("'use client';"); // UseDirective.use_client
let tbl = ServerComponentBoundaryTable.init();
* = tbl.put(10, dir, 100, 200)?; // source=10 -> ref=100, ssr=200
const bs = tbl.buildBitSet()?;   // bs.isSet(10) == true
```

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
