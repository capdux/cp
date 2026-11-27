[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fast-socks5

Zero-allocation RFC 1928 SOCKS5 and RFC 1929 username/password authentication frame encoder and decoder for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fast-socks5` ([https://github.com/dizda/fast-socks5](https://github.com/dizda/fast-socks5)) / `npm:socks` ([https://github.com/JoshGlazebrook/socks](https://github.com/JoshGlazebrook/socks))（原库名 `fast-socks5` → CP 包名 `fast-socks5`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `fast-socks5` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fast-socks5
```

```cp
import {
    Socks5Request,
    encodeGreeting,
    selectMethod,
    encodeUserPassAuth,
    encodeConnectDomain,
    encodeConnectIpv4,
    decodeRequest,
    encodeReplyIpv4,
} from "fast-socks5";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Socks5Request` | `class` | 字段: `cmd: u8, atyp: u8, ipv4: [4]u8, ipv6: [16]u8, domain: string, port: u16, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `encodeGreeting` | `(methods: &[]u8, out: []u8): usize` | 通过 `encodeGreeting` 编码或格式化输出，返回 `usize`。 |
| `selectMethod` | `(greeting: &[]u8, supported: &[]u8, out_method: &mut u8): i32` | 调用 `selectMethod` 并返回 `i32`。 |
| `encodeUserPassAuth` | `(user: string, pass: string, out: []u8): usize` | 通过 `encodeUserPassAuth` 编码或格式化输出，返回 `usize`。 |
| `encodeConnectDomain` | `(host: string, port: u16, out: []u8): usize` | 通过 `encodeConnectDomain` 编码或格式化输出，返回 `usize`。 |
| `encodeConnectIpv4` | `(ip: [4]u8, port: u16, out: []u8): usize` | 通过 `encodeConnectIpv4` 编码或格式化输出，返回 `usize`。 |
| `decodeRequest` | `(data: &[]u8, out: &mut Socks5Request): i32` | 通过 `decodeRequest` 解析输入数据，返回 `i32`。 |
| `encodeReplyIpv4` | `(rep: u8, bind_ip: [4]u8, bind_port: u16, out: []u8): usize` | 通过 `encodeReplyIpv4` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { encodeUserPassAuth } from "fast-socks5";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeUserPassAuth("example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
