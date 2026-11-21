// Source: Absorbed from /data/dev/bun/src/io/source.cp | Synced: 2026-11-20T17:36:08Z
/**
 * CP Standard Console and Stdio (cap:io)
 * 标准控制台与终端流输入输出
 */

pub class Console {
    pub fn log(fmt: string, a1: auto, a2: auto) {
        if (@is_nest) {
            @print(fmt, a1, a2);
        } else {
            @io_write_stdout(fmt, a1, a2);
        }
    }

    pub fn err(fmt: string, a1: auto) {
        if (@is_nest) {
            @print_err(fmt, a1);
        } else {
            @io_write_stderr(fmt, a1);
        }
    }
}

pub const console = new Console();

pub class Stdio {
    pub fn readLine(): string {
        if (@is_nest) {
            return @yield_to_beam(90, "read_line"); 
        } else {
            return @io_read_stdin_line();
        }
    }
}

pub const stdio = new Stdio();

pub fn print(msg: auto) {
    @print("{any}", msg);
}

pub fn println(msg: auto) {
    @print("{any}\n", msg);
}

pub fn eprint(msg: auto) {
    @print_err("{any}", msg);
}

pub fn eprintln(msg: auto) {
    @print_err("{any}\n", msg);
}
