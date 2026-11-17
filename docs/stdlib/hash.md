# cap:hash — 非密码学极速散列与校验和标准库

> **模块协议**：`cap:hash`  
> **源码路径**：`lib/hash/index.cp`  
> **底层算法**：Zig Standard Library `std.hash`（XxHash3-SIMD、Murmur3、CityHash、CRC32、Adler32、SipHash）

---

## 一、模块总览

`cap:hash` 专为高性能内存查找、大数据分片、BloomFilter、分布式一致性路由、报文完整性校验以及防 Hash-DoS 碰撞攻击设计。核心搭载由 Yann Collet 设计的 XxHash3 算法，基于 AVX2/NEON SIMD 指令集可在现代多核 CPU 上实现 30~50 GB/s 的极限吞吐。

---

## 二、API 快速索引

| 函数/类型 | 输出位宽 | 吞吐量级 | 算法与典型应用场景 |
| :--- | :--- | :--- | :--- |
| `Hash.xxhash3(buf, seed)` | 64 位 | 30~50 GB/s (SIMD) | 现代最快非加密哈希，哈希表键值、内存缓存定位 |
| `Hash.xxhash128(buf, seed)` | 128 位 | 25~40 GB/s (SIMD) | 超大容量分布式数据集、全局唯一键防碰撞散列 |
| `Hash.murmur3(buf, seed)` | 32 位 | 3~6 GB/s | 大数据生态标准算法（Cassandra、Kafka、BloomFilter） |
| `Hash.cityhash64(buf)` | 64 位 | 5~10 GB/s | Google 出品，针对中短字符串极限优化 |
| `Hash.crc32(buf)` | 32 位 | 硬件线速 (~20 GB/s) | IEEE 802.3 标准，以太网、PNG、ZIP 硬件加速校验和 |
| `Hash.adler32(buf)` | 32 位 | 8~15 GB/s | RFC 1950 标准，Zlib / Deflate 压缩校验和 |
| `Hash.siphash(buf, key)` | 64 位 | 1~3 GB/s | Jean-Philippe Aumasson 研发，防 Hash-DoS 攻击伪随机散列 |
| `Hash.siphash128(buf, key)` | 128 位 | 1~2 GB/s | 128 位防 DoS 消息认证标签 |
| `Hash.fnv1a(buf)` | 64 位 | 1~3 GB/s | 极小代码体积，微型标识符短串散列 |
| `Hash.hashInt(val)` | 64 位 | 0 内存开销 | 64 位整数雪崩置换（mx3 构造），数据库 ID 快速散列 |
| `XxHash3Stream` | 64 位 | 流式累加 | 大文件与网络数据流增量分块哈希计算句柄 |

---

## 三、代码示例

### 1. XxHash3 单次极速计算与 128 位散列
```typescript
import { test, expect_eq } from "cap:test";
import { Hash } from "cap:hash";

fn main() {
    let payload = "High-throughput AI Agent Task Payload";

    // 64 位 XxHash3 (默认 seed = 0)
    let h64: u64 = Hash.xxhash3(payload);
    
    // 带自定义 seed
    let h64_seed: u64 = Hash.xxhash3(payload, 0x12345678);

    // 128 位极低碰撞散列
    let h128: u128 = Hash.xxhash128(payload);
    expect_eq(h128 != 0, true);
}
```

### 2. XxHash3Stream 流式增量散列
```typescript
import { test, expect_eq } from "cap:test";
import { Hash, XxHash3Stream } from "cap:hash";

fn main() {
    let stream = XxHash3Stream.init(0);

    // 模拟逐块接收网络数据流
    stream.update("Chunk 1: Initializing agent...");
    stream.update("Chunk 2: Processing prompt...");
    stream.update("Chunk 3: Execution finished.");

    // 获取最终 64 位摘要
    let digest: u64 = stream.digest();
    expect_eq(digest != 0, true);
}
```

### 3. CRC32 与 Adler32 数据完整性校验
```typescript
import { test, expect_eq } from "cap:test";
import { Hash } from "cap:hash";

fn main() {
    // CRC32 标准测试向量验证: "123456789" -> 0xCBF43926
    let crc = Hash.crc32("123456789");
    expect_eq(crc, 0xcbf43926);

    // Adler32 标准测试向量验证: "example" -> 0x0BC002ED
    let adler = Hash.adler32("example");
    expect_eq(adler, 0xbc002ed);
}
```

### 4. SipHash 防 Hash-DoS 碰撞攻击
```typescript
import { test, expect_eq } from "cap:test";
import { Hash } from "cap:hash";

fn main() {
    // 16 字节密钥（服务端进程随机生成）
    let secret_key: [16]u8 = [
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    ];

    let untrusted_input = "UserProvidedHeaderValue";
    let sip: u64 = Hash.siphash(untrusted_input, secret_key);
    expect_eq(sip != 0, true);
}
```

### 5. 整数快速哈希
```typescript
import { Hash } from "cap:hash";

fn main() {
    let user_id: u64 = 100086;
    let slot = Hash.hashInt(user_id) % 1024; // 均匀散列到 1024 个桶
}
```
