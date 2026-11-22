# CP 标准库：高速非加密哈希算法矩阵使用手册 (`cap:hash`)

> 参考来源：`/data/dev/bun/docs/runtime/hashing.mdx`, `/data/dev/bun/src/wyhash/wyhash.cp` | 吸收日期：2026-11-22

## 一、 架构总览与设计哲学

`cap:hash` 是 CP 语言官方内置的**超高速非加密哈希（Non-Cryptographic Hash Functions）与校验和矩阵**标准库。模块吸收自 Bun（`/data/dev/bun/src/wyhash/wyhash.cp`）与 Zig 标准库硬件加速原语，涵盖 64/128 位极速内存哈希（**Wyhash**、**XxHash3**、**XxHash128**、**CityHash64**）、32/64 位通用字典与布隆过滤器哈希（**MurmurHash3**、**FNV-1a**、**djb2**、**hashInt**）、抗碰撞洪泛安全伪随机函数（**SipHash-2-4**）以及帧完整性校验和（**CRC32**、**Adler32**）。

> 💡 **密码学哈希分界提示**：若需 SHA-256、SHA-512、BLAKE3、HMAC 等密码学安全摘要，请使用 `cap:crypto`；`cap:hash` 专注于内存哈希表分片、分布式一致性哈希、内容寻址缓存键与高速数据指纹。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              cap:hash 高速非加密哈希矩阵                                │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│   极速 64/128-bit 内存哈希 │  经典字符串/布隆/整型雪崩    │   抗碰撞 PRF 与流式增量引擎   │
│  • Hash.wyhash (Bun 默认) │  • Hash.murmur3 (32-bit)    │  • Hash.siphash (64-bit)     │
│  • Hash.xxhash3 (SIMD)    │  • Hash.fnv1a (64-bit)      │  • Hash.siphash128 (128-bit) │
│  • Hash.xxhash128         │  • Hash.fnv1a32 (32-bit)    │  • WyhashStream (流式 Wyhash)│
│  • Hash.cityhash64        │  • Hash.djb2 (Bernstein)    │  • XxHash3Stream (流式SIMD)  │
│                           │  • Hash.hashInt (64位雪崩)  │  • Hash.crc32 / adler32      │
└───────────────────────────┴─────────────────────────────┴──────────────────────────────┘
```

---

## 二、 算法性能与碰撞特征选型矩阵

| 算法函数 | 输出位宽 | 核心运算机制 | 吞吐量级 (现代 64-bit CPU) | 种子支持 | 首选工程场景 |
| :--- | :---: | :--- | :---: | :---: | :--- |
| **`Hash.wyhash`** | 64-bit (`u64`) | $64 \times 64 \to 128$-bit 乘法折叠 (`MUM`) | **~16 - 25 GB/s** (短键极速) | `seed: u64` | 内存哈希表键、短字符串/标识符寻址（对标 `Bun.hash.wyhash`） |
| **`Hash.xxhash3`** | 64-bit (`u64`) | AVX2 / AVX-512 / NEON 向量条带累加 | **~30 - 50 GB/s** (长块之王) | `seed: u64` | 大文件去重指纹、构建缓存 Key、大块内存快照比对 |
| **`Hash.xxhash128`** | 128-bit (`u128`) | 双种子 XxHash3 128 位合成 | **~25 - 40 GB/s** | `seed: u64` | 全局唯一内容寻址（CAS）、零碰撞风险资产签名 |
| **`Hash.cityhash64`** | 64-bit (`u64`) | 多路移位乘法混淆 | **~12 - 18 GB/s** | 无 | 64~256 字节中等长度 URL / 路径字符串哈希 |
| **`Hash.murmur3`** | 32-bit (`u32`) | 32 位旋转乘法 (`c1=0xcc9e2d51`) | **~4 - 6 GB/s** | `seed: u32` | 布隆过滤器（Bloom Filter）多哈希族、分布式分片 |
| **`Hash.fnv1a` / `fnv1a32`** | 64 / 32-bit | 逐字节异或乘质数 (`0x100000001b3`) | **~1 - 2 GB/s** (代码极简) | 固定 Offset | 编译器符号表、极短关键字（$\le 16\text{B}$）编译期/运行时哈希 |
| **`Hash.djb2`** | 32-bit (`u32`) | `h = h * 33 + c` (`seed = 5381`) | **~1.5 - 2.5 GB/s** | 固定 `5381` | C 兼容传统字典、轻量配置项快速分桶 |
| **`Hash.siphash` / `128`** | 64 / 128-bit | ARX (Add-Rotate-XOR) 2-4 轮伪随机置换 | **~2 - 3 GB/s** | `key: [16]u8`| 面向公网输入的 Map，防御精心构造的哈希碰撞退化 |
| **`Hash.crc32` / `adler32`**| 32-bit (`u32`) | 硬件 CRC32 指令 / 模 65521 累加 | **~10 - 20 GB/s** | 无 | Gzip / Zlib / PKZIP 容器帧完整性校验 |
| **`Hash.hashInt`** | 64-bit (`u64`) | 64 位整数全雪崩可逆置换 | **< 1 ns / 次** (纯寄存器) | 无 | 指针地址、Actor PID、自增 ID 的均匀打散 |

---

## 三、 完整可运行代码示例

### 1. 单发哈希计算矩阵 (`Hash.*`)

```typescript
import { Hash } from "cap:hash";

pub fn demoOneShotHashes() {
    const payload = "cap:hash high-speed non-cryptographic hashing engine";

    // 1. 64-bit & 128-bit 极速哈希
    const h_wy = Hash.wyhash(payload, 0);
    const h_xx3 = Hash.xxhash3(payload, 0);
    const h_xx128 = Hash.xxhash128(payload, 0);
    const h_city = Hash.cityhash64(payload);

    // 2. 经典字符串与布隆过滤器哈希
    const h_murmur = Hash.murmur3(payload, 42);
    const h_fnv64 = Hash.fnv1a(payload);
    const h_fnv32 = Hash.fnv1a32(payload);
    const h_djb2 = Hash.djb2(payload);

    // 3. 64 位整数雪崩打散
    const h_int = Hash.hashInt(123456789);

    // 4. 带 128-bit 密钥的 SipHash-2-4 抗碰撞 PRF
    let secret_key: [16]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
    const h_sip64 = Hash.siphash(payload, secret_key);
    const h_sip128 = Hash.siphash128(payload, secret_key);

    // 5. 标准校验和
    const crc = Hash.crc32("123456789");   // 0xCBF43926
    const adler = Hash.adler32("example"); // 0x0BC002ED
}
```

---

### 2. 流式分块增量哈希 (`WyhashStream` & `XxHash3Stream`)

当对流式读取的大文件或分块网络报文计算 64 位指纹时，使用 `WyhashStream` 或 `XxHash3Stream` 可保证分块增量计算的最终摘要与全量单发计算 **100% 比特级一致**：

```typescript
import { Hash, WyhashStream, XxHash3Stream } from "cap:hash";

pub fn demoStreamingHashes() {
    // 1. Wyhash 流式增量计算（吸收自 Bun Wyhash11）
    let wy_stream = WyhashStream.init(0);
    wy_stream.update("Hello, ");
    wy_stream.update("Wyhash Streaming!");
    const wy_digest = wy_stream.digest();
    const wy_oneshot = Hash.wyhash("Hello, Wyhash Streaming!", 0);
    // wy_digest == wy_oneshot

    // 2. XxHash3 流式增量计算
    let xx_stream = XxHash3Stream.init(0);
    xx_stream.update("Chunk 1 | ");
    xx_stream.update("Chunk 2");
    const xx_digest = xx_stream.digest();
    const xx_oneshot = Hash.xxhash3("Chunk 1 | Chunk 2", 0);
    // xx_digest == xx_oneshot
}
```

---

## 四、 与 `Bun.hash` API 全面对照表

| 算法能力 | Bun JS API (`Bun.hash`) | Bun 底层实现 (`src/wyhash/`) | CP 标准库 (`cap:hash`) |
| :--- | :--- | :--- | :--- |
| **默认 64 位极速哈希** | `Bun.hash(input, seed)` | `Wyhash11.hash(seed, input)` | `Hash.wyhash(input, seed)` / `WyhashStream` |
| **Wyhash 显式调用** | `Bun.hash.wyhash(input, seed)`| `WyhashStateless.hash` | `Hash.wyhash(input, seed)` |
| **XxHash3 / XxHash128**| `Bun.hash.xxHash3(input)` | `std.hash.XxHash3` | `Hash.xxhash3(input, seed)` / `Hash.xxhash128` |
| **MurmurHash3 (32-bit)**| `Bun.hash.murmur32v3(input)` | `std.hash.Murmur3_32` | `Hash.murmur3(input, seed)` |
| **CityHash64** | `Bun.hash.cityHash64(input)` | `std.hash.CityHash64` | `Hash.cityhash64(input)` |
| **CRC32 / Adler32** | `Bun.hash.crc32` / `adler32` | `std.hash.Crc32` / `Adler32` | `Hash.crc32(input)` / `Hash.adler32(input)` |
| **FNV-1a / djb2 / SipHash**| 内部使用 | `std.hash.Fnv1a_64` / `SipHash`| `Hash.fnv1a` / `Hash.fnv1a32` / `Hash.djb2` / `Hash.siphash` |


---

## 一致性哈希子模块（已收敛至 `cap:hash`）

- **`lib/hash/conshash.cp`（虚拟节点哈希环）**：`ConsistentHashError`, `HashRing`
- **`lib/hash/jump.cp`（Jump Consistent Hash & HRW 最高随机权重）**：`JumpError`, `jumpConsistentHash`, `jumpConsistentHashBytes`, `WeightedNode`, `hrwSelect`, `hrwSelectWeighted`
- **`lib/hash/maglev.cp`（Google Maglev 查表路由）**：`DEFAULT_TABLE_SIZE`, `MaglevError`, `Backend`, `MaglevRouter`

---

## 上游 Bun 行为契约与安全选型边界

1. **非加密哈希与密码学哈希严格分界（参考 `/data/dev/bun/docs/runtime/hashing.mdx`）**：
   - `Hash.wyhash`、`Hash.xxhash3`、`Hash.murmur3`、`Hash.cityhash64`、`Hash.crc32` 均为**非加密**高速指纹函数，严禁用于密码存储、令牌签名或防篡改消息认证（MAC）；若面向不可信外部键需防哈希碰撞退化，应选用 `Hash.siphash` / `Hash.siphash128` 并传入随机 128-bit 密钥；密码哈希与加密摘要请使用 `cap:crypto`。
2. **流式增量 vs 一次性哈希一致性**：
   - `WyhashStream` 与 `XxHash3Stream` 无论如何切分输入 chunk，最终摘要与单次调用 `Hash.wyhash` / `Hash.xxhash3` 在相同 `seed` 下保持比特级完全一致，适合网络流与大文件分块指纹计算。
3. **一致性哈希路由选型**：
   - 节点编号连续且固定规模集群优先选用零内存的 `jumpConsistentHash`；支持任意节点上下线与加权均衡时选用 `HashRing`、`hrwSelectWeighted` 或 $O(1)$ 查表 `MaglevRouter`。
