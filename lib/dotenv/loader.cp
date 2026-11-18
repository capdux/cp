// Source: bun/src/dotenv/env_loader.cp
import std from "std";
import { EnvMap, EnvParser, expandString } from "./parser.cp";

pub struct DotenvOptions {
    pub path: []const u8 = "",
    pub mode: []const u8 = "development",
    pub cwd: []const u8 = ".",
    pub override: bool = false,
    pub expand: bool = true,
}

pub struct DotenvLoader {
    pub map: EnvMap = EnvMap.init(),
    pub loaded_files: [8][]const u8 = undefined,
    pub loaded_files_count: usize = 0,

    pub static fn init(): DotenvLoader {
        return DotenvLoader{
            map: EnvMap.init(),
            loaded_files: undefined,
            loaded_files_count: 0,
        };
    }

    pub fn get(key: []const u8): ([]const u8)? {
        return _.map.get(key);
    }

    pub fn getOr(key: []const u8, default_val: []const u8): []const u8 {
        return _.map.getOr(key, default_val);
    }

    pub fn has(key: []const u8): bool {
        return _.map.has(key);
    }

    pub fn count(): usize {
        return _.map.count;
    }

    pub fn loadFromString(*_, content: []const u8, overwrite: bool, expand: bool) {
        let parser = EnvParser.init(content);
        parser.parseAll(&_.map, overwrite, expand);
    }

    pub fn loadFile(*_, file_path: []const u8, overwrite: bool, expand: bool): bool {
        let fd = std.posix.openat(std.posix.AT.FDCWD, file_path, .{ .ACCMODE = .RDONLY }, 0) catch return false;
        defer * = std.c.close(fd);

        let file_buf: [16384]u8 = undefined;
        let bytes_read = std.posix.read(fd, &file_buf) catch return false;
        let content = file_buf[0..bytes_read];

        _.loadFromString(content, overwrite, expand);
        if (_.loaded_files_count < 8) {
            let stored_path = _.map.storeString(file_path);
            _.loaded_files[_.loaded_files_count] = stored_path;
            _.loaded_files_count += 1;
        }
        return true;
    }

    pub fn loadCascade(*_, cwd: []const u8, mode: []const u8, overwrite: bool, expand: bool) {
        let path_buf: [256]u8 = undefined;

        // 1. .env.<mode>.local (highest priority)
        let p1 = formatCascadePath(cwd, ".env.", mode, ".local", &path_buf);
        * = _.loadFile(p1, overwrite, expand);

        // 2. .env.local (if mode != "test")
        if (!std.mem.eql(u8, mode, "test")) {
            let p2 = formatCascadePath(cwd, ".env.local", "", "", &path_buf);
            * = _.loadFile(p2, overwrite, expand);
        }

        // 3. .env.<mode>
        let p3 = formatCascadePath(cwd, ".env.", mode, "", &path_buf);
        * = _.loadFile(p3, overwrite, expand);

        // 4. .env (base fallback)
        let p4 = formatCascadePath(cwd, ".env", "", "", &path_buf);
        * = _.loadFile(p4, overwrite, expand);
    }
}

pub fn formatCascadePath(cwd: []const u8, p1: []const u8, p2: []const u8, p3: []const u8, out_buf: []u8): []const u8 {
    let out_len: usize = 0;
    if (cwd.len > 0 and !std.mem.eql(u8, cwd, ".")) {
        let i: usize = 0;
        while (i < cwd.len and out_len < out_buf.len) {
            out_buf[out_len] = cwd[i];
            out_len += 1;
            i += 1;
        }
        if (out_len < out_buf.len and out_buf[out_len - 1] != '/') {
            out_buf[out_len] = '/';
            out_len += 1;
        }
    }
    let s: usize = 0;
    while (s < p1.len and out_len < out_buf.len) {
        out_buf[out_len] = p1[s];
        out_len += 1;
        s += 1;
    }
    s = 0;
    while (s < p2.len and out_len < out_buf.len) {
        out_buf[out_len] = p2[s];
        out_len += 1;
        s += 1;
    }
    s = 0;
    while (s < p3.len and out_len < out_buf.len) {
        out_buf[out_len] = p3[s];
        out_len += 1;
        s += 1;
    }
    return out_buf[0..out_len];
}

pub fn config(options: DotenvOptions): DotenvLoader {
    let loader = DotenvLoader.init();
    if (options.path.len > 0) {
        * = loader.loadFile(options.path, options.override, options.expand);
    } else {
        loader.loadCascade(options.cwd, options.mode, options.override, options.expand);
    }
    return loader;
}

pub fn load(mode: []const u8): DotenvLoader {
    return config(DotenvOptions{
        path: "",
        mode: mode,
        cwd: ".",
        override: false,
        expand: true,
    });
}
