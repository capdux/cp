# `cap:nntp` — RFC 3977 NNTP 网络新闻传输协议零分配编解码库

纯 CP、零堆分配的 RFC 3977 NNTP 线协议编解码库，覆盖 3 位状态码响应解析、`GROUP 211` 解析、`ARTICLE/HEAD/BODY/STAT` 编号与 `<message-id>` 提取、RFC 3977 §8.3 TAB 分隔 8 字段 `OVER`/`XOVER` 概览解析、多行响应 dot-unstuffing 零拷贝迭代器及带 CRLF 注入防护的命令编码器。

## 导入

```cp
import {
    NNTP_TCP_PORT,
    NNTPS_TCP_PORT,
    NNTP_MAX_CMD_LEN,
    CODE_READY_POST,
    CODE_READY_NOPOST,
    CODE_CLOSING,
    CODE_GROUP_OK,
    CODE_LIST_FOLLOWS,
    CODE_ARTICLE_FOLLOWS,
    CODE_HEAD_FOLLOWS,
    CODE_BODY_FOLLOWS,
    CODE_STAT_OK,
    CODE_OVER_FOLLOWS,
    CODE_SEND_ARTICLE,
    CODE_NO_SUCH_GROUP,
    CODE_NO_SUCH_ARTICLE,
    CODE_SYNTAX_ERROR,
    NntpResponse,
    parseNntpResponse,
    NntpGroupInfo,
    parseNntpGroup,
    NntpArticleStat,
    parseNntpArticleStat,
    NntpOverview,
    parseNntpOverview,
    NntpMultiLineIter,
    encodeNntpCommand,
} from "cap:nntp";
```

## 核心 API

- **协议常量与状态码**：`NNTP_TCP_PORT (119)`、`NNTPS_TCP_PORT (563)`、`NNTP_MAX_CMD_LEN (512)`，以及 `CODE_READY_POST (200)`、`CODE_READY_NOPOST (201)`、`CODE_CLOSING (205)`、`CODE_GROUP_OK (211)`、`CODE_LIST_FOLLOWS (215)`、`CODE_ARTICLE_FOLLOWS (220)`、`CODE_HEAD_FOLLOWS (221)`、`CODE_BODY_FOLLOWS (222)`、`CODE_STAT_OK (223)`、`CODE_OVER_FOLLOWS (224)`、`CODE_SEND_ARTICLE (340)`、`CODE_NO_SUCH_GROUP (411)`、`CODE_NO_SUCH_ARTICLE (430)`、`CODE_SYNTAX_ERROR (500)`。
- **状态响应解析**：`parseNntpResponse(line: []const u8): NntpResponse?`，返回 `{ code, text }` 并提供 `isInfo()` (1xx)、`isOk()` (2xx)、`isContinue()` (3xx)、`isTempError()` (4xx)、`isPermError()` (5xx)。
- **新闻组选择解析**：`parseNntpGroup(resp_text: []const u8): NntpGroupInfo?`，解析 `211` 回复正文提取 `{ count, low, high, name }`。
- **文章状态解析**：`parseNntpArticleStat(resp_text: []const u8): NntpArticleStat?`，解析 `220/221/222/223` 回复正文提取 `{ number, message_id }` 并校验 `<...>` 格式。
- **概览记录解析**：`parseNntpOverview(line: []const u8): NntpOverview?`，零拷贝解析 RFC 3977 §8.3.2 TAB 分隔 8 字段（`article_num`, `subject`, `from`, `date`, `message_id`, `references`, `bytes`, `lines`），安全兼容尾部扩展字段。
- **多行数据块迭代**：`NntpMultiLineIter.init(data)` 按 `\r\n` 零拷贝迭代直至独立 `.\r\n` 终止行，自动对 `..` 开头行执行 dot-unstuffing，并提供 `isTerminated()` 与 `bytesConsumed()`。
- **零分配命令编码**：`encodeNntpCommand(verb, arg, out: []u8): usize?`，强制校验 `NNTP_MAX_CMD_LEN`（512 字节）上限并拒绝非法命令字与 CR/LF/NUL 注入。

## 示例

```cp
import {
    parseNntpResponse,
    parseNntpGroup,
    parseNntpOverview,
    NntpMultiLineIter,
    encodeNntpCommand,
} from "cap:nntp";

fn example() {
    if (resp := parseNntpResponse("211 1234 3000234 3002322 misc.test\r\n")) {
        if (resp.isOk()) {
            if (grp := parseNntpGroup(resp.text)) {
                * = grp.count;
            }
        }
    }

    let it = NntpMultiLineIter.init("Subject: Hello\r\n..quoted dot\r\n.\r\n");
    while (true) {
        if (line := it.next()) {
            * = line;
        } else {
            break;
        }
    }

    let buf: [64]u8 = [0; 64];
    * = encodeNntpCommand("GROUP", "comp.lang.cp", buf[0..]);
}
```
