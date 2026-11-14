/**
 * CP File System SDK (cap:fs)
 * 使用 @is_nest 进行显式双模分支逻辑重构
 */
const builtin = @import("builtin");

import { SystemAction } from "cap:beam";

/**
 * 生产级异步读取
 * 逻辑对开发者完全透明
 */
pub async fn readFile(path: string): string {
    if (@is_nest) {
        // --- BEAM 模式：委托 I/O 网关 ---
        // 发送 FS_READ 请求并让出执行权，等待唤醒
        return await @yield_to_beam(@intFromEnum(SystemAction.FS_READ), path);
    } else {
        // --- Native 模式：直连 Zig std.fs ---
        // 在原生 Reactor 环境下，此操作由编译器映射为同步读取或非阻塞 Poll
        return @fs_read_file(path);
    }
}

/**
 * 生产级异步写入
 */
pub async fn writeFile(path: string, content: string) {
    if (@is_nest) {
        // 构造包含 path 和 content 的载荷（此处简化为 path）
        // 真实的复杂载荷将通过 SharedTensor 或专用编码处理
        await @yield_to_beam(@intFromEnum(SystemAction.FS_WRITE), path);
    } else {
        @fs_write_file(path, content);
    }
}

/**
 * 生产级同步读取
 */
pub fn readFileSync(path: string): string {
    return @fs_read_file(path);
}

/**
 * 生产级同步写入
 */
pub fn writeFileSync(path: string, content: string) {
    @fs_write_file(path, content);
}

/**
 * 目录操作
 */
pub fn mkdir(path: string) {
    @fs_mkdir(path);
}

pub fn exists(path: string): bool {
    return @fs_exists(path);
}

pub fn remove(path: string) {
    @fs_remove(path);
}

pub fn rmdir(path: string) {
    @fs_rmdir(path);
}

pub fn rename(oldPath: string, newPath: string) {
    @fs_rename(oldPath, newPath);
}

pub fn copyFile(src: string, dest: string) {
    @fs_copy_file(src, dest);
}

// 异步版
pub async fn removeAsync(path: string) {
    if (@is_nest) {
        await @yield_to_beam(@intFromEnum(SystemAction.FS_REMOVE), path);
    } else {
        @fs_remove(path);
    }
}

pub async fn rmdirAsync(path: string) {
    if (@is_nest) {
        await @yield_to_beam(@intFromEnum(SystemAction.FS_RMDIR), path);
    } else {
        @fs_rmdir(path);
    }
}

pub async fn renameAsync(oldPath: string, newPath: string) {
    if (@is_nest) {
        // Mock payload passing
        await @yield_to_beam(@intFromEnum(SystemAction.FS_RENAME), oldPath);
    } else {
        @fs_rename(oldPath, newPath);
    }
}

pub async fn copyFileAsync(src: string, dest: string) {
    if (@is_nest) {
        await @yield_to_beam(@intFromEnum(SystemAction.FS_COPY), src);
    } else {
        @fs_copy_file(src, dest);
    }
}

pub fn statSize(path: string): i32 {
    return @fs_stat_size(path);
}

pub fn isDirectory(path: string): bool {
    return @fs_is_dir(path);
}

let tmp_counter: std.atomic.Value(u64) = std.atomic.Value(u64).init(1);

pub const FileSystem = struct {
    pub fn tmpDir(): []const u8 {
        if (std.c.getenv("TMPDIR")) |p| return std.mem.span(p);
        if (std.c.getenv("TEMP")) |p| return std.mem.span(p);
        if (std.c.getenv("TMP")) |p| return std.mem.span(p);
        if (builtin.os.tag == .windows) return "C:\\Temp";
        return "/tmp";
    }

    pub fn tmpPath(prefix: []const u8, suffix: []const u8, dest: []u8): []const u8 {
        const dir = FileSystem.tmpDir();
        const id = tmp_counter.fetchAdd(1, .monotonic);
        let seed: u64 = id;
        if (builtin.os.tag == .linux) {
            let ts: std.os.linux.timespec = undefined;
            * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
            seed ^= @as(u64, @intCast(ts.sec)) * std.time.ns_per_s + @as(u64, @intCast(ts.nsec));
        }
        return std.fmt.bufPrint(dest, "{s}/{s}_{x}{s}", .{ dir, prefix, seed, suffix }) catch "";
    }

    pub fn writeAtomic(dest: string, content: string) {
        let tmp_buf: [1024]u8 = undefined;
        const dir = std.fs.path.dirname(dest) orelse FileSystem.tmpDir();
        const base = std.fs.path.basename(dest);
        const id = tmp_counter.fetchAdd(1, .monotonic);
        const tmp_name = std.fmt.bufPrint(&tmp_buf, "{s}/.{s}.tmp_{x}", .{ dir, base, id }) catch return;

        writeFileSync(tmp_name, content);
        rename(tmp_name, dest);
    }
};

pub const tmpDir = FileSystem.tmpDir;
pub const tmpPath = FileSystem.tmpPath;
pub const writeAtomic = FileSystem.writeAtomic;
