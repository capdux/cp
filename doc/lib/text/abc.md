# `cap:abc` — 纯 CP ABC Music Notation v2.1 乐谱零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

## 概述

`cap:abc` 提供纯 CP、零外部依赖、零堆分配的 ABC Music Notation v2.1 乐谱头与音符事件词法解析器：

- **乐谱头解析**：解析 `X:`（曲号）、`T:`（标题）、`C:`（作曲）、`M:`（拍号，支持 `"C"`=4/4、`"C|"`=2/2 及 `N/D`）、`L:`（默认音长，默认 `1/8`）、`Q:`（速度 BPM，支持 `Q:120` 与 `Q:1/4=120`）及 `K:`（调号并定界 `body_offset`），自动跳过 `%` 行注释与行尾注释；
- **音符与 MIDI 音高换算**：精确解码临时升降号（`^^`/`^`/`=`/`_`/`__`）、八度修饰（`'`/` ,`）并自动换算为标准 MIDI 音高编号（`C=60`, `c=72`），完整支持时值倍率（`A2`, `B/`, `B//`, `C3/2`）；
- **正文零分配词法流**：`AbcEventIter` 自动跳过 `%` 注释、`"..."` 吉他和弦文本、`!trill!` 装饰符与 `[M:...]` 行内字段，流式产出音符、休止符（`z`/`Z`/`x`）、小节线（`|`, `||`, `|:`, `:|`, `|]`）、和弦括号（`[`/`]`）、连音线（`-`）与圆滑线（`(`/` )`）；
- **乐谱统计汇总**：`summarizeAbc` 统计音符数、休止符数、小节线数、和弦数与 MIDI 音域 `[min_midi, max_midi]`。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `AbcFraction` | 分数结构：`num: u32`, `den: u32` |
| `AbcHeader` / `parseAbcHeader(data): AbcHeader?` / `isAbc(data): bool` | 解析并校验 `X:` .. `K:` 乐谱头及 `body_offset` |
| `AbcEventKind` | 事件枚举：`note`, `rest`, `barline`, `chord_start`, `chord_end`, `tie`, `slur_start`, `slur_end` |
| `AbcEvent` | 事件结构：含 `pitch_letter`, `accidental`, `octave_shift`, `midi_note: u8?`, `dur_num`, `dur_den`, `bar_text` |
| `AbcEventIter` | 正文零分配词法迭代器：`init(data): AbcEventIter?`、`fromBody(body)`、`next(*_): AbcEvent?` |
| `AbcStats` / `summarizeAbc(data): AbcStats` | 统计 `note_count`, `rest_count`, `bar_count`, `chord_count`, `min_midi`, `max_midi` |

## 示例

```cp
import { parseAbcHeader, AbcEventIter, AbcEventKind, summarizeAbc } from "cap:text";

fn countNotes(score: []const u8): usize {
    const hdr = parseAbcHeader(score) ?? return 0;
    * = hdr.title;

    let it = AbcEventIter.init(score) ?? return 0;
    let notes: usize = 0;
    while (ev := it.next()) {
        if (ev.kind == AbcEventKind.note) {
            notes += 1;
        }
    }
    * = summarizeAbc(score);
    return notes;
}
```
