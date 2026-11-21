// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/HandshakeResponse41.cp
import hs from "./handshake.cp";

export const DEFAULT_MAX_PACKET_SIZE = hs.DEFAULT_MAX_PACKET_SIZE;
export const DEFAULT_CHARSET_UTF8MB4 = hs.DEFAULT_CHARSET_UTF8MB4;
export const CAP_LONG_PASSWORD = hs.CAP_LONG_PASSWORD;
export const CAP_CONNECT_WITH_DB = hs.CAP_CONNECT_WITH_DB;
export const CAP_PROTOCOL_41 = hs.CAP_PROTOCOL_41;
export const CAP_SECURE_CONNECTION = hs.CAP_SECURE_CONNECTION;
export const CAP_PLUGIN_AUTH = hs.CAP_PLUGIN_AUTH;
export const CAP_CONNECT_ATTRS = hs.CAP_CONNECT_ATTRS;
export const CAP_PLUGIN_AUTH_LENENC = hs.CAP_PLUGIN_AUTH_LENENC;
export const HandshakeError = hs.HandshakeError;
export const ConnectAttr = hs.ConnectAttr;
export const HandshakeResponse41 = hs.HandshakeResponse41;
export const EncodedHandshake = hs.EncodedHandshake;
export const DecodedHandshake41 = hs.DecodedHandshake41;
export const encodeHandshakeResponse41 = hs.encodeHandshakeResponse41;
export const decodeHandshakeResponse41 = hs.decodeHandshakeResponse41;
