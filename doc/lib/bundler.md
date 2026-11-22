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
| `lib/bundler/htmlManifest.cp` | `// Source: /data/dev/bun/src/bundler/HTMLImportManifest.cp` | 零堆分配 HTML Import 清单构建器 `HtmlImportManifest`、`HtmlManifestFile`、`HtmlManifestLoader`、11 字符 URL-safe Base64 `encodeHashEtag` 与 RFC 8259 JSON 序列化 |
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
- **`OutputFileListBuilder` (`lib/bundler/outputList.cp`)**：吸收自 `/data/dev/bun/src/bundler/linker_context/OutputFileListBuilder.cp`，零堆分配三段式构建产物布局器（`[chunks | sourcemaps/bytecode/module_info | additional]`），保证 `outputIndexForChunk(chunk[i]) == i` 的 O(1) 零映射不变量，并在 `--compile` ESM 字节码与 standalone HTML 模式下精确预计算分区容量。
- **`entryPoints.cp` (`lib/bundler/entryPoints.cp`)**：吸收自 `/data/dev/bun/src/bundler/entry_points.cp`，零堆分配虚拟入口路径编解码器与框架引导代码生成器（`EntryPointError`、`EntryPointNamespace`、`MacroEntryId`、`isEntryPointPath`、`encodeEntryPointPath`、`decodeEntryPointPath`、`generateFallbackEntryCode`、`generateClientEntryCode`、`generateServerEntryCode`、`generateMacroEntryId`），支持 JS 字符串安全转义防注入与确定性 `macro://<16-hex>.js` 宏入口 ID 生成。

### 6. 虚拟入口点与宏 ID 生成示例（`entryPoints.cp`）

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

### 3.9 Barrel 文件按需延迟加载优化引擎（`lib/bundler/barrel.cp`，吸收自 `/data/dev/bun/src/bundler/barrel_imports.cp`）
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

### 3.10 React Server Components 边界注册表（`lib/bundler/rsc.cp`，吸收自 `/data/dev/bun/src/js_parser/ast/ServerComponentBoundary.cp`）
- **常量与错误集**：`MAX_SCB_BOUNDARIES` (128)、`MAX_SCB_BITSET_WORDS` (16)、`MAX_SCB_BITSET_BITS` (1024)、`ScbError`（`DuplicateSourceIndex` / `BoundaryCapacityExceeded` / `SourceIndexOutOfRange`）
- **数据模型与注册表**：`UseDirective`（`none` / `use_client` / `use_server`，含 `parse` 与 `asStr`）、`ServerComponentBoundary`（`source_index` / `reference_source_index` / `ssr_source_index` / `hasSeparateSsr`）、`ScbBitSet`（1024 位零堆分配边界索引位图）、`ServerComponentBoundaryTable`（`put` / `putSharedSsr` / `getIndex` / `get` / `getReferenceSourceIndex` / `getSsrSourceIndex` / `getByReferenceIndex` / `countByDirective` / `buildBitSet`）

```typescript
import { UseDirective, ServerComponentBoundaryTable } from "cap:bundler";

const dir = UseDirective.parse("'use client';"); // UseDirective.use_client
let tbl = ServerComponentBoundaryTable.init();
* = tbl.put(10, dir, 100, 200)?; // source=10 -> ref=100, ssr=200
const bs = tbl.buildBitSet()?;   // bs.isSet(10) == true
```

### 3.11 静态路由与 SSG 可达性分析器（`lib/bundler/staticRoute.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/StaticRouteVisitor.cp`）
- **常量与错误集**：`MAX_STATIC_ROUTE_NODES` (256)、`MAX_STATIC_ROUTE_EDGES` (1024)、`INVALID_ROUTE_SOURCE`、`StaticRouteError`（`NodeIndexOutOfRange` / `EdgeCapacityExceeded`）
- **数据模型与分析器**：`StaticRouteEdge`、`StaticRouteGraph`（`init` / `addImport`）、`StaticRouteVisitor`（`init` / `resetCache` / `isClientBoundary` / `hasTransitiveUseClient` / `isFullyStatic`，采用零堆分配迭代 DFS + 环路安全访问位图 + 三态记忆化缓存，直接联动 `ServerComponentBoundaryTable`）

```typescript
import { StaticRouteGraph, StaticRouteVisitor, ServerComponentBoundaryTable } from "cap:bundler";

let graph = StaticRouteGraph.init(4)?;
graph.addImport(0, 1)?;
graph.addImport(1, 3)?;

let scb = ServerComponentBoundaryTable.init();
* = scb.putSharedSsr(2, .use_client, 3)?; // client 边界生成的服务端引用模块为 3

let visitor = StaticRouteVisitor.init();
const is_ssg = visitor.isFullyStatic(&graph, &scb, 0)?; // false（传递依赖 client 边界）
```

### 3.12 反向导入图与变更影响分析器（`lib/bundler/changedFilt.cp`，吸收自 `/data/dev/bun/src/cli/test/ChangedFilesFilter.cp`）
- **错误集与结果类型**：`ChangedFilterError`（`OutOfMemory` / `ModuleIndexOutOfRange`）、`ChangedFilterSummary`、`AffectedEntryResult`（RAII `~init()` 确定性释放 `indices`）
- **核心引擎**：`ReverseImportGraph`（`init` / `addModule` / `addImport` / `addImportByPath` / `moduleCount` / `indexOf` / `importerCount` / `computeAffectedModules` / `filterAffectedEntries`）与 Git 输出路径解析辅助 `parseChangedPathLines` / `freeChangedPathList`
- **算法特性**：采用多源环路安全反向 BFS，自动去重反向边、忽略自环、支持未入图入口直接变更兜底保留，完整覆盖增量打包、HMR 失效冒泡与 `test --changed` 入口筛选场景。

```typescript
import { ReverseImportGraph } from "cap:bundler";

pub fn demoChangedFilter(allocator: std.mem.Allocator): ! {
    let g = ReverseImportGraph.init(allocator);
    defer g.deinit();

    g.addImportByPath("test/a.test.cp", "src/service.cp")?;
    g.addImportByPath("src/service.cp", "src/math.cp")?;
    g.addImportByPath("test/b.test.cp", "src/str.cp")?;

    const entries: [2][]const u8 = ["test/a.test.cp", "test/b.test.cp"];
    const changed: [1][]const u8 = ["src/math.cp"];
    let res = g.filterAffectedEntries(allocator, entries[0..], changed[0..])?;
    defer res.deinit(); // res.indices == [0] ("test/a.test.cp")
}
```

### 3.13 JS 后序 CSS 级联导入收集器（`lib/bundler/cssOrder.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/findImportedCSSFilesInJSOrder.cp`）
- **错误集与枚举**：`CssOrderError`（`OutOfMemory` / `SourceIndexOutOfRange`）、`CssOrderLoaderKind`（`js` / `css` / `other`，含 `isCss()`）
- **核心引擎**：`CssOrderGraph`（`init` / `addFile` / `addImport` / `fileCount` / `findImportedCssInJsOrder` / `findImportedCssForEntries` / `findAllInJsPostorder` + RAII `~init()`），采用显式迭代栈深度优先后序（DFS Postorder）遍历，环路安全且无深层递归栈溢出风险，保证共享基础样式始终先于上层组件样式输出。

```typescript
import { CssOrderGraph, CssOrderLoaderKind } from "cap:bundler";

pub fn demoCssOrder(allocator: std.mem.Allocator): ! {
    let g = CssOrderGraph.init(allocator);
    defer g.deinit();

    const app = g.addFile(CssOrderLoaderKind.js)?;
    const btn_css = g.addFile(CssOrderLoaderKind.css)?;
    const base_css = g.addFile(CssOrderLoaderKind.css)?;
    g.addImport(app, btn_css)?;
    g.addImport(btn_css, base_css)?;

    const css_list = g.findImportedCssInJsOrder(allocator, app)?;
    defer allocator.free(css_list); // [base_css, btn_css]
}
```

### 3.14 CSS Chunk 后处理与 SourceMap 相对行列偏移引擎（`lib/bundler/cssPost.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/postProcessCSSChunk.cp`）
- **常量与错误集**：`CSS_CHUNK_MAX_PARTS` (`64`)、`CssChunkPostProcessError`（`BufferTooSmall` / `TooManyParts`）
- **核心类型与函数**：`CssChunkSourceOffset`（`advance` / `reset` / `eql`，支持 `\n` 与 `\r\n`）、`CssChunkCompileResult`、`CssChunkPostProcessOptions`（`is_bundle` / `minify_whitespace` / `emit_source_maps` / `banner` / `footer`）、`CssSourceMapEntry`、`CssChunkPostProcessResult`（`sourceMapAt`）、`postProcessCssChunk`（零堆分配合并 CSS 片段、自动生成 `/* <path> */` 注释与空行分隔、追踪并重置 SourceMap 相对行列偏移、确保非空输出以 `\n` 结尾）。

```typescript
import {
    CssChunkCompileResult,
    CssChunkPostProcessOptions,
    postProcessCssChunk,
} from "cap:bundler";

const parts: [2]CssChunkCompileResult = [
    CssChunkCompileResult{ source_index: 0, pretty_path: "src/base.css", code: "body { margin: 0; }\n", has_source_map: true },
    CssChunkCompileResult{ source_index: 1, pretty_path: "src/app.css", code: ".app { color: red; }", has_source_map: true },
];
let out: [256]u8 = [0; 256];
const res = postProcessCssChunk(parts[0..], CssChunkPostProcessOptions{ emit_source_maps: true }, out[0..])?;
// res.source_map_count == 2，输出含 "/* src/base.css */\n" 与 "/* src/app.css */\n" 注释及末尾换行
* = res;
```

### 3.15 转译模块分析元数据与对齐二进制缓存（`lib/bundler/modInfo.cp`，吸收自 `/data/dev/bun/src/bundler/analyze_transpiled_module.cp`）
- **常量与错误集**：`MODULE_INFO_MAX_STRINGS` (`128`)、`MODULE_INFO_STRING_BUF_CAP` (`4096`)、`MODULE_INFO_MAX_RECORDS` (`128`)、`MODULE_INFO_MAX_BUFFER_IDS` (`384`)、`MODULE_INFO_MAX_REQUESTED` (`64`)、`MODULE_STRING_STAR_DEFAULT`、`MODULE_STRING_STAR_NAMESPACE`、`MODULE_FETCH_{NONE,JAVASCRIPT,WEBASSEMBLY,JSON}`、`ModuleInfoError`
- **核心类型与方法**：`ModuleRecordKind`（9 种 ESM/TS 记录类型）、`ModuleInfoFlags`（`contains_import_meta` / `is_typescript` / `has_tla` 单字节位打包）、`TranspiledModuleInfo`（`init` / `internString` / `getString` / `addDeclaredVariable` / `addLexicalVariable` / `addImportSingle` / `addImportNamespace` / `addExportIndirect` / `addExportLocal` / `addExportNamespace` / `addExportStar` / `requestModule` / `replaceStringId` / `finalize` / `serialize` / `deserialize`）。

```cp
import {
    TranspiledModuleInfo,
    MODULE_FETCH_JAVASCRIPT,
} from "cap:bundler";

let mi = TranspiledModuleInfo.init(true);
const mod_id = mi.internString("./dep.ts")?;
const sym_id = mi.internString("User")?;
mi.requestModule(mod_id, MODULE_FETCH_JAVASCRIPT)?;
mi.addImportSingle(mod_id, sym_id, sym_id, false)?;
mi.addExportLocal(sym_id, sym_id)?;
mi.finalize()?; // 自动将 ExportInfoLocal 提升为 ExportInfoIndirect，并将 TS 导入标记为 ImportInfoSingleTypeScript

let cache_buf: [512]u8 = [0; 512];
const n = mi.serialize(cache_buf[0..])?;
const restored = TranspiledModuleInfo.deserialize(cache_buf[0..n])?;
* = restored;
```

### 3.16 跨 Chunk 依赖分析与代码分割导入/导出表（`lib/bundler/crossChunk.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/computeCrossChunkDependencies.cp` 与 `/data/dev/bun/docs/bundler/index.mdx`）
- **常量与错误集**：`CROSS_CHUNK_MAX_CHUNKS` (`32`)、`CROSS_CHUNK_MAX_SYMBOLS` (`128`)、`CROSS_CHUNK_MAX_USES` (`256`)、`CROSS_CHUNK_MAX_EDGES` (`128`)、`CrossChunkError`
- **核心类型与方法**：`CrossChunkImportKind`（`Stmt` / `SideEffect` / `Dynamic`）、`CrossChunkNode`、`CrossChunkSymbol`、`CrossChunkImportEdge`、`CrossChunkExportEntry`、`CrossChunkResult`（`hasImportEdge` / `hasAnyStaticImportBetween` / `isSymbolExportedByChunk` / `importCountForChunk` / `exportCountForChunk`）、`CrossChunkAnalyzer`（`init` / `addChunk` / `addSymbol` / `recordSymbolUse` / `recordDynamicImport` / `compute`）。
- **代码分割（`--splitting`）契约**：多入口共享模块拆分为独立 chunk 后，自动将跨 chunk 符号引用汇总为去重且确定性排序的导入/导出表；遵循 CJS `namespace_alias` 重定向；对属于某入口点但无直接符号引用的共享 JS chunk 自动注入 `SideEffect` 导入以保证副作用按序执行；并追踪动态 `import()` 跨 chunk 依赖纳入最终内容哈希。

```cp
import { CrossChunkAnalyzer, CrossChunkImportKind } from "cap:bundler";

let analyzer = CrossChunkAnalyzer.init();
const entry_a = analyzer.addChunk(true, true, 0, 1)?;
const shared = analyzer.addChunk(true, false, 0, 1)?;
const sym_util = analyzer.addSymbol("sharedUtil", shared, false, false, null)?;
analyzer.recordSymbolUse(entry_a, sym_util)?;
const res = analyzer.compute()?;
// res.hasImportEdge(entry_a, shared, CrossChunkImportKind.Stmt, sym_util) == true
// res.isSymbolExportedByChunk(shared, sym_util) == true
* = res;
```

### 3.17 产物命名路径模板引擎（`lib/bundler/pathTpl.cp`，吸收自 `/data/dev/bun/src/bundler/options.cp` 与 `/data/dev/bun/docs/bundler/index.mdx`）
- **错误集与枚举**：`PathTemplateError`、`PathTemplateField`（`dir` / `name` / `ext` / `hash` / `target`）、`PathSeparatorStyle`（`posix` / `windows`）
- **核心类型与方法**：`PathTemplatePlaceholder`、`PathTemplate`（6 大官方预设 `file` / `fileWithTarget` / `chunk` / `chunkWithTarget` / `asset` / `assetWithTarget`、`fromString` / `withValues` / `needs` / `validate` / `format` / `formatWithStyle`）、`formatTruncatedHashHex`、`formatBundlerNaming`。
- **上游 `naming` 契约（参考 `/data/dev/bun/docs/bundler/index.mdx`）**：
  - 入口文件默认 `"[dir]/[name].[ext]"`，共享 chunk 默认 `"./chunk-[hash].[ext]"`，静态资源默认 `"./[name]-[hash].[ext]"`；当 `[dir]` 为空时自动回退为 `"."`；
  - `needs(PathTemplateField.hash)` 可在打包期静态跳过不必要的内容哈希计算；`formatWithStyle(.windows, true, ...)` 自动将 `/` 规范化为 `\`，并在严格模式下拦截未闭合 `[`、未知占位符、缺失哈希值以及 `..` 路径穿越。

```cp
import { PathTemplate, PathTemplatePlaceholder, PathSeparatorStyle } from "cap:bundler";

const vals = PathTemplatePlaceholder{
    dir: "pages/auth",
    name: "login",
    ext: "js",
    hash: 0x1234_5678_9abc_def0,
    target: "browser",
};
const tpl = PathTemplate.fromString("files/[dir]/[name]-[hash].[ext]").withValues(vals);
let buf: [128]u8 = [0; 128];
const n = tpl.formatWithStyle(PathSeparatorStyle.posix, true, buf[0..])?;
// "files/pages/auth/login-88888888.js"
* = n;
```

### 3.18 全局无副作用属性访问与树摇纯净度判定表（`lib/bundler/pureGlobals.cp`，吸收自 `/data/dev/bun/src/bundler/defines-table.cp` 与 `/data/dev/bun/docs/bundler/minifier.mdx`）
- **核心类型与函数**：`PureGlobalKind`（`unknown` / `nan_literal` / `infinity_literal` / `strict_undefined` / `pure_identifier`）、`PureGlobalFlags`、`lookupPureGlobalIdentifier`、`isPureGlobalIdentifier`、`isConstantFoldableGlobal`、`isPureGlobalPropertyAccess`、`isPureGlobalPropertyChain`、`isPureGlobalCallSafeForString`、`classifyGlobalExpr`。
- **DCE 与表达式重排契约**：
  - `NaN` / `Infinity` / `undefined` 标记为可常量折叠；明确将 `eval` 排除在纯全局白名单外以防破坏 direct eval 词法作用域语义；
  - `isPureGlobalPropertyChain("console.log")` 等判定仅表明“读取属性引用本身无副作用”（允许将 `if (a) console.log(b); else console.log(c);` 重排为 `console.log(a ? b : c)`），不代表调用函数本身无副作用；
  - `isPureGlobalCallSafeForString("Symbol", "for")` 对标 React 产物树摇优化，识别字符串参数下永不抛异常的全局调用。

```cp
import { classifyGlobalExpr, isPureGlobalPropertyChain } from "cap:bundler";

const f_sym = classifyGlobalExpr("Symbol.for");
// f_sym.safe_to_reorder_read == true && f_sym.call_is_pure_for_string_arg == true
const f_eval = classifyGlobalExpr("eval");
// f_eval.can_be_removed_if_unused == false
const ok = isPureGlobalPropertyChain("Object.prototype.hasOwnProperty"); // true
* = f_sym;
* = f_eval;
* = ok;
```

### 3.19 HTML 入口静态资源扫描器与无损 URL 改写引擎（`lib/bundler/htmlScan.cp`，吸收自 `/data/dev/bun/src/bundler/HTMLScanner.cp` 与 `/data/dev/bun/docs/bundler/html-static.mdx`）
- **常量与错误集**：`HTML_SCANNER_MAX_ASSETS` (`64`)、`HTML_SCANNER_MAX_SRCSET_CANDIDATES` (`16`)、`HtmlScannerError`
- **核心类型与函数**：`HtmlImportKind`（`stmt` / `at` / `url`）、`HtmlTagKind`（`script` / `link` / `img` / `video` / `audio` / `source`）、`HtmlAssetRef`（含 `isLocalBundleable()` 与精确字节偏移 `url_start`..`url_end`）、`SrcsetCandidate`、`HtmlRewritePair`、`HtmlScanResult`、`isExternalHtmlUrl`、`parseSrcsetCandidates`、`scanHtmlAssets`、`rewriteHtmlAssetUrls`。
- **HTML 打包契约（参考 `/data/dev/bun/docs/bundler/html-static.mdx`）**：
  - 自动发现 `<script src>`（`.stmt`）、`<link rel="stylesheet" href>` / `<link as="style" href>`（`.at`）、`<link as="worker" href>`（`.stmt`）以及字体/图片/视频/音频/Manifest/图标与 `<img/source srcset>` 多候选资源（`.url`）；
  - 自动跳过 `<!-- ... -->` 注释与 `<script>`/`<style>` 原始正文，避免误提取内联代码片段中的伪标签；自动识别 `http://`/`https://`/`//`/`data:`/`blob:`/`#` 等外部或内联 URI 使其不参与本地打包；
  - `rewriteHtmlAssetUrls` 按精确字节偏移将本地静态资源改写为带内容哈希的构建产物路径（支持 SPA 单入口回退与 MPA 多页入口静态站打包）。

```cp
import { scanHtmlAssets, rewriteHtmlAssetUrls, HtmlRewritePair } from "cap:bundler";

const html = "<html><head><link rel=\"stylesheet\" href=\"./styles.css\"><script type=\"module\" src=\"./app.ts\"></script></head><body><img src=\"./logo.png\"></body></html>";
const scan = scanHtmlAssets(html)?;
// scan.count == 3, scan.localAssetCount() == 3, scan.has_head == true

const pairs: [2]HtmlRewritePair = [
    HtmlRewritePair{ from_url: "./styles.css", to_url: "/assets/styles-abc123.css" },
    HtmlRewritePair{ from_url: "./app.ts", to_url: "/assets/app-def456.js" },
];
let out: [256]u8 = [0; 256];
const n = rewriteHtmlAssetUrls(html, pairs[0..], out[0..])?;
* = scan;
* = n;
```

### 3.20 JS Chunk 部件拓扑排序与线性化引擎（`lib/bundler/jsOrder.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/findAllImportedPartsInJSOrder.cp` 与 `/data/dev/bun/docs/bundler/index.mdx`）
- **常量与错误集**：`JS_ORDER_MAX_FILES` (`64`)、`JS_ORDER_MAX_PARTS_PER_FILE` (`16`)、`JS_ORDER_MAX_IMPORTS_PER_PART` (`8`)、`JS_ORDER_MAX_RANGES` (`256`)、`JS_ORDER_RUNTIME_SOURCE_INDEX` (`0`)、`JS_ORDER_INVALID_SOURCE_INDEX` (`0xFFFFFFFF`)、`JS_ORDER_NAMESPACE_EXPORT_PART` (`0`)、`JsOrderError`
- **核心类型与方法**：`JsPartWrapKind`（`None` / `Cjs` / `Esm`）、`JsOrderImportKind`（`Stmt` / `Require` / `DynamicImport`）、`JsOrderImportRecord`、`JsOrderPart`、`JsOrderFile`、`JsPartRange`（`len()` / `canExtend()`）、`JsChunkOrderEntry`（`isBefore()`）、`JsOrderResult`（`filesSlice()` / `partRangesSlice()` / `prefixRangesSlice()` / `mainRangesSlice()` / `totalParts()`）、`JsOrderGraph`（`init()` / `addFile()` / `setFileFlags()` / `addPart()` / `addPartImport()` / `findSlotBySourceIndex()` / `getScbChunkIndex()` / `computeChunkOrder()`）。
- **核心算法契约**：
  - 100% 零动态堆分配：所有文件、部件、导入记录与输出区间均驻留于定长栈/结构体数组中，环路安全且无深层递归风险；
  - 确定性排序：先按 `(distance_from_entry, stable_tie_breaker)` 稳定排序 Chunk 内文件，再优先遍历 Runtime 模块并执行依赖优先 DFS；
  - 紧凑区间合并：同一可拆分 ESM 模块的相邻存活部件自动合并为连续 `JsPartRange`；Runtime 部件及不可拆分的 CommonJS/ESM 包装模块整体归入 `parts_prefix` 置于主区间之前。

```cp
import { JsOrderGraph, JsPartWrapKind, JsOrderImportKind } from "cap:bundler";

let g = JsOrderGraph.init();
const s_dep = g.addFile(2, 1, 2, 0b1, JsPartWrapKind.None)?;
* = g.addPart(s_dep, false, true)?; // namespace export part (inactive)
* = g.addPart(s_dep, true, true)?;  // part 1
* = g.addPart(s_dep, true, true)?;  // part 2 (merged with part 1 -> [1, 3))

const s_entry = g.addFile(1, 0, 1, 0b1, JsPartWrapKind.None)?;
* = g.addPart(s_entry, false, true)?;
const p_entry = g.addPart(s_entry, true, true)?;
g.addPartImport(s_entry, p_entry, 2, JsOrderImportKind.Stmt, false)?;

const files: [2]u32 = [1, 2];
const res = g.computeChunkOrder(0b1, files[0..], 0, false, false)?;
// res.filesSlice() == [2, 1], res.totalParts() == 3
* = res;
```

### 3.21 HTML Chunk 编译注入与单文件 Standalone HTML 打包引擎（`lib/bundler/htmlChunk.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/generateCompileResultForHtmlChunk.cp` 与 `/data/dev/bun/docs/bundler/standalone-html.mdx`）
- **常量与错误集**：`HTML_CHUNK_MAX_PRELOADS` (`32`)、`HtmlChunkError`（`BufferTooSmall` / `TooManyPreloads` / `InvalidAssetUrl`）
- **核心类型与函数**：`HtmlEndTagIndices`（`headInsertOffset()` / `bodyInsertOffset()`）、`HtmlChunkCompileOptions`（`addModulePreload()`）、`HtmlChunkCompileResult`、`findHtmlEndTagIndices`、`writeEscapedAttrUrl`、`writeEscapedInlineScript`、`writeEscapedInlineStyle`、`compileHtmlChunk`。
- **核心编译契约**：
  - 100% 零动态堆分配：精准定位 `</head>`、`</body>`、`</html>` 锚点并跳过 `<!-- ... -->` 注释及 `<script>`/`<style>` 原始正文；
  - 外链打包模式：按 CSS -> `modulepreload` -> JS 顺序注入 `<head>`（减少 FOUC 闪烁）并支持 `crossorigin` 与属性安全转义；
  - Standalone HTML 模式：内联 `<style>` 至 `</head>` 前，内联 `<script type="module">` 至 `</body>` 前确保 DOM 已挂载，并对内联正文中的 `</script>`/`</style>` 执行安全转义防止标签提前闭合。

```cp
import { HtmlChunkCompileOptions, compileHtmlChunk } from "cap:bundler";

const html = "<!doctype html><html><head><title>App</title></head><body><div id=\"root\"></div></body></html>";
let opts = HtmlChunkCompileOptions{
    standalone_html: true,
    minify_whitespace: true,
    css_ref: "body{margin:0}",
    js_ref: "console.log('ready')",
};
let out: [512]u8 = [0; 512];
const compiled = compileHtmlChunk(html, opts, out[0..])?;
// CSS 内联至 </head> 前，JS 内联至 </body> 前
* = compiled;
```

### 3.22 CSS `@import` 依赖排序与 Last-Wins 层叠去重引擎（`lib/bundler/cssImpOrder.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/findImportedFilesInCSSOrder.cp` 与 `/data/dev/bun/docs/bundler/css.mdx`）
- **常量与错误集**：`CSS_IMPORT_MAX_FILES` (`64`)、`CSS_IMPORT_MAX_RULES_PER_FILE` (`16`)、`CSS_IMPORT_MAX_CONDITIONS` (`8`)、`CSS_IMPORT_MAX_ORDER_ENTRIES` (`256`)、`CSS_IMPORT_MAX_DEPTH` (`32`)、`CssImportOrderError`
- **核心类型与方法**：`CssImportCondition`、`CssConditionStack`（`push()` / `eql()` / `isRedundantBefore()`）、`CssImportRuleKind`（`Internal` / `External` / `Composes`）、`CssImportRule`、`CssImportFile`、`CssImportEntryKind`（`SourceFile` / `ExternalUrl` / `LayersOnly`）、`CssImportOrderEntry`、`CssImportOrderResult`、`CssImportGraph`（`init()` / `addFile()` / `addImport()` / `addExternalImport()` / `addComposes()` / `computeCssOrder()`）。
- **核心算法契约**：
  - 100% 零动态堆分配：遵循 WebKit `StyleRuleImport` 父链环路检测语义，安全处理循环 `@import`；
  - 外链 `@import` 自动顶置至文件头部以满足 CSS 语法规范；
  - 逆序 Last-Wins 去重：菱形依赖 `A -> B, C` 且 `B, C -> D` 初始遍历为 `D B D C A`，去重后保留最后一次求值 `B D C A`；若 `D` 含 `@layer` 声明，则首次出现降级为 `LayersOnly` 占位符以保留 CSS Cascade Layers 声明顺序；条件 `@import`（`layer`/`supports`/`media`）仅在后继无条件或条件链完全一致时才判定前驱冗余。

```cp
import { CssImportGraph, CssImportCondition } from "cap:bundler";

let g = CssImportGraph.init();
const s_d = g.addFile(4, true)?; // D 含 @layer 声明
const s_b = g.addFile(2, false)?;
const s_c = g.addFile(3, false)?;
const s_a = g.addFile(1, false)?;
const none = CssImportCondition{};
g.addImport(s_a, 2, none)?;
g.addImport(s_a, 3, none)?;
g.addImport(s_b, 4, none)?;
g.addImport(s_c, 4, none)?;

const entries: [1]u32 = [1];
const res = g.computeCssOrder(entries[0..])?;
// 输出: D(LayersOnly) -> B -> D(SourceFile) -> C -> A
* = res;
```

### 3.23 Bundler Chunk 规划与确定性排序引擎（`lib/bundler/chunkPlan.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/computeChunks.cp` 与 `/data/dev/bun/docs/bundler/index.mdx`）
- **常量与错误集**：`CHUNK_PLAN_MAX_ENTRIES` (`32`)、`CHUNK_PLAN_MAX_FILES` (`64`)、`CHUNK_PLAN_MAX_CHUNKS` (`64`)、`CHUNK_PLAN_MAX_FILES_PER_CHUNK` (`64`)、`CHUNK_PLAN_MAX_CSS_DEPS` (`16`)、`CHUNK_PLAN_INVALID_INDEX` (`0xFFFFFFFF`)、`CHUNK_PLAN_KEY_CAP` (`32`)、`ChunkPlanError`
- **核心类型与函数**：`ChunkPlanContentKind`（`Javascript` / `Css` / `Html`，含 `ext()`）、`ChunkPlanLoaderKind`、`ChunkPlanEntryKind`（`UserSpecified` / `DynamicImport` / `ServerComponent`）、`ChunkPlanTarget`（`Browser` / `Bun` / `Node` / `BunMacro` / `BakeSsr`，含 `isServerSide()` / `asLabel()`）、`ChunkPlanTemplateKind`（`EntryFile` / `EntryFileWithTarget` / `SharedChunk` / `SharedChunkWithTarget`）、`ChunkPlanOptions`、`ChunkPlanFileInput`、`ChunkPlanUniqueKey`、`PlannedChunk`、`EntryChunkMapping`、`ChunkPlanResult`、`hashCssOrderKey()`、`formatChunkUniqueKey()`、`ChunkPlanner`（`init()` / `addFile()` / `addEntryPoint()` / `attachCssDep()` / `setFileEntryBits()` / `computeChunks()`）。
- **核心算法契约**：
  - 100% 零动态堆分配：基于 `u64` 入口可达位掩码，在 `code_splitting = true` 时将共享同一入口子集的活文件聚合成独立共享 Chunk，在 `code_splitting = false` 时按置位掩码扇出到每个入口 Chunk；
  - CSS Chunk 去重与索引重映射：启用 `css_chunking` 时按有序 CSS 依赖列表 64 位 FNV-1a 哈希复用共享 CSS Chunk；排序阶段将 CSS Chunks 按哈希升序排布在 JS/HTML 之后，并自动把所有 JS Chunk 引用的 `css_chunk_index` 重映射为最终索引；
  - 服务端全栈构建兼容：当 `target.isServerSide() && has_html_imports` 时自动为浏览器端入口及含浏览器端文件的共享 Chunk 传播 `is_browser_chunk_from_server_build` 标记，并按 Bun 规则生成 `<hex_lower>C<08d>` 确定性唯一键与 `[target]` 命名模板类型。

```cp
import {
    ChunkPlanner,
    ChunkPlanFileInput,
    ChunkPlanOptions,
} from "cap:bundler";

let planner = ChunkPlanner.init();
* = planner.addFile(ChunkPlanFileInput{ source_index: 1, entry_bits: 0b01, output_base_name: "entry-a" })?;
* = planner.addFile(ChunkPlanFileInput{ source_index: 2, entry_bits: 0b10, output_base_name: "entry-b" })?;
* = planner.addFile(ChunkPlanFileInput{ source_index: 3, entry_bits: 0b11, output_base_name: "shared" })?;
* = planner.addEntryPoint(1)?;
* = planner.addEntryPoint(2)?;

const plan = planner.computeChunks(ChunkPlanOptions{
    code_splitting: true,
    unique_key: 0xbeef,
})?;
// 产出 3 个 JS Chunk：entry-a (0b01)、entry-b (0b10) 与共享 chunk (0b11，含 source_index=3)
* = plan;
```

### 3.24 Chunk 级符号重命名与字符频次压缩协调器（`lib/bundler/chunkRename.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/renameSymbolsInChunk.cp` 与 `/data/dev/bun/docs/bundler/minifier.mdx`）
- **常量与错误集**：`CHUNK_RENAME_MAX_SYMBOLS` (`64`)、`CHUNK_RENAME_MAX_RESERVED` (`32`)、`CHUNK_RENAME_NAME_CAP` (`48`)、`ChunkRenameError`
- **核心类型与方法**：`ChunkStableRef`（`eql`/`lessThan`）、`ChunkSlotCounts`（`unionMax`）、`ChunkCharFreq`（`init`/`scan`/`include`/`compileAlphabets`）、`ChunkRenameSymbolKind`（`TopLevel` / `CrossChunkImport` / `CjsWrapper` / `CjsHoistedExternalImport`）、`ChunkRenameSymbolInput`、`ChunkRenamedName`、`ChunkRenameEntry`、`ChunkRenameOptions`、`ChunkRenameResult`（`slice`/`lookupName`）、`ChunkSymbolRenamer`（`init`/`addReservedName`/`mergeNestedSlots`/`includeCharFreq`/`addSymbol`/`rename`）。
- **核心算法契约**：
  - 100% 零动态堆分配：通过 `ChunkSlotCounts.unionMax` 合并嵌套作用域槽位上界，保证顶层压缩槽位与任意嵌套作用域零冲突；
  - 频次优化短名与压缩友好字母表：`minify_identifiers = true` 时按符号使用频次降序分配短名（同频按 `ChunkStableRef` 稳定破平局），并基于 `ChunkCharFreq` 生成更利于 Gzip/Brotli 压缩的自定义字母表；
  - 非压缩可读重命名与保留字防线：`minify_identifiers = false` 时跨 Chunk 导入符号优先占位，同名冲突自动递增数字后缀（`foo` / `foo2` / `foo3`），自动规避 JS 保留字、CJS 保留全局（`exports`/`module`/`require`）及 `must_not_be_renamed` 锁定符号。

```cp
import {
    ChunkStableRef,
    ChunkCharFreq,
    ChunkRenameSymbolInput,
    ChunkRenameOptions,
    ChunkSymbolRenamer,
} from "cap:bundler";

let ren = ChunkSymbolRenamer.init();
let freq = ChunkCharFreq.init();
freq.scan("zzzzzzzzz");
ren.includeCharFreq(freq);
* = ren.addSymbol(ChunkRenameSymbolInput{
    sym_ref: ChunkStableRef{ stable_source_index: 1, inner_index: 0 },
    original_name: "heavyCounter",
    use_count: 100,
})?;
const res = ren.rename(ChunkRenameOptions{ minify_identifiers: true })?;
// res.lookupName({1, 0}) == "z"（最高频字符优先分配给最高频符号）
* = res;
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
4. **HMR 边界冒泡与失效传播实践（吸收自 `/data/dev/bun/docs/bundler/hot-reloading.mdx`）**：
   - 热更新从变更叶节点沿反向导入图（`ReverseImportGraph`）向上冒泡，直至命中调用 `import.meta.hot.accept()` 的模块边界（仅重评估边界模块并修补其引用方，保留未变更兄弟模块状态）；若冒泡至入口仍无接受边界，则触发整页重载；
   - 生产构建应对 `import.meta.hot` 分支执行死代码消除（DCE），因此业务代码中必须直接书写完整 `import.meta.hot.<API>` 调用，禁止将其赋值给中间变量或作为高阶回调传递。
5. **CSS 打包级联顺序与语法降级基线（吸收自 `/data/dev/bun/docs/bundler/css.mdx`）**：
   - 当 JS/TS 与 CSS 混排打包时，必须按 JS 深度优先后序（`CssOrderGraph.findImportedCssInJsOrder`）合并样式表，确保菱形依赖中的基础样式先于上层样式输出，避免级联覆盖错乱；
   - 现代 CSS 语法（CSS Nesting、嵌套 `@media`、`color-mix()` 等）默认面向 ES2020 / Chrome 87+ / Safari 14+ / Firefox 78+ / Edge 88+ 自动展平降级并补全厂商前缀。
6. **生产压缩三模态与死代码消除策略（吸收自 `/data/dev/bun/docs/bundler/minifier.mdx`）**：
   - 打包压缩分为三类可独立开关的粒度：`--minify-whitespace`（移除空白、换行与打包注释头，配合 `postProcessCssChunk` 自动省略 `/* path */` 注释）、`--minify-syntax`（常量折叠、死代码消除、布尔字面量 `true`/`false` 缩短为 `!0`/`!1` 及布尔代数化简）与 `--minify-identifiers`（基于频次的局部标识符短名重命名，对应 `NumberScopeRenamer`）；
   - 生产模式（`--production`）默认同时启用全部三类压缩、注入 `process.env.NODE_ENV="production"` 并切换至生产态 JSX 转换；需注意 SourceMap 开启时 `postProcessCssChunk` 按带 SourceMap 的片段边界重置相对行列偏移，确保下游 VLQ 拼接零漂移。
7. **转译器扫描与 TypeScript 类型消除（吸收自 `/data/dev/bun/docs/runtime/transpiler.mdx`）**：
   - 除非同时转译大量超大文件，优先使用同步转译（`.transformSync()`）以避免线程池调度开销高于转译本身；异步转译（`.transform()`）按 `Math.floor(cpu_count * 0.8)` 分配 Worker 线程；
   - `.scan()` 静态扫描模块导入/导出元数据时自动忽略 TypeScript 的 `type-only` 导入导出；在 `TranspiledModuleInfo.finalize()` 中，若模块标记为 `is_typescript = true` 且本地重导出引用了导入符号，会自动将该导入升级为 `ImportInfoSingleTypeScript`，防止链接期因类型导出被消除而误报符号缺失。
8. **代码分割（`splitting`）与模块格式（`esm` / `cjs`）Chunk 线性化实践（吸收自 `/data/dev/bun/docs/bundler/index.mdx`）**：
   - 当启用 `--splitting`（`splitting: true`）且多个入口点共享同一模块时，共享代码会被提取至带内容哈希的独立 Chunk（如 `chunk-[hash].js`）；此时 `JsOrderGraph.computeChunkOrder(..., code_splitting = true)` 严格要求文件的 `entry_bits` 与目标 Chunk 完全相等，确保共享模块仅在对应共享 Chunk 中发射一次，避免跨入口重复打包；
   - 选择 `format: "cjs"` 时默认目标切换为 `node`（`target: "bun"` 与 `format: "cjs"` 组合会附加 `// @bun @bun-cjs` 标记，其 CommonJS 包装器不兼容 Node.js）；被 `__commonJS` / `__esm` 包装的模块（`JsPartWrapKind.Cjs` / `.Esm`）不可按 Part 拆解，线性化器会将其全部部件作为单一连续区间提升至 `parts_prefix`，保证运行时延迟初始化闭包在任意消费方之前完成定义；外部动态 `import()` 边在静态 DFS 中自动截断，防止将按需懒加载分支错误内联进首屏同步 Chunk。
9. **单文件 Standalone HTML 打包与防闭合标签注入（吸收自 `/data/dev/bun/docs/bundler/standalone-html.mdx`）**：
   - 使用 `bun build --compile --target=browser ./index.html` 可将整个前端应用（JS/TS/JSX、CSS、图片、字体、WASM）内联为单一自包含 `.html` 文件，无需任何相对路径或本地 HTTP 服务即可离线双击打开、嵌入 WebView 或单文件上传至 S3/CDN；
   - 在 `compileHtmlChunk(..., standalone_html = true)` 中，CSS 内联为 `<style>` 置于 `</head>` 前以避免样式闪烁，JS 内联为 `<script type="module">` 置于 `</body>` 前以确保 DOM 节点完成解析；内联正文必须经过 `writeEscapedInlineScript` / `writeEscapedInlineStyle` 将任意大小写的 `</script`/`</style` 转义为 `<\/script`/`<\/style`，外链 URL 必须经过 `writeEscapedAttrUrl` 防止属性逃逸。
10. **CSS `@import` 多次求值与 `@layer` 首次声明保留（吸收自 `/data/dev/bun/src/bundler/linker_context/findImportedFilesInCSSOrder.cp` & `/data/dev/bun/docs/bundler/css.mdx`）**：
    - 与 JS ESM 模块“首次求值缓存”语义不同，CSS `@import` 每次出现均参与层叠覆盖，因此 `CssImportGraph` 采用“保留最后一次（Last-Wins）”去重策略；但 CSS `@layer` 的优先级由**首次声明顺序**决定，故当被覆盖的早先文件声明了 `@layer` 时必须降级保留为 `CssImportEntryKind.LayersOnly`；
    - 外部 `@import`（如 CDN 字体样式）必须被顶置至打包文件最顶部以保证浏览器正常解析；带 `@media`/`@supports` 条件的后继导入不能覆盖无条件或不同条件的前驱导入。
11. **多入口 Chunk 规划、`naming` 模板占位符与 `cssChunking` 最佳实践（吸收自 `/data/dev/bun/src/bundler/linker_context/computeChunks.cp` & `/data/dev/bun/docs/bundler/index.mdx`）**：
    - 当 `splitting: true` 且多个入口点共享模块时，`ChunkPlanner.computeChunks` 自动按精确 `entry_bits` 位掩码抽离共享 Chunk，而 `splitting: false` 时将共享模块内联扇出到每个引用入口以产出单文件自包含产物；
    - `naming` 支持 `[dir]`、`[name]`、`[hash]`、`[ext]` 与服务端多目标场景下的 `[target]` 占位符；在同时存在服务端入口与 HTML 浏览器入口的构建中，未显式指定 `entry_naming` / `chunk_naming` 时自动启用 `EntryFileWithTarget` / `SharedChunkWithTarget`，避免服务端与浏览器同名产物相互覆盖；
    - 开启 `cssChunking` 时，引用完全相同有序 CSS 依赖序列的多个入口会复用同一 CSS Chunk，排序后通过重映射表同步更新 JS Chunk 的 `css_chunk_index`，保证跨平台构建产物顺序与唯一键（`<hex>C<08d>`）100% 确定性可复现。
12. **延迟分段组装、Standalone HTML 内联转义与 `//# debugId` 发射（`lib/bundler/chunkOut.cp`，吸收自 `/data/dev/bun/src/bundler/Chunk.cp` & `/data/dev/bun/docs/bundler/standalone-html.mdx`）**：
    - **两遍零分配组装（`ChunkPieceAssembler`）**：在最终内容哈希文件名尚未确定前，代码生成器将源码切分为若干 `ChunkPiece`（前置字面量 + `ChunkPieceQuery{Chunk/Asset/Scb/HtmlImport}`）；待路径或内联内容就绪后，`estimateOutputSize()` 精确预计算总字节数，`assemble()` 单遍零拷贝拼接输出；
    - **Standalone HTML 闭合标签安全转义（`countClosingTagOccurrences` / `escapeClosingTagsInto`）**：当 `standalone_mode = true` 时，`.Chunk` 引用直接内联对应 JS/CSS 正文并将任意大小写变体（`</script`、`</SCRIPT`、`</style`）的 `</` 转义为 `<\/`（同时保留 `</div>` 等非匹配标签不变），`.Asset` 引用直接内联 `data:` URI；
    - **跨平台路径前缀规范化与 Debug ID 注释**：`normalizeChunkImportPath` 自动将 Windows `\` 转为 POSIX `/` 并消除重复 `./` 与多余双斜杠；开启 `emit_debug_id = true` 时在 `//# sourceMappingURL` 之前追加确定性 50 字节 `\n//# debugId=00000000-0000-0000-XXXX-XXXXXXXXXXXX\n`。

```cp
import {
    ChunkPieceQueryKind,
    ChunkPieceQuery,
    ChunkInlineKind,
    ChunkRefTarget,
    ChunkOutputAssembleOptions,
    ChunkPieceAssembler,
} from "cap:bundler";

let asm = ChunkPieceAssembler.init();
asm.addPiece("<style>", ChunkPieceQuery.init(ChunkPieceQueryKind.Chunk, 0))?;
asm.addPiece("</style><img src=\"", ChunkPieceQuery.init(ChunkPieceQueryKind.Asset, 0))?;
asm.addPiece("\"><script type=\"module\">", ChunkPieceQuery.init(ChunkPieceQueryKind.Chunk, 1))?;
asm.addLiteral("</script>")?;

const chunks: [2]ChunkRefTarget = [
    ChunkRefTarget{ rel_path: "styles.css", inline_content: "body{content:'</STYLE>'}", kind: ChunkInlineKind.Css },
    ChunkRefTarget{ rel_path: "app.js", inline_content: "console.log('</script>')", kind: ChunkInlineKind.JavaScript },
];
const assets: [1]ChunkRefTarget = [
    ChunkRefTarget{ rel_path: "logo.png", inline_content: "data:image/png;base64,iVBORw0KGgo=", kind: ChunkInlineKind.Html },
];
const empty: [0]ChunkRefTarget = [];
const opts = ChunkOutputAssembleOptions{ standalone_mode: true };

let out: [512]u8 = [0; 512];
const n = asm.assemble(chunks[0..], assets[0..], empty[0..], empty[0..], opts, out[0..])?;
// out[0..n] 中 </STYLE> 与 </script> 均已安全转义为 <\/STYLE> 与 <\/script>，且 logo.png 内联为 data: URI
* = n;
```

- **Bundler 模块图与两阶段扫描/延迟插件协调器（`graph.cp`，吸收自 `/data/dev/bun/src/bundler/Graph.cp` & `/data/dev/bun/docs/bundler/plugins.mdx`）**：
    - 导出：`GRAPH_MAX_INPUT_FILES` (`64`)、`GRAPH_MAX_ENTRY_POINTS` (`16`)、`GRAPH_MAX_HTML_IMPORTS` (`16`)、`GRAPH_MAX_PATH_LEN` (`128`)、`GraphError`、`GraphLoaderKind`、`GraphScanTransition`（`Scanning` / `WakeDeferredPlugins` / `ReadyToLink`）、`GraphInputFileFlags`、`GraphInputFile`、`GraphEntryPoint`、`GraphHtmlImportPair`、`GraphScanStats`、`BundlerGraph`；
    - **插件 `onLoad` `.defer()` 官方约束与两阶段调度**：`incrementScanCounter()` / `decrementScanCounter()` / `deferTask(source_index)` 精确建模 `pending_items` 与 `deferred_pending` 转移——当 `pending_items` 归零且存在 `.defer()` 任务时自动唤醒进入新一轮扫描（`WakeDeferredPlugins`），两者均归零才迁移到 `ReadyToLink`；同时强制执行 Bun 插件文档约束“每个 `onLoad` 回调仅允许调用 `.defer()` 一次”，二次调用返回 `GraphError.DeferAlreadyCalled`；
    - **零重扫描预统计**：`addInputFile` 按路径自动去重并实时累计 `css_file_count`、`estimated_file_loader_count` 与 `has_any_secondary_paths`，同时支持虚拟入口原始名映射（`addEntryPoint`）与服务端->客户端 HTML 导入对追踪（`addHtmlImport`）。

```cp
import { BundlerGraph, GraphLoaderKind, GraphScanTransition } from "cap:bundler";

let g = BundlerGraph.init();
const app_id = g.addInputFile("src/app.ts", "", GraphLoaderKind.Ts, false, 0x1111)?;
const stats_id = g.addInputFile("virtual:stats.json", "", GraphLoaderKind.Json, true, 0x2222)?;
* = g.addEntryPoint(app_id, "src/app.ts")?;

* = g.incrementScanCounter()?; // app.ts
* = g.incrementScanCounter()?; // stats.json (onLoad)
const t1 = g.deferTask(stats_id)?; // Scanning (pending=1, deferred=1)
const t2 = g.decrementScanCounter()?; // WakeDeferredPlugins (pending=1, deferred=0)
const t3 = g.decrementScanCounter()?; // ReadyToLink (pending=0, deferred=0)
* = t1;
* = t2;
* = t3;
```

- **Bundler 路径 → `SourceIndex` 命名空间隔离映射表（`pathSrcMap.cp`，吸收自 `/data/dev/bun/src/bundler/PathToSourceIndexMap.cp` & `/data/dev/bun/docs/bundler/plugins.mdx`）**：
    - 导出：`PATH_SRC_MAP_BUCKETS` (`256`)、`PATH_SRC_MAP_MAX_LOAD` (`192`)、`PATH_SRC_INVALID_INDEX` (`0xFFFF_FFFF`)、`PathSourceMapError`、`BundlerModulePath`（`file`/`withNamespace`/`isFileNamespace`/`isVirtual`/`eql`/`hash64`/`formatInto`）、`GetOrPutSourceResult`、`PathToSourceIndexMap`（`init`/`count`/`isEmpty`/`get`/`getPath`/`contains`/`containsPath`/`put`/`putPath`/`getOrPut`/`getOrPutPath`/`remove`/`removePath`/`reverseLookup`/`clear`）；
    - **插件 `namespace` 隔离最佳实践（参考 `/data/dev/bun/docs/bundler/plugins.mdx`）**：`onResolve` 返回 `{ path, namespace }` 时，不同命名空间（如 `"file"` vs `"virtual"` / `"env"`）下的同名路径（如 `config.json`）在 `BundlerModulePath.hash64()` 中通过 `namespace + 0xFF + text` 严格隔离，避免虚拟模块与磁盘文件发生 `SourceIndex` 碰撞；
    - **零堆分配与墓碑安全删除**：采用 256 槽开放寻址哈希表（75% 负载上限 192），支持 `remove` / `removePath` 墓碑回收而不破坏探测链，并支持 `reverseLookup(source_index)` 诊断反查。

```cp
import { BundlerModulePath, PathToSourceIndexMap } from "cap:bundler";

let map = PathToSourceIndexMap.init();
const r1 = map.getOrPut("src/config.json", 1)?; // source_index=1, found_existing=false
const r2 = map.getOrPutPath(BundlerModulePath.withNamespace("virtual", "src/config.json"), 2)?; // 命名空间隔离, source_index=2
const r3 = map.getOrPut("src/config.json", 99)?; // 命中已有, source_index=1, found_existing=true
* = r1;
* = r2;
* = r3;
```

- **Bundler 延迟插件批次协调器（`deferBatch.cp`，吸收自 `/data/dev/bun/src/bundler/DeferredBatchTask.cp` & `/data/dev/bun/docs/bundler/plugins.mdx`）**：
    - 导出：`DEFERRED_BATCH_MAX_TASKS` (`64`)、`DeferredBatchError`、`DeferredBatchState`（`Idle`/`Scheduled`/`Running`）、`DeferredTaskOutcome`（`Pending`/`Resumed`/`Rejected`）、`DeferredPluginTask`、`DeferredDrainSummary`、`DeferredBatchCoordinator`（`init`/`isIdle`/`isScheduled`/`isRunning`/`pendingCount`/`hasPending`/`hasDeferredSource`/`enqueueDeferred`/`schedule`/`beginRun`/`drainDeferred`/`runBatchNow`/`reset`）；
    - **`onLoad` `.defer()` 多波次唤醒与异常快速失败契约（参考 `/data/dev/bun/docs/bundler/plugins.mdx`）**：当 `BundlerGraph` 迁移至 `WakeDeferredPlugins` 时，协调器按严格非重入状态机 `Idle -> Scheduled -> Running -> Idle` 唤醒当前波次全部挂起的 `.defer()` 任务；若构建已出现致命错误，调用 `drainDeferred(true, ...)` 将整批任务标记为 `Rejected` 避免插件 Promise 永久悬空；同一 `source_index` 跨波次重复 `.defer()` 将被 `DuplicateSourceIndex` 拦截。

```cp
import { DeferredBatchCoordinator, DeferredPluginTask, DeferredTaskOutcome } from "cap:bundler";

let coord = DeferredBatchCoordinator.init();
* = coord.enqueueDeferred(10, 1)?;
* = coord.enqueueDeferred(11, 2)?;
let out: [8]DeferredPluginTask = [DeferredPluginTask{}; 8];
const summary = coord.runBatchNow(false, out[0..])?;
// summary.drained_count == 2, out[0].outcome == DeferredTaskOutcome.Resumed, coord.isIdle() == true
* = summary;
```

- **Bundler 索引 → 字符串零堆映射表（`idxStrMap.cp`，吸收自 `/data/dev/bun/src/bundler/IndexStringMap.cp` & `/data/dev/bun/docs/bundler/index.mdx`）**：
    - 导出：`INDEX_STR_MAP_BUCKETS` (`256`)、`INDEX_STR_MAP_MAX_LOAD` (`192`)、`INDEX_STR_MAP_ARENA_CAP` (`8192`)、`IndexStringMapError`、`IndexStringEntry`、`IndexStringMap`（`init`/`count`/`isEmpty`/`arenaUsed`/`arenaRemaining`/`get`/`contains`/`put`/`putCopy`/`getOrPut`/`remove`/`findIndexByValue`/`collectSortedEntries`/`clear`）；
    - **双模式零堆所有权与确定性输出**：静态/长生命周期字符串使用 `put()` 零拷贝存储；由路径模板或插件动态拼接的临时栈缓冲字符串使用 `putCopy()` 拷贝入内置 8 KiB 定长字节池，避免悬垂引用且完全零堆分配；生成 Metafile / HTML Manifest 时通过 `collectSortedEntries()` 按 `SourceIndex` 严格升序导出，保证跨平台构建产物字节级确定性。

```cp
import { IndexStringMap, IndexStringEntry } from "cap:bundler";

let map = IndexStringMap.init();
* = map.put(2, "chunk-b.js")?;
* = map.putCopy(1, "chunk-a.js")?;
let sorted: [4]IndexStringEntry = [IndexStringEntry{}; 4];
const n = map.collectSortedEntries(sorted[0..])?;
// n == 2, sorted[0] == (1, "chunk-a.js"), sorted[1] == (2, "chunk-b.js")
* = n;
```

- **Bundler 构建产物模型与安全落盘路径解析（`outputFile.cp`，吸收自 `/data/dev/bun/src/bundler/OutputFile.cp` & `/data/dev/bun/docs/bundler/index.mdx`）**：
    - 导出：`OUTPUT_FILE_MAX_COLLECTION` (`64`)、`OutputFileError`、`OutputFileKind`（`EntryPoint`/`Chunk`/`Asset`/`Sourcemap`/`Bytecode`）、`OutputValueKind`（`Move`/`Copy`/`Noop`/`Buffer`/`Pending`/`Saved`）、`OutputLoaderKind`（含 `inferFromPath` 与 `mimeType`）、`OutputBakeExtra`、`BuildOutputFile`（`fromBuffer`/`fromMove`/`fromCopy`/`noop`/`asSlice`/`mimeType`/`markSaved`/`resolveFullPath`/`formatSummaryLine`）、`BuildOutputCollection`（`init`/`push`/`count`/`at`/`findByPath`/`totalBytes`/`countByKind`）；
    - **落盘安全防线与内存态产物语义（参考 `/data/dev/bun/docs/bundler/index.mdx`）**：不指定 `outdir` 时构建产物以 `OutputValueKind.Buffer` 驻留内存并通过 `asSlice()` 零拷贝读取；指定 `outdir` 落盘时，所有构造方法及 `resolveFullPath(outdir, ...)` 均严格拦截含 `..` 段、NUL 字节或绝对路径逃逸的 `dest_path`（返回 `UnsafePathTraversal`），防止恶意包名或自定义 naming 模板向目标目录外写入文件。

```cp
import { BuildOutputFile, OutputFileKind, BuildOutputCollection } from "cap:bundler";

const f = BuildOutputFile.fromBuffer("assets/app.js", OutputFileKind.EntryPoint, "console.log(1);")?;
let full: [128]u8 = [0; 128];
const n = f.resolveFullPath("./dist/", full[0..])?; // "./dist/assets/app.js"
let col = BuildOutputCollection.init();
* = col.push(f)?;
* = n;
```

- **Bundler 懒导出规划器与 CSS Modules `composes` 展开图（`lazyExport.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/generateCodeForLazyExport.cp` & `/data/dev/bun/docs/bundler/loaders.mdx`）**：
    - 导出：`LAZY_EXPORT_MAX_PROPS` (`64`)、`LAZY_EXPORT_MAX_CSS_CLASSES` (`64`)、`LAZY_EXPORT_MAX_COMPOSES_EDGES` (`128`)、`LAZY_EXPORT_MAX_RESOLVED_TOKENS` (`32`)、`LazyExportError`、`isLazyExportJsIdentifier`、`isPromotableLazyNamedExport`、`formatLazyDefaultIdent`、`LazyExportKind`（`Esm`/`Cjs`/`CjsNapiAddon`）、`LazyExportPropDecision`、`LazyExportPropRecord`、`LazyExportPlanSummary`、`LazyExportPlanner`、`CssComposeTargetKind`、`CssModuleClassDef`、`CssComposeEdge`、`CssComposeGraph`；
    - **Built-in Loaders 与 CSS Modules 最佳实践（参考 `/data/dev/bun/docs/bundler/loaders.mdx`）**：
      - `.json` / `.jsonc` / `.toml` / `.yaml` 与 CSS Modules 在 ESM 模式下通过懒导出将合法 JS 标识符顶层属性拆分为独立 `Part`（`PromotedNamedExport`），使未引用的字段在链接期直接被 tree-shake 裁剪；`"default"`、`"__esModule"` 以及含连字符/保留字的键保留在 `<stem>_default` 默认导出对象中；
      - `.node`（N-API Addon）在非 CJS 输出格式（`output_is_cjs = false`）下必须注入运行时 `__require` 符号；
      - CSS Modules 的 `composes` 仅允许作用于单类选择器（复合选择器返回 `NonSingleClassCompose`），支持同文件、跨文件导入与 `from global` 原始类名，`CssComposeGraph` 通过 64 位访问掩码保障递归组合零死循环且无重复类名。

```cp
import {
    LazyExportKind,
    LazyExportPlanner,
    CssComposeGraph,
} from "cap:bundler";

let plan = LazyExportPlanner.init(LazyExportKind.Esm, false);
* = plan.addProperty("version")?;     // PromotedNamedExport (tree-shakeable)
* = plan.addProperty("my-key")?;      // DefaultOnlyNonIdent
* = plan.addProperty("default")?;     // DefaultOnlyReserved

let g = CssComposeGraph.init();
const base = g.registerClass(0, "base", "base_a1", true)?;
const btn = g.registerClass(1, "btn", "btn_b2", true)?;
* = g.addComposeGlobal(base, "u-reset")?;
* = g.addComposeRef(btn, base)?;
let cls_buf: [64]u8 = [0; 64];
const n = g.formatExpandedClassExpr(btn, cls_buf[0..])?; // "u-reset base_a1 btn_b2"
* = n;
```

- **Bundler JS Chunk 后处理与拼装引擎（`jsPostprocess.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/postProcessJSChunk.cp` & `/data/dev/bun/docs/bundler/bytecode.mdx`）**：
    - 导出：`JS_CHUNK_MAX_PARTS` (`64`)、`JsPostprocessError`、`JsChunkOutputFormat`（`Esm`/`Cjs`/`Iife`/`InternalBakeDev`）、`JsChunkWrapKind`（`None`/`Cjs`/`Esm`）、`JsChunkCommentStyle`（`Multiline`/`Single`）、`JsEntryTailDecision`（`None`/`ExportDefaultRequire`/`AwaitInit`/`SyncInit`/`ModuleExports`）、`shouldGenerateEsmBytecodeModuleInfo`、`validateBytecodeConfig`、`decideEntryPointTail`、`JsLineOffsetTracker`、`JsChunkPartInput`、`JsChunkAssemblyOptions`、`JsChunkAssemblyResult`、`assembleJsChunk`；
    - **Bytecode Caching 与 Chunk 拼装最佳实践（参考 `/data/dev/bun/docs/bundler/bytecode.mdx`）**：
      - `--bytecode` 预编译字节码缓存可为 CLI/构建工具带来 2x~4x 启动加速；其中 **ESM 字节码必须搭配 `--compile` 单文件可执行构建**（需要向可执行文件内嵌模块静态导入导出元数据 `ModuleInfo`），否则 `validateBytecodeConfig()` 返回 `BytecodeRequiresCompileForEsm`；CJS 字节码则兼容普通输出与 `--compile`；
      - `.jsc` 字节码与运行时版本强绑定，不可跨版本复用，建议在 CI/CD 构建期生成而非提交至 Git；
      - `assembleJsChunk()` 严格遵循 `Shebang -> Banner -> Format Wrapper Prefix -> Cross-Chunk Prefix -> Parts -> Entry Tail -> Cross-Chunk Suffix -> Format Wrapper Suffix -> Footer` 顺序，保证 Shebang 永远位于文件第 1 行并置位 `is_executable = true`，`InternalBakeDev` 自动跳过内嵌 runtime 段，并同步维护 `JsLineOffsetTracker` 供 SourceMap 偏移对齐。

```cp
import {
    JsChunkOutputFormat,
    JsChunkPartInput,
    JsChunkAssemblyOptions,
    validateBytecodeConfig,
    assembleJsChunk,
} from "cap:bundler";

* = validateBytecodeConfig(true, JsChunkOutputFormat.Esm, true)?; // OK with --compile
const parts: [2]JsChunkPartInput = [
    JsChunkPartInput{ label: "src/util.ts", code: "const x = 1;" },
    JsChunkPartInput{ label: "src/index.ts", code: "console.log(x);" },
];
let out: [512]u8 = [0; 512];
const res = assembleJsChunk(
    JsChunkAssemblyOptions{
        format: JsChunkOutputFormat.Iife,
        shebang: "#!/usr/bin/env bun",
        banner: "// built by cap",
        emit_part_comments: true,
    },
    parts[0..],
    out[0..],
)?;
// res.is_executable == true, res.parts_emitted == 2
* = res;
```

- **Bundler 链接期导入导出扫描与 `export *` 展开引擎（`exportScan.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/scanImportsAndExports.cp` & `/data/dev/bun/docs/runtime/module-resolution.mdx`）**：
    - 导出：`EXPORT_SCAN_MAX_MODULES` (`64`)、`EXPORT_SCAN_MAX_EXPORTS` (`128`)、`EXPORT_SCAN_MAX_RECORDS` (`128`)、`ExportScanError`、`ScanExportsKind`（`None`/`Esm`/`Cjs`/`EsmWithDynamicFallback`）、`ScanWrapKind`（`None`/`Cjs`/`Esm`）、`ScanImportKind`（`Stmt`/`Require`/`DynamicImport`/`RequireResolve`）、`ScanModuleNode`、`ScanImportEdge`、`ScanExportDecl`、`ExportScanner`（`init`/`addModule`/`addLocalExport`/`addImportEdge`/`resolveModuleWraps`/`resolveExportStars`/`lookupExport`/`moduleAt`）；
    - **ESM/CJS 互操作与 `export *` 语义契约（参考 `/data/dev/bun/docs/runtime/module-resolution.mdx`）**：
      - 当 `require()` 引用任意模块或 `import *` / `import def` 引用无导出语法的空模块时，链接器自动将目标标记为 `ScanWrapKind.Cjs` 以生成命名空间/兼容包装；`export *` 指向 CJS 模块时宿主升级为 `EsmWithDynamicFallback`；含顶层 `await` 的模块分配 `ScanWrapKind.Esm` 异步包装；
      - `resolveExportStars()` 严格遵守 ES 规范：**绝不转发 `"default"` 导出**、本地显式声明优先遮蔽重导出、菱形重导出同一源模块不冲突、多源同名导出自动置位 `ambiguous = true`，并内置 64 位访问掩码防止循环 `export *` 死循环。

```cp
import {
    ScanExportsKind,
    ScanImportKind,
    ScanModuleNode,
    ScanImportEdge,
    ExportScanner,
} from "cap:bundler";

let sc = ExportScanner.init();
const m_entry = sc.addModule(ScanModuleNode{ source_index: 0, exports_kind: ScanExportsKind.Esm, is_entry_point: true })?;
const m_lib = sc.addModule(ScanModuleNode{ source_index: 1, exports_kind: ScanExportsKind.Esm })?;
* = sc.addLocalExport(m_lib, "foo")?;
* = sc.addLocalExport(m_lib, "default")?; // 不会被 export * 转发
* = sc.addImportEdge(ScanImportEdge{ importer_idx: m_entry, target_idx: m_lib, kind: ScanImportKind.Stmt, is_export_star: true })?;
* = sc.resolveModuleWraps();
* = sc.resolveExportStars();
const hit = sc.lookupExport(m_entry, "foo");     // found (origin == m_lib)
const def = sc.lookupExport(m_entry, "default"); // null
* = hit;
* = def;
```

- **Bundler 磁盘落盘写入规划器（`writePlan.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/writeOutputFilesToDisk.cp` & `/data/dev/bun/docs/bundler/index.mdx`）**：
    - 导出：`WRITE_PLAN_MAX_FILES` (`64`)、`WRITE_PLAN_MAX_DIRS` (`64`)、`WritePlanError`、`validateOutputRelPath`、`extractParentDirPosix`、`WriteChunkContentKind`（`Js`/`Css`/`Html`/`Asset`）、`WriteSourceMapMode`（`None`/`Inline`/`Linked`/`External`）、`WriteFileRole`（`PrimaryChunk`/`SourceMap`/`Bytecode`/`Placeholder`）、`formatSourceMapUrlComment`、`WritePlannedFile`、`WritePlanStats`、`DiskWritePlanner`（`init`/`planChunk`/`dirAt`/`fileAt`/`stats`）；
    - **落盘安全防线与 Standalone HTML 对齐契约（参考 `/data/dev/bun/docs/bundler/index.mdx`）**：
      - 所有写入目标相对路径必须通过 `validateOutputRelPath()` 严格校验（拦截空路径、绝对路径、Windows 盘符、控制字符及 `..` 目录穿越段），并在 `DiskWritePlanner` 中执行全局输出路径碰撞去重（重复返回 `DuplicateOutputPath`）；
      - Standalone HTML 构建模式下（`standalone_html_only = true`），非 HTML Chunk 仅插入 `Placeholder` 保持输出数组索引对齐而不写盘；
      - 自动去重待创建父目录、按 `is_executable` 分配 `0o755`/`0o644` 权限，并按 `Linked`/`External` SourceMap 与 Bytecode 开关自动规划 `.map` / `.jsc` 伴生落盘文件及生成 JS/CSS `sourceMappingURL` 注释。

```cp
import {
    WriteChunkContentKind,
    WriteSourceMapMode,
    DiskWritePlanner,
} from "cap:bundler";

let wp = DiskWritePlanner.init(false);
* = wp.planChunk(
    0,
    WriteChunkContentKind.Js,
    "bin/cli.js",
    4096,
    true,
    WriteSourceMapMode.Linked,
    "bin/cli.js.map",
    true,
    "bin/cli.jsc",
)?;
const st = wp.stats(); // dirs_to_create == 1 ("bin"), files_to_write == 3, executable_count == 1
* = st;
```

- **CSS Chunk AST 预处理、条件包装与逆序去重引擎（`cssChunkPrep.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/prepareCssAstsForChunk.cp` & `/data/dev/bun/docs/bundler/css.mdx`）**：
    - 导出：`CSS_PREP_MAX_RULES` (`64`)、`CSS_PREP_MAX_CONDITIONS` (`8`)、`CSS_PREP_SCRATCH_CAP` (`1024`)、`CssChunkPrepError`、`CssPrepRuleKind`（`Import`/`Ignored`/`LayerStatement`/`StyleRule`/`AtRule`）、`CssPrepRule`、`CssPrepRuleList`、`CssPrepCondition`、`CssPrepPrefixStats`、`stripCssPrefixImportsPreservingLayers`、`wrapCssBodyWithConditions`、`formatNestedExternalImport`、`dedupCssChunkRulesReverse`；
    - **CSS 打包级联保序与边界最佳实践（参考 `/data/dev/bun/docs/bundler/css.mdx` & Bun Regression #28914）**：
      - **交错 `@layer` 语句保护**：内联 CSS 文件剥离头部 `@import` 时，必须无损保留与 `@import` 交错声明的 `@layer` 顺序声明（如 Tailwind v4 的 `@layer theme, base, components, utilities;`），且严禁原地修改共享源规则数组；
      - **空主体条件包装语义**：按 `layer` -> `supports` -> `media` 由内向外包装时，空匿名 `@layer {}` 及空 `@supports`/`@media` 应省略，但空**具名** `@layer foo {}` 必须降级为 `@layer foo;` 语句以保留声明层顺序的级联副作用；
      - **多层嵌套外部 `@import` 与逆序去重**：多层条件无法扁平合并为单条 `@import` 时，按 Bun 策略通过 `data:text/css,` 嵌套 `@import` 链完整保留条件与层序；跨文件重复规则必须按**从后往前（后声明胜出）**顺序串行去重以保证 CSS 级联语义不变。

```cp
import {
    CssPrepRule,
    CssPrepRuleList,
    CssPrepCondition,
    stripCssPrefixImportsPreservingLayers,
    wrapCssBodyWithConditions,
} from "cap:bundler";

const src: [3]CssPrepRule = [
    CssPrepRule{ kind: .LayerStatement, text: "@layer base, utilities;" },
    CssPrepRule{ kind: .Import, text: "@import \"./base.css\";" },
    CssPrepRule{ kind: .StyleRule, text: ".a{color:blue}" },
];
let kept = CssPrepRuleList.init();
const st = stripCssPrefixImportsPreservingLayers(src[0..], &kept)?; // kept.count == 2

const conds: [1]CssPrepCondition = [CssPrepCondition{ has_layer: true, layer_name: "base", media: "screen" }];
let out: [128]u8 = [0; 128];
const n = wrapCssBodyWithConditions("", conds[0..], out[0..])?; // "@media screen{@layer base;}"
* = st;
* = n;
```

- **Bundler Step 5 命名空间导出规划与 `__export(...)` 代码生成器（`nsExport.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/doStep5.cp` & `/data/dev/bun/docs/bundler/index.mdx`）**：
    - 导出：`NS_EXPORT_MAX_ENTRIES` (`64`)、`NS_EXPORT_MAX_AMBIG_REFS` (`8`)、`NS_EXPORT_MAX_BINDINGS` (`64`)、`NS_EXPORT_MAX_BIND_HOPS` (`16`)、`NsExportError`、`NsSymbolRef`、`NsImportBinding`、`NsResolvedExport`、`NsExportExprKind`（`DirectIdent`/`ImportIdent`）、`NsEmittedExport`、`NsExportEmitOptions`、`NsExportStats`、`NsExportPlanner`（`init`/`addBinding`/`addExport`/`addAmbiguousRef`/`build`/`emittedSlice`/`formatJsCode`）、`isSimpleJsIdent`；
    - **ESM/CJS 互操作与命名空间导出最佳实践（参考 `/data/dev/bun/docs/bundler/index.mdx`）**：
      - **菱形重导出 vs 歧义冲突**：多个 `export *` 导出同名符号时，先经 `imports_to_bind` 传递归约至最终符号；若归约到同一目标符号（菱形重导出）则正常保留，若指向不同目标符号则按 ECMAScript 规范静默剔除；
      - **TypeScript `type-only` 静默省略与确定性排序**：未解析的 TypeScript 类型重导出（`probably_ts_type == true`）自动忽略；全部保留导出别名按字典降序排序（对齐 Bun `strings.sortDesc`），保证跨平台构建产物字节级确定性；
      - **CJS 入口包装与属性名转义**：CommonJS 入口点自动追加 `module.exports = __toCommonJS(<exports>);`，非合法 JS 标识符别名（如 `"kebab-export"`）自动以双引号包裹。

```cp
import { NsExportPlanner, NsSymbolRef, NsExportEmitOptions } from "cap:bundler";

let p = NsExportPlanner.init();
* = p.addExport("alpha", NsSymbolRef{ source_index: 1, inner_index: 1 }, "alpha_fn", false)?;
* = p.addExport("zeta", NsSymbolRef{ source_index: 1, inner_index: 2 }, "zeta_val", false)?;
* = p.build();

let buf: [256]u8 = [0; 256];
const n = p.formatJsCode(NsExportEmitOptions{ exports_var_name: "exports_mod" }, buf[0..])?;
// "var exports_mod = {};\n__export(exports_mod, { zeta: () => zeta_val, alpha: () => alpha_fn });\n"
* = n;
```

- **Dev Server / HMR 语句转换器、紧凑导入编码器与 `import.meta.hot` 契约校验（`hmrStmts.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/convertStmtsForChunkForDevServer.cp` & `/data/dev/bun/docs/bundler/hot-reloading.mdx`）**：
    - 导出：`HMR_MAX_STMTS` (`64`)、`HMR_MAX_IMPORTS` (`32`)、`HMR_MAX_NAMED_ITEMS` (`16`)、`HmrStmtsError`、`HmrImportTag`（`UserModule`/`Builtin`/`Bun`/`Runtime`/`Css`）、`HmrImportRecord`、`HmrStmtKind`、`HmrStmtInput`、`HmrPrefixDeclKind`、`HmrPrefixDecl`、`HmrEsmBindingSlot`、`HmrConvertOptions`、`HmrConvertStats`、`HmrBuiltinEvent`、`parseHmrBuiltinEvent`、`hmrBuiltinEventName`、`isDirectImportMetaHotCall`、`HmrChunkStmtConverter`（`init`/`addImportRecord`/`addStmt`/`getImportRecord`/`outsidePrefixIndices`/`insidePrefixDecls`/`insideSuffixIndices`/`esmSlots`/`convert`/`formatPackedImports`/`formatInsideWrapperPrefix`/`formatInsideWrapperSuffix`）；
    - **Dev Server HMR 包装与热更新最佳实践（参考 `/data/dev/bun/docs/bundler/hot-reloading.mdx`）**：
      - **三段式语句分流与零 TDZ 绑定**：CommonJS 模块全部语句直通 `inside_wrapper_suffix`；ESM 模块自动禁用 `.css` loader 导入记录，将 barrel 优化未用子模块重写为 `var <ns> = {};`，将 `builtin`/`bun`/`runtime` 重写为 `hmr.builtin(...)` / `hmr.require("bun:wrap")`，将用户 ESM 依赖压缩为紧凑数组 `['mod', N, "a", ...]` 并生成 `var [...] = hmr.imports;` 与 `hmr.updateImport = [...];`（使用 `var` 消除 TDZ，裸导入保留数组空位与 `() => {}` 占位回调，`<= 2` 项单行输出）；
      - **`import.meta.hot` 生产死代码消除直调约束**：`import.meta.hot.<API>()` 必须完整直调（禁止 `const hot = import.meta.hot; hot.accept()` 或将 `dispose` 作为高阶回调传参），仅 `import.meta.hot.data` 允许作为值传递（生产构建会被内联为 `{}`，推荐 `import.meta.hot.data.x ??= init()` 以便压缩器消除）；
      - **内置事件与 Vite 兼容前缀**：支持全部 8 个 HMR 生命周期事件（`beforeUpdate`/`afterUpdate`/`beforeFullReload`/`beforePrune`/`invalidate`/`error`/`ws:disconnect`/`ws:connect`），同时兼容 `bun:*` 与 `vite:*` 前缀；`prune` 适用于模块被完全移除时释放长连接资源，而 `dispose` 每次热替换前均会触发。

```cp
import {
    HmrChunkStmtConverter,
    HmrImportRecord,
    HmrStmtInput,
    HmrConvertOptions,
    parseHmrBuiltinEvent,
    isDirectImportMetaHotCall,
} from "cap:bundler";

let conv = HmrChunkStmtConverter.init();
const r0 = conv.addImportRecord(HmrImportRecord{ resolved_path: "module_1", tag: .UserModule })?;
let s0 = HmrStmtInput{ kind: .Import, import_record_index: r0, namespace_ident: "module_1" };
* = s0.addNamedItem("add")?;
* = conv.addStmt(s0)?;
* = conv.addStmt(HmrStmtInput{ kind: .Body, body_code: "console.log(module_1.add(1, 2));" })?;

const st = conv.convert(HmrConvertOptions{})?;
let buf: [256]u8 = [0; 256];
const n = conv.formatPackedImports(buf[0..])?; // "['module_1', 1, \"add\"]"
const ok = isDirectImportMetaHotCall("import.meta.hot.accept()"); // true
const ev = parseHmrBuiltinEvent("vite:beforeUpdate"); // .BeforeUpdate
* = st;
* = n;
* = ok;
* = ev;
```

- **RSC 客户端引用代理与入口包装器代码生成器（`rscProxy.cp`，吸收自 `/data/dev/bun/src/bundler/ServerComponentParseTask.cp` & `/data/dev/bun/docs/bundler/fullstack.mdx`）**：
    - 导出：`RSC_PROXY_MAX_EXPORTS` (`64`)、`RscProxyError`、`RscTaskKind`（`ClientReferenceProxy`/`ClientEntryWrapper`）、`RscTargetPlatform`（`Server`/`Browser`）、`RscFrameworkConfig`、`RscProxyModuleMeta`、`rscTargetForTask`、`formatRscProdModuleKey`、`formatRscServerCallErrorMsg`、`formatClientEntryWrapper`、`RscProxyGenerator`（`init`/`addExport`/`exportSlice`/`generateProxyCode`）；
    - **RSC 与全栈 Dev/Prod 边界最佳实践（参考 `/data/dev/bun/docs/bundler/fullstack.mdx`）**：
      - **开发 vs 生产模块引用键**：开发模式下客户端引用代理直接使用源码相对路径 `pretty_path` 以支持零延迟 HMR；生产模式下由于最终 Chunk 路径尚未确定，统一输出 `<hex_unique_key>S<08d_source_index>` 占位键交由后续 Chunk 替换管线回填；
      - **跨端调用错误守卫与零副作用合成**：每个 `"use client"` 导出均包装为 `registerClientReference(() => { throw new Error(...); }, modulePath, exportName)`，严格阻止在服务端直接调用客户端函数，同时合成模块本身标记为无副作用纯数据以便树摇；
      - **浏览器入口裸导入包装**：`formatClientEntryWrapper()` 生成 `import "<entry_path>";` 裸导入（对应目标平台 `.Browser`），确保客户端入口副作用按序执行。

```cp
import { RscFrameworkConfig, RscProxyModuleMeta, RscProxyGenerator } from "cap:bundler";

let gen = RscProxyGenerator.init(
    RscFrameworkConfig{ is_dev_server: false, bundle_unique_key: 0x1a2b, server_runtime_import: "react-server-dom-webpack/server" },
    RscProxyModuleMeta{ pretty_path: "src/Counter.tsx", source_index: 42 },
);
* = gen.addExport("default")?;
* = gen.addExport("CounterButton")?;

let buf: [1024]u8 = [0; 1024];
const n = gen.generateProxyCode(buf[0..])?;
// 包含 `import { registerClientReference } from "react-server-dom-webpack/server";` 与 `"1a2bS00000042"` 占位键
* = n;
```

- **合成 JS AST 构建器与符号/作用域表（`astBuilder.cp`，吸收自 `/data/dev/bun/src/bundler/AstBuilder.cp` & `/data/dev/bun/docs/bundler/plugins.mdx`）**：
    - 导出：`AST_BUILDER_MAX_SYMBOLS` (`64`)、`AST_BUILDER_MAX_SCOPES` (`16`)、`AST_BUILDER_MAX_IMPORTS` (`32`)、`AST_BUILDER_MAX_IMPORT_ITEMS` (`16`)、`AST_BUILDER_MAX_STMTS` (`64`)、`AstBuilderError`、`AstScopeKind`、`AstSymbolKind`、`AstImportKind`、`AstStmtKind`、`AstSymRef`、`AstNamespaceAlias`、`AstSymbolEntry`、`AstImportRecord`、`AstSyntheticStmt`、`AstBuildSummary`、`SyntheticAstBuilder`（`init`/`pushScope`/`popScope`/`currentScopeKind`/`newSymbol`/`newExternalSymbol`/`getSymbol`/`addImportRecord`/`addImportStmt`/`addExportConstStmt`/`addExportDefaultStmt`/`finalize`/`formatJsPreview`）；
    - **虚拟模块与合成 AST 最佳实践（参考 `/data/dev/bun/docs/bundler/plugins.mdx`）**：
      - **免解析直接构造 AST**：插件 `onLoad` / RSC 边界模块无需拼接字符串再回送 JS Parser，直接通过 `SyntheticAstBuilder` 预分配 `module`/`hmr` 顶层符号、维护作用域栈并生成 2-Part 打包 AST（Part 0 空占位 + Part 1 主体，所有符号标记 `use_count = 1` 防树摇误删）；
      - **HMR `namespace_alias` 自动绑定**：当 `hot_reloading == true` 时，`addImportStmt()` 自动将导入符号关联至 `namespace_ref` 与 `import_record_index`，确保热更新回调可原地刷新导入绑定；
      - **外部全局符号防重命名**：通过 `newExternalSymbol("Error")` 标记 `must_not_be_renamed = true`，防止压缩器改写宿主全局标识符。

```cp
import { SyntheticAstBuilder } from "cap:bundler";

let ab = SyntheticAstBuilder.init(5, true);
const items: [1][]const u8 = ["registerClientReference"];
* = ab.addImportStmt("react-server-dom-webpack/server", "import_rsc", items[0..])?;
* = ab.addExportConstStmt("App", "registerClientReference(fn, \"App\")")?;
const summary = ab.finalize()?; // part_count == 2, hmr_aliased_import_count == 1
* = summary;
```

- **生产 Chunk 语句降级与 4 区域落位引擎（`chunkStmts.cp`，吸收自 `/data/dev/bun/src/bundler/linker_context/convertStmtsForChunk.cp` & `/data/dev/bun/docs/bundler/index.mdx`）**：
    - 导出：`CHUNK_STMTS_MAX_INPUT` (`64`)、`CHUNK_STMTS_MAX_IMPORTS` (`32`)、`CHUNK_STMTS_MAX_REGION` (`32`)、`ChunkStmtsError`、`ChunkWrapKind`（`None`/`Esm`/`Cjs`）、`ChunkOutputFormat`（`Esm`/`Cjs`/`Iife`）、`ChunkStmtRegion`（`OutsideWrapperPrefix`/`InsideWrapperPrefix`/`InsideWrapperSuffix`/`OutsideWrapperSuffix`）、`ChunkStmtInputKind`、`ChunkEmittedStmtKind`、`ChunkImportRecordMeta`、`ChunkStmtInput`、`ChunkEmittedStmt`、`ChunkConvertOptions`、`ChunkConvertStats`、`ChunkStmtConverter`（`init`/`addImportRecord`/`addStmt`/`convert`/`regionSlice`/`formatPreview`）；
    - **打包格式（`esm` / `cjs` / `iife`）与包装闭包最佳实践（参考 `/data/dev/bun/docs/bundler/index.mdx`）**：
      - **顶层 ESM 导入提升**：当模块被包装在 `__esm(() => { ... })` 或 `__commonJS` 闭包内时，所有保留的外部 `import` / `export * as` / `export { ... } from` 必须提升至 `OutsideWrapperPrefix` 顶层，避免非法嵌套 ES 模块语法；
      - **CJS 入口双写 `module.exports`**：当 `format == .Cjs && is_entry_point == true` 时，`__reExport(exports, target, module.exports)` 自动附加 `module.exports` 第 3 参数以同时满足外部 CJS 消费方（带 `__esModule`）与内部 ESM 自引用；
      - **Barrel 优化与 `export default` 降级**：标记 `is_unused` 的 `export *` 直接裁剪；打包模式下自动剥离函数/类/变量的 `export` 修饰符并将 `export default` 降级为命名本地绑定或函数/类声明。

```cp
import {
    ChunkStmtConverter,
    ChunkImportRecordMeta,
    ChunkStmtInput,
    ChunkConvertOptions,
    ChunkWrapKind,
    ChunkOutputFormat,
} from "cap:bundler";

let conv = ChunkStmtConverter.init();
const ext_idx = conv.addImportRecord(ChunkImportRecordMeta{
    specifier: "node:fs",
    namespace_name: "import_fs",
    is_internal: false,
})?;
* = conv.addStmt(ChunkStmtInput{ kind: .Import, import_record_idx: ext_idx })?;
* = conv.addStmt(ChunkStmtInput{ kind: .ExportDefaultExpr, default_name: "mod_default", code: "42" })?;

const st = conv.convert(ChunkConvertOptions{ wrap: ChunkWrapKind.Esm, format: ChunkOutputFormat.Esm })?;
let buf: [512]u8 = [0; 512];
const n = conv.formatPreview(buf[0..], "init_app")?;
* = st;
* = n;
```

- **HMR DevServer 二进制错误覆盖层编解码器（`hmrFailure.cp`，吸收自 `/data/dev/bun/src/bake/DevServer/SerializedFailure.cp` & `/data/dev/bun/docs/bundler/hot-reloading.mdx`）**：
    - 导出：`HMR_FAILURE_MAX_MSGS` (`16`)、`HMR_FAILURE_MAX_NOTES` (`8`)、`HMR_FAILURE_MAX_OWNER_INDEX` (`0x3FFF_FFFF`)、`HmrFailureError`、`HmrFailureOwnerKind`（`None`/`Route`/`Client`/`Server`）、`HmrFailureOwner`（32-bit 紧凑位打包 `pack`/`unpack`）、`HmrFailureErrorKind`（12 变体：5 种打包日志级别 + 7 种 JS 异常类型）、`HmrSourceLoc`、`HmrFailureData`、`HmrFailureMsg`、`HmrFailureFrame`、`HmrFailureCodec`（`peekOwner`/`encode`/`decode`）；
    - **HMR 错误隔离与热更新边界最佳实践（参考 `/data/dev/bun/docs/bundler/hot-reloading.mdx`）**：
      - **按增量图归属句柄去重**：每个 `HmrFailureOwner`（30-bit 文件/路由索引 + 2-bit 归属类别）在 DevServer 错误表中仅维护唯一一条 `HmrFailureFrame`（可聚合多条消息与附注），通过 O(1) `peekOwner()` 直接路由更新或清除浏览器错误覆盖层；
      - **`import.meta.hot` 直接调用约束**：`import.meta.hot.accept()` / `dispose()` / `on()` 必须直接静态书写，严禁赋值给中间变量，以确保生产构建可完全死代码消除；若文件未建立热更新边界则自动回退整页刷新。

```cp
import {
    HmrFailureOwner,
    HmrFailureFrame,
    HmrFailureMsg,
    HmrFailureData,
    HmrSourceLoc,
    HmrFailureCodec,
} from "cap:bundler";

const owner = HmrFailureOwner.init(.Client, 7)?;
let frame = HmrFailureFrame.init(owner, "src/App.tsx");
* = frame.addMsg(HmrFailureMsg{
    kind: .BundlerLogErr,
    data: HmrFailureData{
        text: "Unexpected token",
        location: HmrSourceLoc{ line: 12, column: 4, length: 3, line_text: "const = 1;" },
    },
})?;

let wire: [512]u8 = [0; 512];
const n = HmrFailureCodec.encode(frame, wire[0..])?;
const decoded = HmrFailureCodec.decode(wire[0..n])?;
* = decoded;
```

- **内容寻址引用计数静态资产存储（`assetStore.cp`，吸收自 `/data/dev/bun/src/bake/DevServer/Assets.cp` & `/data/dev/bun/docs/bundler/fullstack.mdx`）**：
    - 导出：`ASSET_STORE_MAX_PATHS` (`128`)、`ASSET_STORE_MAX_FILES` (`128`)、`ASSET_URL_PREFIX` (`"/_bun/asset/"`)、`AssetStoreError`、`AssetEntry`、`AssetPathMapping`、`AssetReplaceOutcome`、`formatAssetUrl`、`AssetStore`（`init`/`pathCount`/`uniqueAssetCount`/`totalMemoryBytes`/`getHash`/`getByHash`/`getRefCount`/`replacePath`/`putOrIncrementRefCount`/`unrefByHash`/`removePath`）；
    - **Bun 全栈 DevServer 内容寻址资产最佳实践（参考 `/data/dev/bun/docs/bundler/fullstack.mdx`）**：
      - HTML 路由中引用的静态资源由全栈开发服务器自动打包并挂载至 `/_bun/asset/<16-hex>.<ext>`；不同路径若内容哈希相同，自动通过 `AssetStore` 共享唯一底层条目并递增引用计数；
      - 开发期文件覆写时，`replacePath()` 在 `refs == 1` 且新哈希不冲突时执行原地替换并立即吊销旧哈希，彻底避免浏览器缓存陈旧资源；引用计数归零时以 O(1) `swapRemove` 紧凑回收并自动修复所有受影响的 `path -> entry_index` 映射。

```cp
import { AssetStore, formatAssetUrl } from "cap:bundler";

let store = AssetStore.init();
const r1 = store.replacePath("/app/logo.png", 0x0123456789abcdef, 4096, "image/png")?;
const r2 = store.replacePath("/app/favicon.png", 0x0123456789abcdef, 4096, "image/png")?;
// r2.shared_existing == true, store.uniqueAssetCount() == 1, store.getRefCount(...) == 2
let url_buf: [64]u8 = [0; 64];
const url = formatAssetUrl(0x0123456789abcdef, ".png", url_buf[0..])?; // "/_bun/asset/0123456789abcdef.png"
* = r1; * = r2; * = url;
```
