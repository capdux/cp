// Type definitions for CP

import { eqIgnoreCase, findByte } from "cap:ascii";
export fn rptNormalizeProtocol(proto: string): string;
export fn defaultPortForProtocol(protocol: string): u16;
export fn requiresPort(port: u16, protocol: string): bool;
