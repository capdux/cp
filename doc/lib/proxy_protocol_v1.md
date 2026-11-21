# cap:proxy_protocol_v1

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（HAProxy PROXY Protocol v1 规范），提供纯 CP、零外部依赖、零堆分配的 PROXY v1 文本头解析与格式化库，用于负载均衡器与反向代理透传真实客户端地址。

## 导入方式

```cp
import { ProxyProtoFamily, ProxyV1Header } from "cap:proxy_protocol_v1";
```

## 核心类型与 API

- `ProxyProtoFamily`：枚举 `tcp4`、`tcp6`、`unknown`。
- `ProxyV1Header`：
  - `family: ProxyProtoFamily`
  - `src_addr: []const u8`
  - `dst_addr: []const u8`
  - `src_port: u16`
  - `dst_port: u16`
  - `header_len: usize`（包含末尾 `\r\n` 的总字节数，便于切片分离后续 HTTP/TCP 负载）
- `ProxyV1Header.parse(buf: []const u8): ProxyV1Header?`：
  - 严格校验 HAProxy v1 协议约束：最大 107 字节、`"PROXY "` 精确前缀、`\r\n` 结尾、字段间单空格分隔；
  - 校验 `TCP4` IPv4 与 `TCP6` IPv6 地址格式及 `0..=65535` 无前导零端口；
  - `UNKNOWN` 允许携带任意附加信息直至 `\r\n`。
- `ProxyV1Header.format(out: []u8): ([]const u8)?`：规范化输出 `PROXY TCP4 ... \r\n` / `PROXY TCP6 ... \r\n` / `PROXY UNKNOWN\r\n`。

## 使用示例

```cp
import { ProxyProtoFamily, ProxyV1Header } from "cap:proxy_protocol_v1";

fn example(packet: []const u8) {
    if (hdr := ProxyV1Header.parse(packet)) {
        const payload = packet[hdr.header_len..];
        * = payload;
        let out: [128]u8 = undefined;
        if (formatted := hdr.format(&out)) {
            * = formatted;
        }
    }
}
```
