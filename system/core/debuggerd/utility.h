

#ifndef __utility_h
#define __utility_h

#include <stddef.h>
#include <stdbool.h>

#ifndef PT_ARM_EXIDX
#define PT_ARM_EXIDX    0x70000001      /* .ARM.exidx segment */
#endif

#define STACK_CONTENT_DEPTH 32

typedef struct mapinfo {
    struct mapinfo *next;
    unsigned start;
    unsigned end;
    unsigned exidx_start;
    unsigned exidx_end;
    char name[];
} mapinfo;

/* Get a word from pid using ptrace. The result is the return value. */
extern int get_remote_word(int pid, void *src);

extern void get_remote_struct(int pid, void *src, void *dst, size_t size);

/* Find the containing map for the pc */
const mapinfo *pc_to_mapinfo (mapinfo *mi, unsigned pc, unsigned *rel_pc);

/* Map a pc address to the name of the containing ELF file */
const char *map_to_name(mapinfo *mi, unsigned pc, const char* def);

/* Log information onto the tombstone */
extern void _LOG(int tfd, bool in_tombstone_only, const char *fmt, ...);

#endif
