# `cap:hid` — USB HID 1.11 Report Descriptor 零分配流式解析库

纯 CP、零堆分配的 USB HID 1.11 Report Descriptor 解析与报告布局摘要库，支持 Short Item（0/1/2/4B 大小码与符号/零扩展数值读取）、Long Item (`0xFE`) 流式迭代，以及单遍 `Push/Pop` 与 `Collection` 深度平衡验证。

## 导入

```cp
import {
    ITEM_TYPE_MAIN,
    ITEM_TYPE_GLOBAL,
    ITEM_TYPE_LOCAL,
    ITEM_LONG_PREFIX,
    MAIN_INPUT,
    MAIN_OUTPUT,
    MAIN_COLLECTION,
    MAIN_FEATURE,
    MAIN_END_COLLECTION,
    GLOBAL_USAGE_PAGE,
    GLOBAL_LOGICAL_MIN,
    GLOBAL_LOGICAL_MAX,
    GLOBAL_PHYSICAL_MIN,
    GLOBAL_PHYSICAL_MAX,
    GLOBAL_UNIT_EXPONENT,
    GLOBAL_UNIT,
    GLOBAL_REPORT_SIZE,
    GLOBAL_REPORT_ID,
    GLOBAL_REPORT_COUNT,
    GLOBAL_PUSH,
    GLOBAL_POP,
    LOCAL_USAGE,
    LOCAL_USAGE_MIN,
    LOCAL_USAGE_MAX,
    COLL_PHYSICAL,
    COLL_APPLICATION,
    COLL_LOGICAL,
    USAGE_PAGE_GENERIC_DESKTOP,
    USAGE_PAGE_KEYBOARD,
    USAGE_PAGE_LED,
    USAGE_PAGE_BUTTON,
    USAGE_PAGE_CONSUMER,
    FLAG_CONSTANT,
    FLAG_VARIABLE,
    FLAG_RELATIVE,
    HidItem,
    HidItemIter,
    HidReportSummary,
    summarizeHidDescriptor,
} from "cap:hid";
```

## 核心 API

- **流式项迭代器**：`HidItemIter.init(desc)`（`next(*_): HidItem?`、`isMalformed(): bool`）。
- **项数据解码**：`HidItem` 提供 `u32Value(): u32`、`i32Value(): i32`（小端符号扩展）、`isMain()`、`isGlobal()`、`isLocal()`。
- **单遍报告布局摘要**：`summarizeHidDescriptor(desc): HidReportSummary?`，返回 `input_bits`, `output_bits`, `feature_bits`, `collection_count`, `max_depth`, `has_report_ids`, `usage_page`，并在检测到截断、保留类型或栈/集合失衡时返回 `null`。

## 示例

```cp
import { summarizeHidDescriptor } from "cap:hid";

fn example(desc: []const u8) {
    if (s := summarizeHidDescriptor(desc)) {
        * = s.input_bits;
        * = s.output_bits;
        * = s.max_depth;
    }
}
```
