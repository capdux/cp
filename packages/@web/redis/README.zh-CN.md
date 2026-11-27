[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/redis

Official thin Redis lifecycle, distributed namespace key builder, and zero-heap RESP2/3 command encoder plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/redis` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/redis
```

```cp
import {
    RedisPluginConfig,
    rdsDefaultPing,
    rdsIsSafeKeySegment,
    rdsWriteByte,
    rdsWriteRaw,
    rdsWriteUsize,
    redisBuildKey,
    redisEncodeRespCommand,
} from "@web/redis";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RedisPluginConfig` | `class` | 字段: `host: string, port: u16, key_prefix: string, ping_path: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rdsDefaultPing` | `(cfg: &RedisPluginConfig): bool` | 调用 `rdsDefaultPing` 并返回 `bool`。 |
| `rdsIsSafeKeySegment` | `(s: string): bool` | 调用 `rdsIsSafeKeySegment` 并返回 `bool`。 |
| `rdsWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `rdsWriteByte` 并返回 `bool`。 |
| `rdsWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `rdsWriteRaw` 并返回 `bool`。 |
| `rdsWriteUsize` | `(out_buf: []u8, pos: &mut usize, val: usize): bool` | 调用 `rdsWriteUsize` 并返回 `bool`。 |
| `redisBuildKey` | `(prefix: string, namespace: string, key: string, out_buf: []u8): usize?` | 调用 `redisBuildKey` 并返回 `usize?`。 |
| `redisEncodeRespCommand` | `(args: &[]string, out_buf: []u8): usize?` | 调用 `redisEncodeRespCommand` 并返回 `usize?`。 |
| `rdsPingHandler` | `(ctx: &mut Context): Response` | 调用 `rdsPingHandler` 并返回 `Response`。 |
| `createRedisPlugin` | `(cfg: RedisPluginConfig, ping_fn: RedisPingFn): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerRedisWithPing` | `(app: &mut App, cfg: RedisPluginConfig, ping_fn: RedisPingFn): &mut App` | 将插件或中间件注册到目标应用实例。 |
| `registerRedis` | `(app: &mut App, cfg: RedisPluginConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { rdsIsSafeKeySegment } from "@web/redis";

pub fn main() {
    let result = rdsIsSafeKeySegment("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
