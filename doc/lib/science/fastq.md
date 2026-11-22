# `cap:fastq` — 高通量基因测序 FASTQ 零分配解析与质控统计库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:science`，导入请使用 `import { ... } from "cap:science"`。

> **导入方式**：`import { PHRED_OFFSET_33, PHRED_OFFSET_64, isFastq, FastqIter, summarizeFastq } from "cap:science";`  
> **内存模型**：100% 纯 CP 零堆分配，每条读段的 `id`/`description`/`seq`/`qual` 切片零拷贝指向原始输入缓冲区。

---

## 核心常量

- `PHRED_OFFSET_33 = 33`（Sanger / Illumina 1.8+）
- `PHRED_OFFSET_64 = 64`（旧版 Illumina 1.3–1.7）

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isFastq` | `fn isFastq(data: []const u8): bool` | 校验首字节为 `'@'` 且存在合法完整 4 行读段（第 3 行 `'+'` 且 `seq.len == qual.len`） |
| `FastqRecord` | `phredAt`, `meanQuality`, `gcContent`, `minQuality`, `trimTailEnd` | 单条读段零拷贝视图及 Phred 评分、GC 含量与 3' 端低质量修剪长度计算 |
| `FastqIter` | `FastqIter.init(data)` / `next(): FastqRecord?` | 逐条迭代 4 行 FASTQ 读段（兼容 `\r\n`/`\n`，跳过空行，畸形置 `had_error = true`） |
| `FastqStats` / `summarizeFastq` | `fn summarizeFastq(data, offset): FastqStats` | 单遍统计 `read_count`, `total_bases`, `q20_bases`, `q30_bases`, `gc_bases`, `n_bases`, `min_len`, `max_len` 及 `q20Rate()`/`q30Rate()`/`gcRate()` |

---

## 使用示例

```typescript
import { PHRED_OFFSET_33, FastqIter, summarizeFastq } from "cap:science";

fn qcFastq(bytes: []const u8) {
    const stats = summarizeFastq(bytes, PHRED_OFFSET_33);
    * = stats.q30Rate();
    let it = FastqIter.init(bytes);
    while (rec := it.next()) {
        const safe_len = rec.trimTailEnd(20, PHRED_OFFSET_33);
        const trimmed_seq = rec.seq[0..safe_len];
        * = trimmed_seq;
    }
}
```
