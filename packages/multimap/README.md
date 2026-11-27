[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/multimap

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:multimap` ([https://github.com/havarnov/multimap](https://github.com/havarnov/multimap))

零堆分配的 CP 一键多值映射表（保持插入顺序，容量 128） — 洁净室 MIT 实现，对标 Rust crates.io #390 `multimap`（3.81 亿+ 总下载），适用于 HTTP 多值 Header、URL 多值 Query、标签倒排索引与事件路由。

Zero-allocation insertion-ordered one-to-many string multimap for CP — clean-room MIT parity with Rust `multimap` (381M+ downloads).

## 安装 / Install

```bash
cap add @cap/multimap
```

## API 参考 / API Reference

- **`MultiEntry`**：`{ key: string, value: string }`
- **`MultiMapStr`**（容量 128）：
  - `init()`, `totalValues()`, `isEmpty()`, `isFull()`, `clear(*_)`, `entryAt(idx)`
  - `insert(*_, key, value) -> bool`
  - `getFirst(key)`, `getLast(key)`, `getNth(key, n)`
  - `countFor(key)`, `containsKey(key)`, `uniqueKeyCount()`
  - `removeKey(*_, key) -> usize`, `removePair(*_, key, value) -> bool`

## 使用示例 / Usage

```cp
import { MultiMapStr } from "@cap/multimap";

let hdrs = MultiMapStr.init();
* = hdrs.insert("Set-Cookie", "sid=abc; HttpOnly");
* = hdrs.insert("Set-Cookie", "theme=dark");
* = hdrs.insert("Content-Type", "text/html");

const n = hdrs.countFor("Set-Cookie");     // 2
const second = hdrs.getNth("Set-Cookie", 1 as usize).?; // "theme=dark"
```

## License

MIT
