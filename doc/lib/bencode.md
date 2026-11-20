// Source: cp/doc/lib/bencode.md
# `cap:bencode` — BEP-3 Bencode 编解码器与 Torrent 元信息引擎

纯 CP、零 C 依赖的 BitTorrent BEP-3 Bencode 严格编解码器、规范字典序校验器与 `.torrent` MetaInfo / SHA-1 `info_hash` 提取引擎（复用 `cap:crypto` SHA-1）。

## 导入方式

```cp
import {
    BENCODE_MAX_DEPTH,
    BencodeError,
    BencodeKind,
    BencodeDictEntry,
    BencodeValue,
    BencodeDoc,
    BencodeCodec,
    TorrentMetaInfo,
    TorrentParser,
} from "cap:bencode";

// 或动态命名空间挂载：
import cap from "cap";
// cap.bencode.BencodeCodec / cap.bencode.TorrentParser
```

## 核心类型

### `BencodeError`
- `UnexpectedEof`：输入提前结束或字节串长度超出剩余缓冲
- `InvalidInteger`：非法的整数格式或 `i64` 溢出
- `LeadingZeroNotAllowed`：整数或字节串长度出现非法前导零（如 `i03e`、`02:ab`）
- `NegativeZeroNotAllowed`：出现非法负零整数 `i-0e`
- `InvalidStringLength`：非法的字节串长度或字典键非字节串
- `UnsortedDictKeys`：字典键未按原始字节序严格递增
- `DuplicateDictKey`：字典中出现重复键
- `TrailingBytes`：顶层值解析完毕后存在多余尾部字节
- `MaxDepthExceeded`：列表/字典嵌套深度超过 `BENCODE_MAX_DEPTH`（64）
- `InvalidTorrent`：缺少必需字段或字段类型不合法的 `.torrent` 元数据
- `OutOfMemory`：内存分配失败

### `BencodeKind` & `BencodeValue` & `BencodeDoc`
- `BencodeKind`：`Int`、`Bytes`、`List`、`Dict`
- `BencodeValue`：
  - `makeInt(v: i64)` / `makeBytes(v: []const u8)` / `makeList(items: []BencodeValue)` / `makeDict(entries: []BencodeDictEntry)`
  - `kind(): BencodeKind`
  - `asInt(): i64?`、`asBytes(): ([]const u8)?`、`asList(): ([]BencodeValue)?`、`asDict(): ([]BencodeDictEntry)?`
  - `get(key: []const u8): BencodeValue?`
  - `deinit(*_, alloc: std.mem.Allocator)`：完整递归释放堆分配节点
- `BencodeDoc`：
  - `init(alloc, root)` / `deinit(*_)`
  - `value(): BencodeValue`、`kind()`、`get(key)`、`asInt()`、`asBytes()`、`asList()`、`asDict()`

## 编解码 (`BencodeCodec`)
- `BencodeCodec.decode(alloc, data: []const u8): BencodeDoc ! BencodeError`
  - 严格执行 BEP-3 校验：拦截 `i03e`、`i-0e`、`02:ab`、乱序字典键、重复字典键、尾部多余字节及 64 层深度限制。
- `BencodeCodec.encode(alloc, val: BencodeValue): ([]u8) ! BencodeError`
- `BencodeCodec.encodeDoc(alloc, doc: BencodeDoc): ([]u8) ! BencodeError`
  - 对字典自动按原始字节序排序后输出规范 Bencode。

## Torrent 元信息与 InfoHash (`TorrentParser`)
- `TorrentParser.parseMetaInfo(alloc, torrent_bytes: []const u8): TorrentMetaInfo ! BencodeError`
  - 直接定位原始字节流中顶层 `"info"` 字典的起止切片，通过 `cap:crypto` 的 `sha1Raw` 计算 20 字节 `info_hash` 与 40 位小写 hex `info_hash_hex`。
  - 自动兼容单文件（`length`）与多文件（`files` 累加）`.torrent` 结构，提取 `announce`、`name`、`piece_length`、`pieces_count`、`total_size`。
  - 使用完毕后调用 `meta.deinit()` 释放内存。
