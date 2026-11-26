// Type definitions for CP

export class MysqlConfig {
    tls: bool,
    host: string,
    port: u16,
    user: string,
    password: string,
    database: string,
    charset: string,
    valid: bool,
}

export fn eqBytes(a: string, b: string): bool;
export fn startsWith(s: string, prefix: string): bool;
export fn parsePort(s: string): i32;
export fn indexOf(s: string, ch: u8): i64;
export fn lastIndexOf(s: string, ch: u8): i64;
export fn applyQueryParams(query: string, cfg: &mut MysqlConfig);
export fn splitHostPort(hp: string, out_host: &mut string, out_port: &mut u16): bool;
export fn parseUri(uri: string): MysqlConfig;
export fn parseGoDsn(dsn: string): MysqlConfig;
export fn parse(input: string): MysqlConfig;
export fn appendStr(dst: []u8, pos: usize, src: string): usize;
export fn appendU16(dst: []u8, pos: usize, val: u16): usize;
export fn formatUri(cfg: MysqlConfig, redact_password: bool, out: []u8): usize;
