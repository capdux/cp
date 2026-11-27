[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# proxy-from-env

Zero-allocation HTTP_PROXY / HTTPS_PROXY / ALL_PROXY / NO_PROXY URL routing and wildcard bypass matcher (clean-room MIT implementation of npm proxy-from-env, #303 / 169M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:proxy-from-env` ([https://github.com/Rob--W/proxy-from-env](https://github.com/Rob--W/proxy-from-env)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `proxy-from-env` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add proxy-from-env
```

```cp
import {
    ProxyEnv,
    PfeParsedUrl,
    pfeEndsWithIgnoreCase,
    pfeParseUrl,
    shouldBypassProxy,
    getProxyForUrl,
} from "proxy-from-env";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ProxyEnv` | `class` | 字段: `http_proxy: string, https_proxy: string, all_proxy: string, no_proxy: string` · 方法: `empty` |
| `PfeParsedUrl` | `class` | 字段: `scheme: string, host: string, port: u16` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pfeEndsWithIgnoreCase` | `(s: string, suffix: string): bool` | 调用 `pfeEndsWithIgnoreCase` 并返回 `bool`。 |
| `pfeParseUrl` | `(url: string): PfeParsedUrl` | 调用 `pfeParseUrl` 并返回 `PfeParsedUrl`。 |
| `shouldBypassProxy` | `(host: string, port: u16, no_proxy: string): bool` | 调用 `shouldBypassProxy` 并返回 `bool`。 |
| `getProxyForUrl` | `(target_url: string, env: ProxyEnv): string` | 查询或读取目标数据，返回 `string`。 |

## 最小调用示例

```cp
import { getProxyForUrl } from "proxy-from-env";

pub fn main() {
    let result = getProxyForUrl("example", /* env: ProxyEnv */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
