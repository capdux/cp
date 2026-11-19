// Source: /data/dev/bun/src/bun_alloc/MimallocArena.cp
// lib/mem/_mimalloc.cp
// Low-level Mimalloc C-FFI bindings and opaque heap handles (absorbed from bun/src/mimalloc_sys/mimalloc.cp)

pub @opaque struct MiHeap {}

pub @c {
    fn mi_malloc(size: usize): *!?;
    fn mi_calloc(count: usize, size: usize): *!?;
    fn mi_realloc(p: *!?, newsize: usize): *!?;
    fn mi_expand(p: *!?, newsize: usize): *!?;
    fn mi_free(p: *!?);
    fn mi_malloc_small(size: usize): *!?;
    fn mi_zalloc_small(size: usize): *!?;
    fn mi_zalloc(size: usize): *!?;
    fn mi_mallocn(count: usize, size: usize): *!?;
    fn mi_reallocn(p: *!?, count: usize, size: usize): *!?;
    fn mi_reallocf(p: *!?, newsize: usize): *!?;
    fn mi_usable_size(p: *!?): usize;
    fn mi_good_size(size: usize): usize;
    fn mi_collect(force: bool);
    fn mi_version(): i32;
    fn mi_stats_reset();
    fn mi_stats_merge();
    fn mi_process_init();
    fn mi_thread_init();
    fn mi_thread_done();
    fn mi_malloc_aligned(size: usize, alignment: usize): *!?;
    fn mi_zalloc_aligned(size: usize, alignment: usize): *!?;
    fn mi_calloc_aligned(count: usize, size: usize, alignment: usize): *!?;
    fn mi_realloc_aligned(p: *!?, newsize: usize, alignment: usize): *!?;

    fn mi_heap_new(): (*MiHeap!)?;
    fn mi_heap_delete(heap: *MiHeap!);
    fn mi_heap_destroy(heap: *MiHeap!);
    fn mi_heap_main(): *MiHeap!;
    fn mi_heap_contains(heap: *MiHeap!, p: *!?): bool;
    fn mi_heap_collect(heap: *MiHeap!, force: bool);
    fn mi_heap_malloc(heap: *MiHeap!, size: usize): *!?;
    fn mi_heap_zalloc(heap: *MiHeap!, size: usize): *!?;
    fn mi_heap_calloc(heap: *MiHeap!, count: usize, size: usize): *!?;
    fn mi_heap_realloc(heap: *MiHeap!, p: *!?, newsize: usize): *!?;
    fn mi_heap_malloc_aligned(heap: *MiHeap!, size: usize, alignment: usize): *!?;
}
