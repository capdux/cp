// Source: Absorbed from /data/dev/bun/src/io/io.cp | Synced: 2026-11-22T01:52:38Z
/**
 * CP Standard I/O (cap:io)
 * 工业级统一高性能 I/O 流体系、非阻塞管道/文件流与标准终端
 */

export {
    IOError,
    SeekOrigin,
    Reader,
    Writer,
    Closer,
    Seeker
} from "./traits";

export {
    IoStatus,
    ReadState,
    WriteStatus,
    PollState,
    FileType,
    PollResult,
    IoSource
} from "./source";

export {
    BufferReader,
    BufferWriter,
    BufferedReader,
    BufferedWriter,
    LimitedReader,
    MultiReader,
    TeeReader,
    DiscardWriter,
    CountingWriter,
    ChunkedBuffer,
    copyStream,
    copyN
} from "./buffer_stream";

export {
    Console,
    console,
    Stdio,
    stdio,
    print,
    println,
    eprint,
    eprintln
} from "./stdio";

export {
    MaxBuf,
    MaxBufKind
} from "./max_buf";

export {
    PipeReadResult,
    PipeWriteResult,
    PipeReader,
    PipeWriter,
    Pipe
} from "./pipe";

export {
    FileReader,
    FileWriter
} from "./file_stream";
