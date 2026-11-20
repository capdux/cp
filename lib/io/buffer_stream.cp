// Source: /data/dev/bun/src/io/pipes.cp
/**
 * CP Buffer Streams (cap:io)
 * 内存切片、增强型缓冲读写器、组合流装饰器与动态 ChunkedBuffer
 */

const std = @import("std");
import { IOError, SeekOrigin, Reader, Writer, Closer, Seeker } from "./traits";

/**
 * 内存只读流：直接在源切片上无堆分配零拷贝读取与寻址
 */
pub class BufferReader implements Reader, Seeker {
    data: []const u8;
    pos: usize;

    pub fn init(data: []const u8) {
        _.data = data;
        _.pos = 0;
    }

    pub fn read(*_, buf: *[]u8): usize! {
        if (_.pos >= _.data.len) {
            return 0; // EOF
        }
        let available = _.data.len - _.pos;
        let to_copy = buf.len;
        if (to_copy > available) {
            to_copy = available;
        }
        let i: usize = 0;
        while (i < to_copy) {
            buf[i] = _.data[_.pos + i];
            i = i + 1;
        }
        _.pos = _.pos + to_copy;
        return to_copy;
    }

    pub fn seek(*_, offset: i64, origin: SeekOrigin): u64! {
        let new_pos: i64 = 0;
        match (origin) {
            SeekOrigin.Start => {
                new_pos = offset;
            },
            SeekOrigin.Current => {
                new_pos = (_.pos as i64) + offset;
            },
            SeekOrigin.End => {
                new_pos = (_.data.len as i64) + offset;
            },
        }
        if (new_pos < 0 || (new_pos as usize) > _.data.len) {
            return err.InvalidSeek;
        }
        _.pos = new_pos as usize;
        return _.pos as u64;
    }

    pub fn remaining(): usize {
        if (_.pos >= _.data.len) {
            return 0;
        }
        return _.data.len - _.pos;
    }

    pub fn reset(*_) {
        _.pos = 0;
    }
}

/**
 * 内存写入器：栈/堆定长缓冲区写入流
 */
pub class BufferWriter implements Writer {
    buf: [4096]u8;
    written: usize;

    pub fn init() {
        _.buf = [0 as u8; 4096];
        _.written = 0;
    }

    pub fn write(*_, data: []const u8): usize! {
        let available: usize = (4096 as usize) - _.written;
        if (available == 0) {
            return err.BufferFull;
        }
        let to_write = data.len;
        if (to_write > available) {
            to_write = available;
        }
        let i: usize = 0;
        while (i < to_write) {
            _.buf[_.written + i] = data[i];
            i = i + 1;
        }
        _.written = _.written + to_write;
        return to_write;
    }

    pub fn bytes(): []const u8 {
        return _.buf[0.._.written];
    }

    pub fn len(): usize {
        return _.written;
    }

    pub fn reset(*_) {
        _.written = 0;
    }
}

/**
 * 增强型带缓冲读取器：内部 4096 字节缓存
 * 支持 readByte / read / readFull / peek / peekByte / discard / readUntil / readLine
 */
pub class BufferedReader<R: Reader> implements Reader {
    reader: *R;
    cache: [4096]u8;
    cache_len: usize;
    cache_pos: usize;

    pub fn init(reader: *R) {
        _.reader = reader;
        _.cache = [0 as u8; 4096];
        _.cache_len = 0;
        _.cache_pos = 0;
    }

    pub fn buffered(): usize {
        if (_.cache_pos >= _.cache_len) return 0;
        return _.cache_len - _.cache_pos;
    }

    pub fn unreadSlice(): []const u8 {
        if (_.cache_pos >= _.cache_len) return "";
        return _.cache[_.cache_pos.._.cache_len];
    }

    /// 滑动窗口压缩：将未读数据移至缓冲区头部 (0..rem)
    pub fn shift(*_) {
        let rem = _.buffered();
        if (rem > 0 && _.cache_pos > 0) {
            let i: usize = 0;
            while (i < rem) {
                _.cache[i] = _.cache[_.cache_pos + i];
                i += 1;
            }
        }
        _.cache_pos = 0;
        _.cache_len = rem;
    }

    pub fn fill(*_): usize! {
        return _.compactAndFill();
    }

    pub fn clear(*_) {
        _.cache_pos = 0;
        _.cache_len = 0;
    }

    fn compactAndFill(*_): usize! {
        let rem = _.buffered();
        if (rem > 0 && _.cache_pos > 0) {
            let i: usize = 0;
            while (i < rem) {
                _.cache[i] = _.cache[_.cache_pos + i];
                i += 1;
            }
        }
        _.cache_pos = 0;
        _.cache_len = rem;

        if (rem >= 4096) {
            return rem;
        }
        let slice = _.cache[rem..4096];
        let n = _.reader.read(slice)?;
        _.cache_len = rem + n;
        return _.cache_len;
    }

    pub fn readByte(*_): u8! {
        if (_.cache_pos >= _.cache_len) {
            let slice = _.cache[0..4096];
            let n = _.reader.read(slice)?;
            if (n == 0) {
                return err.EndOfStream;
            }
            _.cache_len = n;
            _.cache_pos = 0;
        }
        let b = _.cache[_.cache_pos];
        _.cache_pos = _.cache_pos + 1;
        return b;
    }

    pub fn peekByte(*_): u8! {
        if (_.cache_pos >= _.cache_len) {
            let filled = _.compactAndFill()?;
            if (filled == 0) {
                return err.EndOfStream;
            }
        }
        return _.cache[_.cache_pos];
    }

    /// 预读最多 n 字节（不推进读游标），返回缓存内的只读视图
    pub fn peek(*_, n: usize): ([]const u8)! {
        if (n == 0) return "";
        let want = if (n > 4096) (4096 as usize) else n;
        while (_.buffered() < want) {
            let prev = _.buffered();
            let total = _.compactAndFill()?;
            if (total == prev) {
                break; // EOF reached
            }
        }
        let avail = _.buffered();
        let take = if (want < avail) want else avail;
        return _.cache[_.cache_pos .. _.cache_pos + take];
    }

    /// 丢弃最多 n 字节，返回实际跳过的字节数
    pub fn discard(*_, n: usize): usize! {
        let skipped: usize = 0;
        while (skipped < n) {
            let avail = _.buffered();
            if (avail > 0) {
                let rem = n - skipped;
                let step = if (rem < avail) rem else avail;
                _.cache_pos += step;
                skipped += step;
            } else {
                let slice = _.cache[0..4096];
                let got = _.reader.read(slice)?;
                if (got == 0) break;
                _.cache_len = got;
                _.cache_pos = 0;
            }
        }
        return skipped;
    }

    pub fn read(*_, buf: *[]u8): usize! {
        if (buf.len == 0) {
            return 0;
        }
        let out_idx: usize = 0;
        while (out_idx < buf.len) {
            if (_.cache_pos >= _.cache_len) {
                if (buf.len - out_idx >= 4096) {
                    let direct_n = _.reader.read(buf[out_idx..buf.len])?;
                    if (direct_n == 0) break;
                    out_idx += direct_n;
                    continue;
                }
                let slice = _.cache[0..4096];
                let n = _.reader.read(slice)?;
                if (n == 0) {
                    break;
                }
                _.cache_len = n;
                _.cache_pos = 0;
            }
            let avail = _.cache_len - _.cache_pos;
            let needed = buf.len - out_idx;
            let to_take = needed;
            if (to_take > avail) {
                to_take = avail;
            }
            let i: usize = 0;
            while (i < to_take) {
                buf[out_idx + i] = _.cache[_.cache_pos + i];
                i = i + 1;
            }
            _.cache_pos = _.cache_pos + to_take;
            out_idx = out_idx + to_take;
        }
        return out_idx;
    }

    /// 精确读满 buf.len 字节；若提前遇到 EOF 则返回 err.UnexpectedEof
    pub fn readFull(*_, buf: *[]u8): usize! {
        let total: usize = 0;
        while (total < buf.len) {
            let n = _.read(buf[total..buf.len])?;
            if (n == 0) {
                return err.UnexpectedEof;
            }
            total += n;
        }
        return total;
    }

    /// 持续读取直至遇到分隔符 delim（包含 delim）或 EOF；若 out 满仍未找到 delim 返回 err.BufferFull
    pub fn readUntil(*_, delim: u8, out: *[]u8): usize! {
        let written: usize = 0;
        while (true) {
            if (_.cache_pos >= _.cache_len) {
                let slice = _.cache[0..4096];
                let n = _.reader.read(slice)?;
                if (n == 0) {
                    return written;
                }
                _.cache_len = n;
                _.cache_pos = 0;
            }
            if (written >= out.len) {
                return err.BufferFull;
            }
            let b = _.cache[_.cache_pos];
            _.cache_pos += 1;
            out[written] = b;
            written += 1;
            if (b == delim) {
                return written;
            }
        }
    }

    /// 别名方法：按分隔符读取（含分隔符）至定长切片
    pub fn readUntilDelimiter(*_, delim: u8, out: *[]u8): usize! {
        return _.readUntil(delim, out);
    }

    /// 动态分配版 readUntil（读取直至 delim 或 EOF，包含 delim）
    pub fn readUntilAlloc(*_, allocator: std.mem.Allocator, delim: u8): ([]u8)! {
        let list: std.ArrayListUnmanaged(u8) = .empty;
        errdefer list.deinit(allocator);

        while (true) {
            if (_.cache_pos >= _.cache_len) {
                let slice = _.cache[0..4096];
                let n = _.reader.read(slice)?;
                if (n == 0) {
                    return try list.toOwnedSlice(allocator);
                }
                _.cache_len = n;
                _.cache_pos = 0;
            }
            let b = _.cache[_.cache_pos];
            _.cache_pos += 1;
            try list.append(allocator, b);
            if (b == delim) {
                return try list.toOwnedSlice(allocator);
            }
        }
    }

    /// 读取一行（自动剥离末尾 `\n` 与 `\r\n`）；EOF 时返回 null
    pub fn readLine(*_, out: *[]u8): ([]const u8)?! {
        let n = _.readUntil('\n', out)?;
        if (n == 0) return null;
        let end = n;
        if (end > 0 && out[end - 1] == '\n') {
            end -= 1;
        }
        if (end > 0 && out[end - 1] == '\r') {
            end -= 1;
        }
        return out[0..end];
    }

    /// 动态分配读取一行（自动剥离末尾 `\n` 与 `\r\n`）；EOF 时返回 null
    pub fn readLineAlloc(*_, allocator: std.mem.Allocator): ([]u8)?! {
        let line = try _.readUntilAlloc(allocator, '\n');
        if (line.len == 0) {
            allocator.free(line);
            return null;
        }
        let end = line.len;
        if (end > 0 && line[end - 1] == '\n') {
            end -= 1;
        }
        if (end > 0 && line[end - 1] == '\r') {
            end -= 1;
        }
        if (end == line.len) {
            return line;
        }
        let trimmed = try allocator.dupe(u8, line[0..end]);
        allocator.free(line);
        return trimmed;
    }
}

/**
 * 增强型带缓冲写入器：内部 4096 字节缓存
 * 支持自定义自动刷盘阈值（flush_threshold）与大数据块零拷贝写直通（Zero-Copy Pass-Through）
 */
pub class BufferedWriter<W: Writer> implements Writer {
    writer: *W;
    cache: [4096]u8;
    cache_len: usize;
    flush_threshold: usize;

    pub fn init(writer: *W) {
        _.writer = writer;
        _.cache = [0 as u8; 4096];
        _.cache_len = 0;
        _.flush_threshold = 4096;
    }

    pub static fn withThreshold(writer: *W, threshold: usize): BufferedWriter<W> {
        let bw = new BufferedWriter<W>(writer);
        bw.setThreshold(threshold);
        unsafe { return bw.*; }
    }

    pub fn setThreshold(*_, threshold: usize) {
        if (threshold == 0) {
            _.flush_threshold = 1;
        } else if (threshold > 4096) {
            _.flush_threshold = 4096;
        } else {
            _.flush_threshold = threshold;
        }
    }

    pub fn buffered(): usize {
        return _.cache_len;
    }

    pub fn available(): usize {
        if (_.cache_len >= _.flush_threshold) return 0;
        return _.flush_threshold - _.cache_len;
    }

    fn writeAllDirect(*_, data: []const u8): void! {
        let pos: usize = 0;
        while (pos < data.len) {
            let n = _.writer.write(data[pos..data.len])?;
            if (n == 0) {
                return err.ShortWrite;
            }
            pos += n;
        }
    }

    pub fn clear(*_) {
        _.cache_len = 0;
    }

    pub fn flush(*_): void! {
        if (_.cache_len > 0) {
            let slice = _.cache[0.._.cache_len];
            _.cache_len = 0;
            try _.writeAllDirect(slice);
        }
    }

    pub fn writeByte(*_, b: u8): void! {
        if (_.cache_len >= _.flush_threshold) {
            try _.flush();
        }
        _.cache[_.cache_len] = b;
        _.cache_len += 1;
        if (_.cache_len >= _.flush_threshold) {
            try _.flush();
        }
    }

    pub fn write(*_, data: []const u8): usize! {
        if (data.len == 0) return 0;

        // 大块数据零拷贝直通：先刷空当前缓存，再直接透传到底层 writer
        if (data.len >= _.flush_threshold) {
            try _.flush();
            try _.writeAllDirect(data);
            return data.len;
        }

        let in_idx: usize = 0;
        while (in_idx < data.len) {
            let avail = _.flush_threshold - _.cache_len;
            if (avail == 0) {
                try _.flush();
                avail = _.flush_threshold;
            }
            let rem = data.len - in_idx;
            let to_copy = if (rem < avail) rem else avail;
            let i: usize = 0;
            while (i < to_copy) {
                _.cache[_.cache_len + i] = data[in_idx + i];
                i += 1;
            }
            _.cache_len += to_copy;
            in_idx += to_copy;

            if (_.cache_len >= _.flush_threshold) {
                try _.flush();
            }
        }
        return data.len;
    }
}

/**
 * 限长读取器：最多从底层 Reader 读取 limit 字节，达到上限后返回 0 (EOF)
 */
pub class LimitedReader<R: Reader> implements Reader {
    reader: *R;
    remaining: usize;

    pub fn init(reader: *R, limit: usize) {
        _.reader = reader;
        _.remaining = limit;
    }

    pub fn read(*_, buf: *[]u8): usize! {
        if (_.remaining == 0 || buf.len == 0) {
            return 0;
        }
        let max_len = if (buf.len < _.remaining) buf.len else _.remaining;
        let n = _.reader.read(buf[0..max_len])?;
        if (n > _.remaining) {
            _.remaining = 0;
        } else {
            _.remaining -= n;
        }
        return n;
    }

    pub fn bytesRemaining(): usize {
        return _.remaining;
    }

    pub fn isExhausted(): bool {
        return _.remaining == 0;
    }
}

/**
 * 多路串联读取器：按顺序依次从多个同构 Reader 中读取直至全部耗尽
 */
pub class MultiReader<R: Reader> implements Reader {
    slots: [16]*R;
    slot_count: usize;
    index: usize;

    pub fn init() {
        _.slots = undefined;
        _.slot_count = 0;
        _.index = 0;
    }

    pub static fn initPair(r1: *R, r2: *R): MultiReader<R> {
        let mr = new MultiReader<R>();
        mr.slots[0] = r1;
        mr.slots[1] = r2;
        mr.slot_count = 2;
        unsafe { return mr.*; }
    }

    pub static fn fromSlice(readers: []*R): MultiReader<R> {
        let mr = new MultiReader<R>();
        for (r of readers) {
            if (mr.slot_count < 16) {
                mr.slots[mr.slot_count] = r;
                mr.slot_count += 1;
            }
        }
        unsafe { return mr.*; }
    }

    pub fn add(*_, r: *R) {
        if (_.slot_count < 16) {
            _.slots[_.slot_count] = r;
            _.slot_count += 1;
        }
    }

    pub fn read(*_, buf: *[]u8): usize! {
        if (buf.len == 0) return 0;
        while (_.index < _.slot_count) {
            let n = _.slots[_.index].read(buf)?;
            if (n > 0) {
                return n;
            }
            _.index += 1;
        }
        return 0;
    }

    pub fn remainingReaders(): usize {
        if (_.index >= _.slot_count) return 0;
        return _.slot_count - _.index;
    }
}

/**
 * 分流读取器 (TeeReader)：从 reader 读取数据的同时将所有读到的字节镜像写入 writer
 */
pub class TeeReader<R: Reader, W: Writer> implements Reader {
    reader: *R;
    writer: *W;
    bytes_teed: usize;

    pub fn init(reader: *R, writer: *W) {
        _.reader = reader;
        _.writer = writer;
        _.bytes_teed = 0;
    }

    pub fn read(*_, buf: *[]u8): usize! {
        let n = _.reader.read(buf)?;
        if (n > 0) {
            let pos: usize = 0;
            while (pos < n) {
                let wn = _.writer.write(buf[pos..n])?;
                if (wn == 0) {
                    return err.ShortWrite;
                }
                pos += wn;
            }
            _.bytes_teed += n;
        }
        return n;
    }

    pub fn bytesTransferred(): usize {
        return _.bytes_teed;
    }
}

/**
 * 黑洞写入器 (DiscardWriter / io.Discard)：零开销丢弃所有写入数据并统计总字节数
 */
pub class DiscardWriter implements Writer {
    total_discarded: usize;

    pub fn init() {
        _.total_discarded = 0;
    }

    pub fn write(*_, buf: []const u8): usize! {
        _.total_discarded += buf.len;
        return buf.len;
    }

    pub fn bytesDiscarded(): usize {
        return _.total_discarded;
    }
}

/**
 * 计数写入器装饰器：透传写入底层 writer 并精确累计已写入字节数
 */
pub class CountingWriter<W: Writer> implements Writer {
    writer: *W;
    bytes_written: usize;

    pub fn init(writer: *W) {
        _.writer = writer;
        _.bytes_written = 0;
    }

    pub fn write(*_, data: []const u8): usize! {
        let n = _.writer.write(data)?;
        _.bytes_written += n;
        return n;
    }

    pub fn count(): usize {
        return _.bytes_written;
    }

    pub fn resetCount(*_) {
        _.bytes_written = 0;
    }
}

/**
 * 动态可扩展字节流缓冲 (ChunkedBuffer)：按块增量扩容，支持完整 Reader / Writer / Seeker 契约
 */
pub class ChunkedBuffer implements Reader, Writer, Seeker {
    allocator: std.mem.Allocator;
    buf: std.ArrayListUnmanaged(u8);
    read_pos: usize;
    chunk_size: usize;

    pub fn init(allocator: std.mem.Allocator) {
        _.allocator = allocator;
        _.buf = .empty;
        _.read_pos = 0;
        _.chunk_size = 4096;
    }

    pub static fn withChunkSize(allocator: std.mem.Allocator, chunk_size: usize): ChunkedBuffer {
        let cb = new ChunkedBuffer(allocator);
        cb.chunk_size = if (chunk_size == 0) (4096 as usize) else chunk_size;
        return cb;
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
        _.read_pos = 0;
    }

    fn growFor(*_, needed: usize): !void {
        let req_cap = _.buf.items.len + needed;
        if (req_cap <= _.buf.capacity) return;
        let new_cap = if (_.buf.capacity == 0) _.chunk_size else _.buf.capacity;
        while (new_cap < req_cap) {
            if (new_cap < _.chunk_size) {
                new_cap = _.chunk_size;
            } else {
                new_cap += _.chunk_size;
                if (new_cap < req_cap && new_cap * 2 > new_cap) {
                    let doubled = _.buf.capacity * 2;
                    if (doubled > new_cap) new_cap = doubled;
                }
            }
        }
        try _.buf.ensureTotalCapacityPrecise(_.allocator, new_cap);
    }

    pub fn write(*_, data: []const u8): usize! {
        if (data.len == 0) return 0;
        try _.growFor(data.len);
        _.buf.appendSliceAssumeCapacity(data);
        return data.len;
    }

    pub fn writeByte(*_, b: u8): void! {
        try _.growFor(1);
        _.buf.appendAssumeCapacity(b);
    }

    pub fn writeAll(*_, data: []const u8): void! {
        * = try _.write(data);
    }

    pub fn read(*_, buf: *[]u8): usize! {
        if (_.read_pos >= _.buf.items.len || buf.len == 0) {
            return 0;
        }
        let avail = _.buf.items.len - _.read_pos;
        let to_copy = if (buf.len < avail) buf.len else avail;
        @memcpy(buf[0..to_copy], _.buf.items[_.read_pos .. _.read_pos + to_copy]);
        _.read_pos += to_copy;
        return to_copy;
    }

    pub fn readByte(*_): u8! {
        if (_.read_pos >= _.buf.items.len) {
            return err.EndOfStream;
        }
        let b = _.buf.items[_.read_pos];
        _.read_pos += 1;
        return b;
    }

    pub fn peek(*_, n: usize): []const u8 {
        let avail = _.remaining();
        let take = if (n < avail) n else avail;
        return _.buf.items[_.read_pos .. _.read_pos + take];
    }

    pub fn seek(*_, offset: i64, origin: SeekOrigin): u64! {
        let new_pos: i64 = 0;
        match (origin) {
            SeekOrigin.Start => {
                new_pos = offset;
            },
            SeekOrigin.Current => {
                new_pos = (_.read_pos as i64) + offset;
            },
            SeekOrigin.End => {
                new_pos = (_.buf.items.len as i64) + offset;
            },
        }
        if (new_pos < 0 || (new_pos as usize) > _.buf.items.len) {
            return err.InvalidSeek;
        }
        _.read_pos = new_pos as usize;
        return _.read_pos as u64;
    }

    pub fn bytes(): []const u8 {
        return _.buf.items;
    }

    pub fn unreadBytes(): []const u8 {
        if (_.read_pos >= _.buf.items.len) return "";
        return _.buf.items[_.read_pos.._.buf.items.len];
    }

    pub fn len(): usize {
        return _.buf.items.len;
    }

    pub fn remaining(): usize {
        if (_.read_pos >= _.buf.items.len) return 0;
        return _.buf.items.len - _.read_pos;
    }

    pub fn capacity(): usize {
        return _.buf.capacity;
    }

    pub fn chunkCount(): usize {
        if (_.buf.capacity == 0) return 0;
        return (_.buf.capacity + _.chunk_size - 1) / _.chunk_size;
    }

    pub fn reset(*_) {
        _.read_pos = 0;
    }

    pub fn clear(*_) {
        _.buf.clearRetainingCapacity();
        _.read_pos = 0;
    }

    pub fn toOwnedSlice(*_): []u8! {
        _.read_pos = 0;
        return try _.buf.toOwnedSlice(_.allocator);
    }
}

/**
 * 将数据从读取器全量流式写入到写入器直至 EOF
 * 返回累计传输的总字节数
 */
pub fn copyStream<R: Reader, W: Writer>(r: *R, w: *W): usize! {
    let chunk_buf: [4096]u8 = [0 as u8; 4096];
    let slice = chunk_buf[0..4096];
    let total: usize = 0;
    while (true) {
        let n = r.read(slice)?;
        if (n == 0) {
            break;
        }
        let pos: usize = 0;
        while (pos < n) {
            let written = w.write(chunk_buf[pos..n])?;
            if (written == 0) {
                return err.ShortWrite;
            }
            pos += written;
            total += written;
        }
    }
    return total;
}

/**
 * 精确从读取器复制 n 字节至写入器
 */
pub fn copyN<R: Reader, W: Writer>(r: *R, w: *W, limit: usize): usize! {
    let chunk_buf: [4096]u8 = [0 as u8; 4096];
    let total: usize = 0;
    while (total < limit) {
        let needed = limit - total;
        let to_read: usize = 4096;
        if (needed < to_read) {
            to_read = needed;
        }
        let slice = chunk_buf[0..to_read];
        let n = r.read(slice)?;
        if (n == 0) {
            break;
        }
        let pos: usize = 0;
        while (pos < n) {
            let written = w.write(chunk_buf[pos..n])?;
            if (written == 0) {
                return err.ShortWrite;
            }
            pos += written;
            total += written;
        }
    }
    return total;
}
