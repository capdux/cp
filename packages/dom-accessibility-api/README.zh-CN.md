[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# dom-accessibility-api

适用于 CP 语言的零堆分配 W3C 无障碍名称与描述计算库（AccName 1.2），对齐 npm [`dom-accessibility-api`](https://github.com/eps1lon/dom-accessibility-api) v0.6.3（MIT 协议，CP v0.6.3）。

严格遵循 W3C AccName 优先级（`aria-labelledby` → `aria-label` → 宿主语言标签 / `alt` / `value` / `legend` / `caption` / `figcaption` → 元素内容 → `placeholder` → `title`），支持空白折叠、展示角色（presentation）与隐藏元素判定，全程零堆分配。

## 安装与导入

```bash
cap add dom-accessibility-api
```

```cp
import {
    A11yNodeInput,
    A11yNodeEx,
    ComputeNameOptions,
    NameResult,
    defaultComputeOptions,
    defaultA11yNodeEx,
    computeAccessibleName,
    computeAccessibleNameWithOptions,
    computeAccessibleDescription,
    computeAccessibleDescriptionForNode,
} from "dom-accessibility-api";
```

## 核心 API 参考

### 名称来源常量（`NameResult.source`）

| 常量 | 值 | 含义 |
| :--- | :--- | :--- |
| `NAME_SOURCE_NONE` | `0` | 未解析出无障碍名称。 |
| `NAME_SOURCE_ARIA_LABELLEDBY` | `1` | 来自 `labelled_by_text`。 |
| `NAME_SOURCE_ARIA_LABEL` | `2` | 来自 `aria_label`。 |
| `NAME_SOURCE_ALT` | `3` | 来自 `alt`（`img` / `area` / `input`）。 |
| `NAME_SOURCE_HOST_LABEL` | `4` | 来自 `<label>` / `<legend>` / `<caption>` / `<figcaption>`。 |
| `NAME_SOURCE_VALUE` | `5` | 来自按钮类 `input` 的 `value`。 |
| `NAME_SOURCE_CONTENT` | `6` | 来自元素文本内容 `inner_text`（满足 `allowsNameFromContent`）。 |
| `NAME_SOURCE_PLACEHOLDER` | `7` | 回退自 `placeholder`。 |
| `NAME_SOURCE_TITLE` | `8` | 回退自 `title`。 |

### 数据结构

| 类型 | 关键字段 | 说明 |
| :--- | :--- | :--- |
| `A11yNodeInput` | `tag`, `aria_label`, `labelled_by_text`, `inner_text`, `alt`, `title`, `placeholder`, `aria_hidden` | 精简节点输入，供 `computeAccessibleName` 快速调用。 |
| `A11yNodeEx` | 包含 `A11yNodeInput` 全部字段及 `role`, `input_type`, `described_by_text`, `aria_description`, `value`, `label_text`, `legend_text`, `caption_text`, `figcaption_text`, `is_hidden` | 完整 W3C AccName 1.2 节点描述符。 |
| `ComputeNameOptions` | `include_hidden`, `compute_from_content`, `fallback_to_placeholder`, `fallback_to_title` | 精细化 AccName 计算选项。 |
| `NameResult` | `len: usize`, `source: u8`, `used_title: bool`, `used_placeholder: bool` | 写入字节数及来源标记，用于描述计算阶段防重。 |

### 函数列表

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `defaultComputeOptions` | `(): ComputeNameOptions` | 返回标准默认选项（忽略隐藏元素，开启内容与属性回退）。 |
| `defaultA11yNodeEx` | `(tag: string): A11yNodeEx` | 创建预填 `tag` 的默认 `A11yNodeEx`。 |
| `computeAccessibleName` | `(node: A11yNodeInput, out: []u8): usize` | 计算折叠空白后的无障碍名称写入 `out`，返回写入字节数。 |
| `computeAccessibleNameWithOptions` | `(node: A11yNodeEx, opts: ComputeNameOptions, out: []u8): NameResult` | 完整 AccName 1.2 计算，返回长度、来源码及回退标志。 |
| `computeAccessibleDescription` | `(aria_desc: string, title: string, used_title_as_name: bool, out: []u8): usize` | 计算无障碍描述，若 `title` 已被用作名称则跳过。 |
| `computeAccessibleDescriptionForNode` | `(node: A11yNodeEx, opts: ComputeNameOptions, used_title_as_name: bool, used_placeholder_as_name: bool, out: []u8): usize` | 完整节点无障碍描述计算（`described_by_text` → `aria_description` → `title` / `placeholder`）。 |
| `collapseWhitespace` | `(input: string, out: []u8): usize` | 去除首尾空白并将内部连续空白折叠为单个空格。 |
| `isPresentationRole` / `isHiddenElement` / `allowsNameFromContent` | `(...): bool` | ARIA 角色、隐藏态及内容取名资格辅助判断。 |
| `a11yUsesAlt` / `a11yIsInputButtonType` / `a11yIsFormControl` | `(string): bool` | HTML 宿主语言元素分类辅助函数。 |

## 代码示例

### 1. 完整无障碍名称与描述计算

```cp
import {
    defaultA11yNodeEx,
    defaultComputeOptions,
    computeAccessibleNameWithOptions,
    computeAccessibleDescriptionForNode,
} from "dom-accessibility-api";

fn inspectInput() {
    let mut node = defaultA11yNodeEx("input");
    node.input_type = "text";
    node.label_text = "  Email   Address ";
    node.title = "We will never share your email";

    let opts = defaultComputeOptions();
    let name_buf: [128]u8 = undefined;
    let res = computeAccessibleNameWithOptions(node, opts, name_buf[0..128]);
    // name_buf[0..res.len] == "Email Address", res.used_title == false

    let desc_buf: [128]u8 = undefined;
    let dlen = computeAccessibleDescriptionForNode(
        node, opts, res.used_title, res.used_placeholder, desc_buf[0..128],
    );
    // desc_buf[0..dlen] == "We will never share your email"
}
```

### 2. 轻量快速名称计算（`A11yNodeInput`）

```cp
import { A11yNodeInput, computeAccessibleName } from "dom-accessibility-api";

fn quickButtonName(): usize {
    let btn = A11yNodeInput {
        tag: "button",
        aria_label: "",
        labelled_by_text: "",
        inner_text: "  Save \n Changes  ",
        alt: "",
        title: "Save",
        placeholder: "",
        aria_hidden: false,
    };
    let out: [64]u8 = undefined;
    return computeAccessibleName(btn, out[0..64]); // 写入 "Save Changes"（12 字节）
}
```

## 与源包差异与零拷贝安全亮点

- **DOM 无关值结构体**：不依赖浏览器活体 DOM 树，调用方填充栈上 `A11yNodeInput` / `A11yNodeEx` 即可运行，完美适配 SSR、静态分析器与测试框架。
- **零堆分配输出**：空白折叠与名称/描述拼接均直接写入调用方提供的 `[]u8` 缓冲区。
- **来源显式追踪**：`NameResult` 返回 `source`、`used_title` 与 `used_placeholder`，确保描述计算严格遵循规范不重复消费回退属性。
