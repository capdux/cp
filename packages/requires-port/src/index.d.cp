// Type definitions for CP

export fn rptToLower(b: u8): u8;
export fn rptEqLower(a: string, target: string): bool;
export fn rptNormalizeProtocol(proto: string): string;
export fn defaultPortForProtocol(protocol: string): u16;
export fn requiresPort(port: u16, protocol: string): bool;
