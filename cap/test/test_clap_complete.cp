// cap/test/test_clap_complete.cp
// Comprehensive end-to-end verification of cap:clap standard library
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Names,
    Values,
    Param,
    Help,
    ParamHelp,
    Diagnostic,
    ParseOptions,
    parseParam,
    parseEx,
    usage,
    help,
    SliceIterator,
    ShellIterator,
    OsIterator,
    StreamingClap,
    ComptimeClap,
} from "cap:clap";

fn test_slice_iterator(): !void {
    const raw_args: [5][]const u8 = ["bun", "build", "./src/index.ts", "--target=bun", "--minify"];
    let iter = SliceIterator.init(&raw_args);

    const a0 = iter.next();
    expect_eq(std.mem.eql(u8, a0.?, "bun"), true);

    const a1 = iter.next();
    expect_eq(std.mem.eql(u8, a1.?, "build"), true);

    const a2 = iter.next();
    expect_eq(std.mem.eql(u8, a2.?, "./src/index.ts"), true);

    const a3 = iter.next();
    expect_eq(std.mem.eql(u8, a3.?, "--target=bun"), true);

    const a4 = iter.next();
    expect_eq(std.mem.eql(u8, a4.?, "--minify"), true);

    const a5 = iter.next();
    expect_eq(a5 == null, true);
}

fn test_shell_iterator(): !void {
    let allocator = std.heap.page_allocator;
    const cmd = "cp run \"hello world\" 'single quote' --flag=test\\ arg";
    let iter = ShellIterator.init(allocator, cmd);
    defer iter.deinit();

    const t0 = (iter.next() catch unreachable).?;
    expect_eq(std.mem.eql(u8, t0, "cp"), true);

    const t1 = (iter.next() catch unreachable).?;
    expect_eq(std.mem.eql(u8, t1, "run"), true);

    const t2 = (iter.next() catch unreachable).?;
    expect_eq(std.mem.eql(u8, t2, "hello world"), true);

    const t3 = (iter.next() catch unreachable).?;
    expect_eq(std.mem.eql(u8, t3, "single quote"), true);

    const t4 = (iter.next() catch unreachable).?;
    expect_eq(std.mem.eql(u8, t4, "--flag=test arg"), true);

    const t5 = iter.next() catch unreachable;
    expect_eq(t5 == null, true);

    // Test syntax error on unclosed quote
    let bad_iter = ShellIterator.init(allocator, "echo \"unclosed quote");
    defer bad_iter.deinit();
    * = bad_iter.next() catch null; // "echo"
    const err_res = bad_iter.next() catch null;
    expect_eq(err_res == null, true);
}

fn test_parse_param(): !void {
    // 1. Short + Long with value
    const p1 = parseParam("-s, --size <num> Buffer size in bytes") catch unreachable;
    expect_eq(p1.names.short.?, 's');
    expect_eq(std.mem.eql(u8, p1.names.long.?, "size"), true);
    expect_eq(p1.takes_value == Values.one, true);
    expect_eq(std.mem.eql(u8, p1.id.value, "num"), true);
    expect_eq(std.mem.eql(u8, p1.id.msg, "Buffer size in bytes"), true);

    // 2. Short + Long with many values
    const p2 = parseParam("-i, --include <dir>... Include search paths") catch unreachable;
    expect_eq(p2.names.short.?, 'i');
    expect_eq(std.mem.eql(u8, p2.names.long.?, "include"), true);
    expect_eq(p2.takes_value == Values.many, true);
    expect_eq(std.mem.eql(u8, p2.id.value, "dir"), true);

    // 3. Long only flag (takes no value)
    const p3 = parseParam("--verbose Enable detailed debug logging") catch unreachable;
    expect_eq(p3.names.short == null, true);
    expect_eq(std.mem.eql(u8, p3.names.long.?, "verbose"), true);
    expect_eq(p3.takes_value == Values.none, true);

    // 4. Short only with optional value
    const p4 = parseParam("-p <port>? Listen port number") catch unreachable;
    expect_eq(p4.names.short.?, 'p');
    expect_eq(p4.takes_value == Values.one_optional, true);

    // 5. Positional parameter
    const p5 = parseParam("<input> Target input file") catch unreachable;
    expect_eq(p5.names.short == null, true);
    expect_eq(p5.names.long == null, true);
    expect_eq(p5.takes_value == Values.one, true);
    expect_eq(std.mem.eql(u8, p5.id.value, "input"), true);
}

fn test_streaming_clap(): !void {
    const p0 = (parseParam("-h, --help Show help") catch unreachable);
    const p1 = (parseParam("-o, --output <val> Output file") catch unreachable);
    const p2 = (parseParam("-v, --verbose Enable verbose") catch unreachable);
    const p3 = (parseParam("<input> Input file") catch unreachable);
    const params: []const ParamHelp = [p0, p1, p2, p3];

    const input_args: [5][]const u8 = ["-h", "-o", "out.bin", "-v", "source.cp"];
    let iter = SliceIterator.init(&input_args);
    const StreamType = StreamingClap(Help, SliceIterator);
    let stream = StreamType{
        params: &params,
        iter: &iter,
    };

    // 1st arg: -h
    const a1 = (stream.next() catch unreachable).?;
    expect_eq(a1.param.names.short.?, 'h');
    expect_eq(a1.value == null, true);

    // 2nd arg: -o out.bin
    const a2 = (stream.next() catch unreachable).?;
    expect_eq(a2.param.names.short.?, 'o');
    expect_eq(std.mem.eql(u8, a2.value.?, "out.bin"), true);

    // 3rd arg: -v
    const a3 = (stream.next() catch unreachable).?;
    expect_eq(a3.param.names.short.?, 'v');
    expect_eq(a3.value == null, true);

    // 4th arg: source.cp (positional)
    const a4 = (stream.next() catch unreachable).?;
    expect_eq(a4.param.names.short == null, true);
    expect_eq(a4.param.names.long == null, true);
    expect_eq(std.mem.eql(u8, a4.value.?, "source.cp"), true);

    // End of stream
    const a5 = stream.next() catch unreachable;
    expect_eq(a5 == null, true);
}

pub struct BufferWriter {
    buf: [1024]u8 = [0; 1024],
    len: usize = 0,

    pub fn writeByte(*_, b: u8): !void {
        if (_.len < _.buf.len) {
            _.buf[_.len] = b;
            _.len += 1;
        }
    }

    pub fn writeAll(*_, bytes: []const u8): !void {
        let i: usize = 0;
        while (i < bytes.len and _.len < _.buf.len) : (i += 1) {
            _.buf[_.len] = bytes[i];
            _.len += 1;
        }
    }

    pub fn getWritten(*_): []const u8 {
        return _.buf[0.._.len];
    }
}

const cp_p1 = (parseParam("-h, --help Show help instructions") catch unreachable);
const cp_p2 = (parseParam("-v, --verbose Enable verbose logs") catch unreachable);
const cp_p3 = (parseParam("-o, --out <file> Output artifact file") catch unreachable);
const cp_p4 = (parseParam("-I, --include <dir>... Include search paths") catch unreachable);
const cp_p5 = (parseParam("<target> Main compilation target") catch unreachable);
const comptime_params: [5]ParamHelp = [cp_p1, cp_p2, cp_p3, cp_p4, cp_p5];

fn test_comptime_clap_parsing(): !void {
    const cli_input: [8][]const u8 = [
        "--verbose",
        "-o", "build/out",
        "-I", "/usr/include",
        "-I", "/data/dev/include",
        "main.cp",
    ];

    let iter = SliceIterator.init(&cli_input);
    let opt = ParseOptions{
        allocator: std.heap.page_allocator,
    };

    const ClapType = ComptimeClap(Help, &comptime_params);
    let parsed = ClapType.parse(&iter, opt) catch unreachable;
    defer parsed.deinit();

    // Verify boolean flags
    expect_eq(parsed.flag("--verbose"), true);
    expect_eq(parsed.flag("-v"), true);
    expect_eq(parsed.flag("--help"), false);
    expect_eq(parsed.flag("-h"), false);

    // Verify single option
    const out_file = parsed.option("--out");
    expect_eq(out_file != null, true);
    expect_eq(std.mem.eql(u8, out_file.?, "build/out"), true);

    // Verify multi options
    const includes = parsed.options("--include");
    expect_eq(includes.len as i64, 2);
    expect_eq(std.mem.eql(u8, includes[0], "/usr/include"), true);
    expect_eq(std.mem.eql(u8, includes[1], "/data/dev/include"), true);

    // Verify positionals
    const pos = parsed.positionals();
    expect_eq(pos.len as i64, 1);
    expect_eq(std.mem.eql(u8, pos[0], "main.cp"), true);
}

fn test_help_and_usage(): !void {
    const p1 = (parseParam("-h, --help Print this help message") catch unreachable);
    const p2 = (parseParam("-c, --config <path> Path to configuration file") catch unreachable);
    const p3 = (parseParam("<entry> Entrypoint script") catch unreachable);
    const params: [3]ParamHelp = [p1, p2, p3];

    let stream = BufferWriter{};

    usage(&stream, &params) catch unreachable;
    const usage_str = stream.getWritten();
    expect_eq(std.mem.indexOf(u8, usage_str, "[-h]") != null, true);
    expect_eq(std.mem.indexOf(u8, usage_str, "[-c <path>]") != null, true);
    expect_eq(std.mem.indexOf(u8, usage_str, "<entry>") != null, true);
}

fn main() {
    test("1. cap:clap — SliceIterator argument iteration", () => {
        test_slice_iterator()?;
    });

    test("2. cap:clap — ShellIterator shell string lexical tokenization", () => {
        test_shell_iterator()?;
    });

    test("3. cap:clap — parseParam declaration syntax decoding", () => {
        test_parse_param()?;
    });

    test("4. cap:clap — StreamingClap stateful streaming parser", () => {
        test_streaming_clap()?;
    });

    test("5. cap:clap — ComptimeClap strongly-typed CLI arguments extraction", () => {
        test_comptime_clap_parsing()?;
    });

    test("6. cap:clap — help & usage generation formatting", () => {
        test_help_and_usage()?;
    });
}
