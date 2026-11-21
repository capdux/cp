// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/NewWriter.cp, /data/dev/bun/src/sql/mysql/protocol/NewReader.cp
import pkt from "./packet.cp";

export const PACKET_HEADER_SIZE = pkt.PACKET_HEADER_SIZE;
export const MAX_PACKET_PAYLOAD = pkt.MAX_PACKET_PAYLOAD;
export const MySqlPacketError = pkt.MySqlPacketError;
export const PacketHeader = pkt.PacketHeader;
export const DecodedLenInt = pkt.DecodedLenInt;
export const decodeLenInt = pkt.decodeLenInt;
export const MySqlPacketReader = pkt.MySqlPacketReader;
export const PacketHandle = pkt.PacketHandle;
export const MySqlPacketWriter = pkt.MySqlPacketWriter;
