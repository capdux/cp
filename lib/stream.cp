// Source: Absorbed from /data/dev/bun/src/io/pipes.cp | Synced: 2026-11-20T09:23:15Z
/**
 * CP Standard Stream Library (cap:stream)
 * 工业级统一流式 Trait 契约与管道体系
 */

export {
    IOError,
    SeekOrigin,
    Reader,
    Writer,
    Closer,
    Seeker,
    BufferReader,
    BufferWriter,
    BufferedReader,
    BufferedWriter,
    copyStream,
    copyN,
    MaxBuf,
    MaxBufKind,
    PipeReader,
    PipeWriter,
    Pipe
} from "./io";
