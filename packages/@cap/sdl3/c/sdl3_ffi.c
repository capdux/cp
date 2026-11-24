// @cap/sdl3 — Thin C ABI bridge for SDL3 (~35 lines)
#include <stdint.h>
#include <stdbool.h>
#include "SDL3/SDL_version.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_cpuinfo.h"
#include "SDL3/SDL_timer.h"

int32_t cp_sdl_version(void) {
    return (int32_t)SDL_GetVersion();
}

const char *cp_sdl_revision(void) {
    const char *rev = SDL_GetRevision();
    return rev ? rev : "";
}

int32_t cp_sdl_cpu_count(void) {
    return (int32_t)SDL_GetNumLogicalCPUCores();
}

int32_t cp_sdl_system_ram(void) {
    return (int32_t)SDL_GetSystemRAM();
}

int32_t cp_sdl_init_headless(void) {
    return SDL_Init(SDL_INIT_EVENTS) ? 1 : 0;
}

void cp_sdl_quit(void) {
    SDL_Quit();
}

uint64_t cp_sdl_ticks_ns(void) {
    return (uint64_t)SDL_GetTicksNS();
}
