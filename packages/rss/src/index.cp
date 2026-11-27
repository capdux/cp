// @cap/rss — Zero-allocation RSS 2.0 XML Feed Generator & Tag Extractor
// Clean-room MIT implementation in pure CP.

import { isSpace, trim } from "cap:ascii";
import { startsWithBytes, endsWithBytes, findByteInBytes } from "cap:bin";
import { writeStr } from "cap:fmt";

pub struct RssChannel {
    pub title: []const u8,
    pub link: []const u8,
    pub description: []const u8,
    pub language: []const u8,
}

pub struct RssItem {
    pub title: []const u8,
    pub link: []const u8,
    pub description: []const u8,
    pub guid: []const u8,
    pub pub_date: []const u8,
}

fn pushSlice(dst: []u8, pos: usize, src: []const u8) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    return writeStr(src, dst, pos) as i64;
}

fn pushXmlEscaped(dst: []u8, pos: usize, src: []const u8) -> i64 {
    let p = pos;
    let i: usize = 0;
    while (i < src.len) {
        const ch = src[i];
        let next: i64 = 0;
        if (ch == 38) {
            next = pushSlice(dst, p, "&amp;");
        } else if (ch == 60) {
            next = pushSlice(dst, p, "&lt;");
        } else if (ch == 62) {
            next = pushSlice(dst, p, "&gt;");
        } else if (ch == 34) {
            next = pushSlice(dst, p, "&quot;");
        } else if (ch == 39) {
            next = pushSlice(dst, p, "&apos;");
        } else {
            if (p >= dst.len) {
                return -1;
            }
            dst[p] = ch;
            next = (p + 1) as i64;
        }
        if (next < 0) {
            return -1;
        }
        p = next as usize;
        i += 1;
    }
    return p as i64;
}

fn pushTag(dst: []u8, pos: usize, tag: []const u8, val: []const u8) -> i64 {
    if (val.len == 0) {
        return pos as i64;
    }
    let p = pushSlice(dst, pos, "<");
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, tag);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, ">");
    if (p < 0) { return -1; }
    p = pushXmlEscaped(dst, p as usize, val);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, "</");
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, tag);
    if (p < 0) { return -1; }
    return pushSlice(dst, p as usize, ">\n");
}

pub struct RssWriter {
    buf: []u8,
    pos: usize,
    item_count: usize,
    failed: bool,
    finished: bool,

    /// Initializes an RSS 2.0 XML writer into `buf` with the channel metadata `ch`.
    pub static fn init(buf: []u8, ch: RssChannel) -> RssWriter {
        let p = pushSlice(buf, 0, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<rss version=\"2.0\">\n<channel>\n");
        if (p >= 0) { p = pushTag(buf, p as usize, "title", ch.title); }
        if (p >= 0) { p = pushTag(buf, p as usize, "link", ch.link); }
        if (p >= 0) { p = pushTag(buf, p as usize, "description", ch.description); }
        if (p >= 0) { p = pushTag(buf, p as usize, "language", ch.language); }

        if (p < 0) {
            return RssWriter {
                buf: buf,
                pos: 0,
                item_count: 0,
                failed: true,
                finished: false,
            };
        }
        return RssWriter {
            buf: buf,
            pos: p as usize,
            item_count: 0,
            failed: false,
            finished: false,
        };
    }

    /// Appends an `<item>` element with strict XML entity escaping.
    /// Optional fields that are empty are omitted automatically.
    pub fn addItem(*_, item: RssItem) -> bool {
        if (_.failed || _.finished) {
            return false;
        }
        const saved = _.pos;
        let p = pushSlice(_.buf, _.pos, "<item>\n");
        if (p >= 0) { p = pushTag(_.buf, p as usize, "title", item.title); }
        if (p >= 0) { p = pushTag(_.buf, p as usize, "link", item.link); }
        if (p >= 0) { p = pushTag(_.buf, p as usize, "description", item.description); }
        if (p >= 0) { p = pushTag(_.buf, p as usize, "guid", item.guid); }
        if (p >= 0) { p = pushTag(_.buf, p as usize, "pubDate", item.pub_date); }
        if (p >= 0) { p = pushSlice(_.buf, p as usize, "</item>\n"); }

        if (p < 0) {
            _.pos = saved;
            _.failed = true;
            return false;
        }
        _.pos = p as usize;
        _.item_count += 1;
        return true;
    }

    /// Closes `</channel></rss>` and returns the total bytes written, or `-1` on overflow.
    pub fn finish(*_) -> i64 {
        if (_.failed) {
            return -1;
        }
        if (!_.finished) {
            const p = pushSlice(_.buf, _.pos, "</channel>\n</rss>\n");
            if (p < 0) {
                _.failed = true;
                return -1;
            }
            _.pos = p as usize;
            _.finished = true;
        }
        return _.pos as i64;
    }

    /// Returns the number of `<item>` entries added.
    pub fn itemCount() -> usize {
        return _.item_count;
    }

    /// Returns the XML bytes written so far.
    pub fn written() -> []const u8 {
        return _.buf[0.._.pos];
    }
}

/// Zero-copy extraction of the first `<tag>...</tag>` value starting at `start_pos`.
/// Automatically strips `<![CDATA[...]]>` wrappers if present. Returns `""` if not found.
pub fn extractTagValue(xml: []const u8, tag: []const u8, start_pos: usize) -> []const u8 {
    if (tag.len == 0 || start_pos >= xml.len) {
        return "";
    }
    const n = xml.len;
    let i = start_pos;

    while (i < n) {
        if (xml[i] == 60 && i + 1 + tag.len < n && startsWithBytes(xml[i + 1..n], tag)) {
            const after = i + 1 + tag.len;
            const sep = xml[after];
            if (sep == 62 || isSpace(sep)) {
                // Find end of opening tag '>'
                const open_end_opt = findByteInBytes(xml, 62, after);
                if (open_end_opt == null) {
                    return "";
                }
                const content_start = open_end_opt.? + 1;

                // Find closing "</tag>"
                let j = content_start;
                while (j + 2 + tag.len < n) {
                    if (xml[j] == 60 && xml[j + 1] == 47 && startsWithBytes(xml[j + 2..n], tag) && xml[j + 2 + tag.len] == 62) {
                        const content = xml[content_start..j];
                        const trimmed = trim(content);
                        if (trimmed.len >= 12 && startsWithBytes(trimmed, "<![CDATA[") && endsWithBytes(trimmed, "]]>")) {
                            return trimmed[9..trimmed.len - 3];
                        }
                        return content;
                    }
                    j += 1;
                }
                return "";
            }
        }
        i += 1;
    }

    return "";
}

/// Counts the number of `<item>` elements in an RSS XML document.
pub fn countItems(xml: []const u8) -> usize {
    const n = xml.len;
    let count: usize = 0;
    let i: usize = 0;
    while (i + 5 < n) {
        if (xml[i] == 60 && startsWithBytes(xml[i + 1..n], "item")) {
            const sep = xml[i + 5];
            if (sep == 62 || isSpace(sep)) {
                count += 1;
                i += 6;
                continue;
            }
        }
        i += 1;
    }
    return count;
}
