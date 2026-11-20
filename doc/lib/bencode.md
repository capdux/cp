// Source: cp/doc/lib/bencode.md
# `cap:bencode` — BEP-3 Bencode 编解码与 Torrent 规范化哈希引擎

纯 CP、零 C 依赖的 BitTorrent BEP-3 Bencode 严格编解码器、规范化字典序编码器、零拷贝原始段提取器与 SHA-1 `info_hash` 计算引擎。

## 导入方式

```cp
import {
    BENCODE_MAX_DEPTH,
    BencodeError,
    BencodeKind,
    BencodeDictEntry,
    BencodeValue,
    decode,
    decodeWithLimits,
    encode,
    extractRawSection,
    computeInfoHashSha1,
    formatInfoHashHex,
} from "cap:bencode";

// 或动态命名空间挂载：
import cap from "cap";
// cap.bencode.decode / cap.bencode.encode / cap.bencode.computeInfoHashSha1
```

## 核心类型与错误建模

### `BencodeError`
- `UnexpectedEof`：输入提前结束或字节串长度超出剩余缓冲
- `InvalidSyntax`：非法语法前缀、缺失分隔符、非字节串字典键或负字节串长度
- `InvalidInteger`：空整数（`ie`）、非数字字符或 `i64` 溢出
- `LeadingZero`：整数或字节串长度出现非法前导零（如 `i03e`、`i-03e`、`02:ab`）
- `NegativeZero`：非法负零整数 `i-0e`
- `UnorderedDictKeys`：字典键未按原始字节字典序严格递增
- `DuplicateDictKey`：字典中出现重复键
- `TrailingBytes`：顶层值解码完毕后存在多余尾部字节
- `MaxDepthExceeded`：嵌套深度超过限制（默认 `BENCODE_MAX_DEPTH = 64`）
- `MaxSizeExceeded`：解码节点总数超过 `max_items` 限制
- `KeyNotFound`：在顶层字典中未找到目标键
- `OutOfMemory`：内存分配失败

### `BencodeDictEntry` & `BencodeValue`
- `BencodeDictEntry`：`{ key: []const u8, value: BencodeValue }`
- `BencodeValue`：`Int(i64)` | `Bytes([]const u8)` | `List([]BencodeValue)` | `Dict([]BencodeDictEntry)`
  - `deinit(*_, alloc: std.mem.Allocator)`：递归释放所有堆分配切片
  - `get(key: []const u8): BencodeValue?`
  - `getBytes(key: []const u8): ([]const u8)?`
  - `getInt(key: []const u8): i64?`
  - `equals(other: BencodeValue): bool`
  - `clone(alloc: std.mem.Allocator): BencodeValue ! BencodeError`

## 编解码与 Torrent 哈希 API

- `decode(alloc, data: []const u8): BencodeValue ! BencodeError`
  - 使用默认深度上限 `BENCODE_MAX_DEPTH = 64` 严格解码 BEP-3 数据。
- `decodeWithLimits(alloc, data: []const u8, max_depth: usize, max_items: usize): BencodeValue ! BencodeError`
  - 支持自定义最大嵌套深度与最大节点数量上限。
- `encode(alloc, value: BencodeValue): ([]u8) ! BencodeError`
  - 规范化编码：字典自动按原始字节字典序排序输出。
- `extractRawSection(data: []const u8, dict_key: []const u8): ([]const u8) ! BencodeError`
  - 零分配扫描顶层字典并直接返回指定键对应值的原始切片。
- `computeInfoHashSha1(torrent_bytes: []const u8): ([20]u8) ! BencodeError`
  - 提取原始 `"info"` 字典切片并通过 `std.crypto.hash.Sha1` 计算 20 字节 `info_hash`。
- `formatInfoHashHex(digest: [20]u8): [40]u8`
  - 将 20 字节哈希格式化为 40 位小写十六进制数组。
