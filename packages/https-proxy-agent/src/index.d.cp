// Type definitions for CP

export class ProxyEndpoint {
    scheme: string,
    host: string,
    port: u16,
    username: string,
    password: string,
    has_auth: bool,
    is_tls: bool,
    valid: bool,
}

export fn hpaStrEq(a: string, b: string): bool;
export fn hpaFindByte(s: string, ch: u8, start_pos: usize): usize?;
export fn hpaParseU16(s: string): u16?;
export fn parseProxyUrl(url: string): ProxyEndpoint;
export fn hpaWriteStr(s: string, out: []u8, pos: usize): usize;
export fn hpaWriteU16(val: u16, out: []u8, pos: usize): usize;
export fn hpaWriteBase64(input: &[]u8, out: []u8, pos: usize): usize;
export fn buildConnectRequest(target_host: string, target_port: u16, proxy: ProxyEndpoint, out: []u8): usize;
