// Source: cp/doc/lib/mermaid.md
# `cap:mermaid` — 声明式 Mermaid 图表构造与语法安全转义引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

纯 CP、零 C 依赖的工业级 Mermaid 图表构造引擎，专为 AI Agent 架构图生成、技术文档（Artifact）自动可视化与工作流展示设计。支持 `Flowchart`、`SequenceDiagram`、`ErDiagram` 与 `StateDiagram-v2`，并自动对含括号、引号、换行及 HTML 字符的标签进行安全引号转义。

## 导入方式

```cp
import {
    MermaidError,
    FlowDirection,
    NodeShape,
    EdgeStyle,
    MsgArrow,
    ErCardinality,
    isValidIdentifier,
    FlowchartBuilder,
    SequenceBuilder,
    ErBuilder,
    StateBuilder,
} from "cap:text";

// 或动态命名空间挂载：
import cap from "cap";
// cap.mermaid.FlowchartBuilder / cap.mermaid.SequenceBuilder / cap.mermaid.ErBuilder / cap.mermaid.StateBuilder
```

## 核心错误与枚举

### `MermaidError`
- `InvalidIdentifier`：节点/参与者/实体 ID 为空或包含非法字符（仅允许 `[A-Za-z0-9_-]`；`StateBuilder` 转换额外允许 `"[*]"`）
- `DuplicateId`：重复声明相同的节点、参与者或实体 ID
- `EmptyDiagram`：尝试渲染空图表
- `UnknownNode`：严格模式下引用未注册节点，或 `ErBuilder.addAttribute` 引用不存在的实体
- `UnbalancedBlock`：未闭合或多余的 `subgraph` / `loop` / `alt` / 复合状态块
- `OutOfMemory`：内存分配失败

### 枚举类型
- `FlowDirection`：`TD`、`LR`、`BT`、`RL`
- `NodeShape`：`Rect` (`[...]`)、`Rounded` (`(...)`)、`Diamond` (`{...}`)、`Cylinder` (`[(...)]`)、`Circle` (`((...))`)、`Hexagon` (`{{...}}`)
- `EdgeStyle`：`SolidArrow` (`-->`)、`DottedArrow` (`-.->`)、`ThickArrow` (`==>`)、`SolidLine` (`---`)
- `MsgArrow`：`Sync` (`->>`)、`Async` (`-->>`)、`Reply` (`-->`)、`Solid` (`->`)
- `ErCardinality`：`OneToOne`、`OneToMany`、`ZeroOrOne`、`ZeroOrMany`

## 构建器 API

### `FlowchartBuilder`
- `FlowchartBuilder.init(alloc: std.mem.Allocator, direction: FlowDirection): FlowchartBuilder`
- `deinit(*_)`：释放所有内部字符串与列表内存
- `setStrict(*_, strict: bool)`：开启/关闭 `addEdge` 节点存在性校验（默认 `false`）
- `addNode(*_, id: string, label: string, shape: NodeShape): void ! MermaidError`
  - 自动检测 `()[]{}"'<>&;#|:/\` 及换行符，安全包裹双引号并将 `"` 转为 `#quot;`、`<`/`>` 转为 `&lt;`/`&gt;`、`\n` 转为 `<br/>`
- `addEdge(*_, from_id: string, to_id: string, label: string, style: EdgeStyle): void ! MermaidError`
- `beginSubgraph(*_, id: string, title: string): void ! MermaidError` / `endSubgraph(*_): void ! MermaidError`
- `render(alloc): ([]u8) ! MermaidError` / `renderMarkdownBlock(alloc): ([]u8) ! MermaidError`

### `SequenceBuilder`
- `SequenceBuilder.init(alloc: std.mem.Allocator): SequenceBuilder` / `deinit(*_)`
- `addParticipant(*_, id: string, alias: string): void ! MermaidError`
- `addActor(*_, id: string, alias: string): void ! MermaidError`
- `message(*_, from_id: string, to_id: string, text: string, arrow: MsgArrow): void ! MermaidError`
- `noteOver(*_, participant: string, text: string): void ! MermaidError`
- `activate(*_, id: string): void ! MermaidError` / `deactivate(*_, id: string): void ! MermaidError`
- `beginLoop(*_, label: string): void ! MermaidError`
- `beginAlt(*_, cond: string): void ! MermaidError` / `elseBranch(*_, cond: string): void ! MermaidError`
- `endBlock(*_): void ! MermaidError`
- `render(alloc): ([]u8) ! MermaidError` / `renderMarkdownBlock(alloc): ([]u8) ! MermaidError`

### `ErBuilder`
- `ErBuilder.init(alloc: std.mem.Allocator): ErBuilder` / `deinit(*_)`
- `addEntity(*_, name: string): void ! MermaidError`
- `addAttribute(*_, entity: string, type_name: string, attr_name: string, key_tag: string): void ! MermaidError`
  - `key_tag` 支持 `"PK"`、`"FK"`、`"UK"` 或 `""`
- `addRelation(*_, left: string, card_left: ErCardinality, right: string, card_right: ErCardinality, label: string): void ! MermaidError`
- `render(alloc): ([]u8) ! MermaidError` / `renderMarkdownBlock(alloc): ([]u8) ! MermaidError`

### `StateBuilder`
- `StateBuilder.init(alloc: std.mem.Allocator): StateBuilder` / `deinit(*_)`
- `addState(*_, id: string, description: string): void ! MermaidError`
- `addTransition(*_, from_id: string, to_id: string, label: string): void ! MermaidError`（支持 `"[*]"` 起止伪状态）
- `beginComposite(*_, id: string, title: string): void ! MermaidError` / `endComposite(*_): void ! MermaidError`
- `noteRightOf(*_, state_id: string, text: string): void ! MermaidError`
- `render(alloc): ([]u8) ! MermaidError` / `renderMarkdownBlock(alloc): ([]u8) ! MermaidError`
