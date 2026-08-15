// disk_manager.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include "disk_manager.h"

static DiskSlot g_disks[MAX_DISKS];

// -----------------------------------------------------------------------------
// Logging shim (swap to your log_printf if you prefer)
// -----------------------------------------------------------------------------
#ifndef DM_LOGF
#define DM_LOGF(...) do { printf(__VA_ARGS__); } while (0)
#endif

// -----------------------------------------------------------------------------
// File helper: read entire file into memory
// -----------------------------------------------------------------------------
static uint8_t* dm_read_entire_file(const char *path, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz <= 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }

    uint8_t *buf = (uint8_t*)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f); free(buf); return NULL;
    }
    fclose(f);
    if (out_size) *out_size = (size_t)sz;
    return buf;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
void disk_init(void) {
    memset(g_disks, 0, sizeof(g_disks));
}

static bool slot_ok(int slot) {
    return (slot >= 0 && slot < MAX_DISKS);
}

bool disk_attach(int slot, const char *path, bool readonly) {
    if (!slot_ok(slot) || !path) return false;

    size_t sz = 0;
    uint8_t *img = dm_read_entire_file(path, &sz);
    if (!img || sz < SECTOR_SIZE) {
        if (img) free(img);
        DM_LOGF("[DISK] attach disk%d failed: cannot read %s\n", slot, path);
        return false;
    }

    if (g_disks[slot].present && g_disks[slot].img) {
        free(g_disks[slot].img);
    }

    memset(&g_disks[slot], 0, sizeof(g_disks[slot]));
    g_disks[slot].present    = true;
    g_disks[slot].readonly   = readonly;
    g_disks[slot].img        = img;
    g_disks[slot].size_bytes = sz;
    strncpy(g_disks[slot].path, path, sizeof(g_disks[slot].path)-1);

    DM_LOGF("[DISK] disk%d attached: %s (%zu bytes)%s\n",
            slot, path, sz, readonly ? " [RO]" : "");
    return true;
}

bool disk_detach(int slot) {
    if (!slot_ok(slot)) return false;
    if (!g_disks[slot].present) return true;
    if (g_disks[slot].img) free(g_disks[slot].img);
    memset(&g_disks[slot], 0, sizeof(g_disks[slot]));
    DM_LOGF("[DISK] disk%d detached\n", slot);
    return true;
}

bool disk_present(int slot) {
    return slot_ok(slot) && g_disks[slot].present;
}

size_t disk_size_bytes(int slot) {
    if (!slot_ok(slot) || !g_disks[slot].present) return 0;
    return g_disks[slot].size_bytes;
}

size_t disk_num_sectors(int slot) {
    return disk_size_bytes(slot) / SECTOR_SIZE;
}

bool disk_read_sectors(int slot, uint64_t lba, void *dst, uint32_t nsec) {
    if (!slot_ok(slot) || !g_disks[slot].present || !dst) return false;
    size_t off = (size_t)(lba * SECTOR_SIZE);
    size_t len = (size_t)nsec * SECTOR_SIZE;
    if (off + len > g_disks[slot].size_bytes) return false;
    memcpy(dst, g_disks[slot].img + off, len);
    return true;
}

bool disk_write_sectors(int slot, uint64_t lba, const void *src, uint32_t nsec) {
    if (!slot_ok(slot) || !g_disks[slot].present || !src) return false;
    if (g_disks[slot].readonly) return false;
    size_t off = (size_t)(lba * SECTOR_SIZE);
    size_t len = (size_t)nsec * SECTOR_SIZE;
    if (off + len > g_disks[slot].size_bytes) return false;
    memcpy(g_disks[slot].img + off, src, len);
    return true;
}

const DiskSlot *disk_get_slot(int slot) {
    if (!slot_ok(slot)) return NULL;
    return &g_disks[slot];
}

// -----------------------------------------------------------------------------
// Partition + FS probe (lightweight): MBR, GPT, ext superblock
// -----------------------------------------------------------------------------
#if defined(_MSC_VER)
#pragma pack(push,1)
#endif
typedef struct {
    uint8_t  status;
    uint8_t  chs_first[3];
    uint8_t  type;       // 0x83 = Linux
    uint8_t  chs_last[3];
    uint32_t lba_first;
    uint32_t sectors;
}
#if defined(__GNUC__)
__attribute__((packed))
#endif
mbr_entry_t;

typedef struct {
    uint8_t  boot[446];
    mbr_entry_t part[4];
    uint16_t sig; // 0xAA55 (on disk little-endian)
}
#if defined(__GNUC__)
__attribute__((packed))
#endif
mbr_t;

typedef struct {
    uint64_t signature;         // "EFI PART" = 0x5452415020494645
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t part_entries_lba;
    uint32_t num_part_entries;
    uint32_t size_part_entry;   // typically 128
    uint32_t part_array_crc32;
    // rest ignored
}
#if defined(__GNUC__)
__attribute__((packed))
#endif
gpt_header_t;

typedef struct {
    uint8_t  type_guid[16];
    uint8_t  unique_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attrs;
    uint16_t name_utf16[36]; // UTF-16LE
}
#if defined(__GNUC__)
__attribute__((packed))
#endif
gpt_entry_t;
#if defined(_MSC_VER)
#pragma pack(pop)
#endif

static const uint64_t GPT_SIG = 0x5452415020494645ULL; // "EFI PART"
// Linux filesystem GUID on disk (little-endian fields already)
static const uint8_t LINUX_FS_GUID[16] = {
    0xAF,0x3D,0xC6,0x0F, 0x83,0x84, 0x72,0x47, 0x8E,0x79, 0x3D,0x69,0xD8,0x47,0x7D,0xE4
};

static inline bool guid_eq(const uint8_t a[16], const uint8_t b[16]) {
    for (int i=0;i<16;i++) if (a[i]!=b[i]) return false;
    return true;
}

static void u16le_to_ascii_packed(const void *src_bytes, int max_u16, char *dst, int dstsz) {
    const uint8_t *p = (const uint8_t*)src_bytes;
    int j = 0;
    for (int i = 0; i < max_u16 && j < dstsz - 1; i++) {
        uint16_t w = (uint16_t)p[2*i] | ((uint16_t)p[2*i + 1] << 8);
        if (w == 0) break;                // stop at NUL terminator
        dst[j++] = (w <= 0x7F) ? (char)w : '?';
    }
    dst[j] = '\0';
}

static bool probe_mbr(const uint8_t *img, size_t sz) {
    if (sz < SECTOR_SIZE) return false;
    const mbr_t *m = (const mbr_t*)img;
    return (m->sig == 0xAA55);
}

static bool probe_gpt_header(const uint8_t *img, size_t sz, gpt_header_t *out) {
    if (sz < SECTOR_SIZE * 2) return false;
    memcpy(out, img + SECTOR_SIZE, sizeof(gpt_header_t));
    return (out->signature == GPT_SIG && out->num_part_entries > 0 && out->size_part_entry >= sizeof(gpt_entry_t));
}

static void print_mbr(const uint8_t *img) {
    const mbr_t *m = (const mbr_t*)img;
    DM_LOGF("partitioning: MBR (sig=0x%04x)\n", m->sig);
    for (int i=0;i<4;i++) {
        const mbr_entry_t *e = &m->part[i];
        if (e->lba_first == 0 || e->sectors == 0) continue;
        int boot = (e->status == 0x80) ? 1 : 0;
        DM_LOGF("  p%d: type=0x%02x boot=%d start=%u size=%u\n",
            i+1, e->type, boot, e->lba_first, e->sectors);
    }
}

static inline int guid_is_all_zero(const uint8_t g[16]) {
    for (int i = 0; i < 16; i++) if (g[i]) return 0;
    return 1;
}

static void guid_to_string(const uint8_t g[16], char out[37]) {
    // GPT/UEFI GUID layout is mixed-endian:
    // - d1 (4B), d2 (2B), d3 (2B) are LE
    // - the last 8 bytes are BE-ish (just byte order as stored)
    uint32_t d1 = (uint32_t)g[0] | ((uint32_t)g[1] << 8) |
                  ((uint32_t)g[2] << 16) | ((uint32_t)g[3] << 24);
    uint16_t d2 = (uint16_t)g[4] | ((uint16_t)g[5] << 8);
    uint16_t d3 = (uint16_t)g[6] | ((uint16_t)g[7] << 8);
    // g[8]..g[15] print in the order stored
    // 8-4-4-4-12
    // 00000000-0000-0000-0000-000000000000
    sprintf(out,
            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            d1, d2, d3,
            g[8], g[9],
            g[10], g[11], g[12], g[13], g[14], g[15]);
}

// Print a concise GPT partition table summary from a whole-disk image.
static void print_gpt(const uint8_t *img, size_t sz)
{
    gpt_header_t hdr;
    if (!probe_gpt_header(img, sz, &hdr)) {
        DM_LOGF("partitioning: GPT (invalid/unsupported)\n");
        return;
    }

    DM_LOGF("partitioning: GPT\n");
    DM_LOGF("  entries @ LBA %" PRIu64 ", count=%u, entry_size=%u\n",
            hdr.part_entries_lba, hdr.num_part_entries, hdr.size_part_entry);

    const uint32_t esz = hdr.size_part_entry;                  // usually 128
    if (esz == 0) {
        DM_LOGF("  (bad entry size: 0)\n");
        return;
    }

    const uint32_t per_sec = SECTOR_SIZE / esz;                // entries per 512B sector
    if (per_sec == 0) {
        // Uncommon: very large entry size (>512). Keep code simple and bail.
        DM_LOGF("  (unsupported entry size: %u > sector size)\n", esz);
        return;
    }

    uint64_t lba       = hdr.part_entries_lba;
    uint32_t remaining = hdr.num_part_entries;
    uint32_t index     = 0;

    while (remaining && (lba * (uint64_t)SECTOR_SIZE) < sz) {
        // Bounds check for this sector
        const uint64_t sec_off = lba * (uint64_t)SECTOR_SIZE;
        if (sec_off + SECTOR_SIZE > sz) break;

        const uint8_t *sec  = img + (size_t)sec_off;
        const uint32_t take = (remaining > per_sec) ? per_sec : remaining;

        for (uint32_t i = 0; i < take; i++, index++) {
            const uint8_t     *entry_bytes = sec + (size_t)i * esz;
            const gpt_entry_t *e           = (const gpt_entry_t *)entry_bytes;

            // Skip completely empty entries (type GUID all-zero)
            if (guid_is_all_zero(e->type_guid))
                continue;

            // GUIDs
            char type_guid_str[37], uniq_guid_str[37];
            guid_to_string(e->type_guid,   type_guid_str);
            guid_to_string(e->unique_guid, uniq_guid_str);

            // Name: avoid &e->name_utf16 (packed-member address) – use byte view + offsetof
            char name[64];
            const uint8_t *name_bytes = entry_bytes + offsetof(gpt_entry_t, name_utf16);
            u16le_to_ascii_packed(name_bytes, 36, name, (int)sizeof(name));

            const uint64_t first = e->first_lba;
            const uint64_t last  = e->last_lba;
            const uint64_t secs  = (last >= first) ? (last - first + 1ULL) : 0ULL;

            DM_LOGF("  #%u  %-36s  %s\n", index, name, type_guid_str);
            DM_LOGF("      LBA: [%" PRIu64 " .. %" PRIu64 "]  (%" PRIu64 " sectors)  attrs=0x%016" PRIx64 "\n",
                    first, last, secs, e->attrs);
            DM_LOGF("      GUID: %s\n", uniq_guid_str);
        }

        remaining -= take;
        lba++;
    }
}

static void print_ext_probe_from_linux_parts_mbr(const uint8_t *img, size_t sz) {
    const mbr_t *m = (const mbr_t*)img;
    for (int i=0;i<4;i++) {
        const mbr_entry_t *e = &m->part[i];
        if (e->type != 0x83 || e->lba_first == 0 || e->sectors == 0) continue;
        uint64_t fs_byte = (uint64_t)e->lba_first * SECTOR_SIZE;
        if (fs_byte + 2048 > sz) continue;
        const uint8_t *sb = img + fs_byte + 1024; // superblock
        uint16_t magic = (uint16_t)sb[56] | ((uint16_t)sb[57] << 8);
        if (magic != 0xEF53) continue;
        uint32_t lsz = (uint32_t)sb[24] | ((uint32_t)sb[25]<<8) | ((uint32_t)sb[26]<<16) | ((uint32_t)sb[27]<<24);
        uint32_t block_sz = 1024u << lsz;
        char label[17]; memset(label,0,sizeof(label));
        memcpy(label, sb + 120, 16);
        DM_LOGF("fs probe (p%d): ext_magic=0x%04x  block=%u  label=\"%s\"\n",
                i+1, magic, block_sz, label);
        break; // first match only
    }
}

static void print_ext_probe_from_linux_parts_gpt(const uint8_t *img, size_t sz) {
    gpt_header_t h = {0};
    if (!probe_gpt_header(img, sz, &h)) return;
    const uint64_t ents_lba = h.part_entries_lba;
    const uint32_t ents_per_sector = SECTOR_SIZE / h.size_part_entry;
    uint32_t remaining = h.num_part_entries;
    uint64_t lba = ents_lba;

    while (remaining && (lba * SECTOR_SIZE) < sz) {
        if ((lba * SECTOR_SIZE) + SECTOR_SIZE > sz) break;
        const uint8_t *sec = img + (size_t)(lba * SECTOR_SIZE);
        uint32_t count = (remaining > ents_per_sector) ? ents_per_sector : remaining;
        for (uint32_t i=0;i<count; i++) {
            const gpt_entry_t *e = (const gpt_entry_t*)(sec + i * h.size_part_entry);
            if (e->first_lba == 0 && e->last_lba == 0) continue;
            if (!guid_eq(e->type_guid, LINUX_FS_GUID)) continue;
            uint64_t fs_byte = e->first_lba * SECTOR_SIZE;
            if (fs_byte + 2048 > sz) continue;
            const uint8_t *sb = img + (size_t)fs_byte + 1024;
            uint16_t magic = (uint16_t)sb[56] | ((uint16_t)sb[57] << 8);
            if (magic != 0xEF53) continue;
            uint32_t lsz = (uint32_t)sb[24] | ((uint32_t)sb[25]<<8) | ((uint32_t)sb[26]<<16) | ((uint32_t)sb[27]<<24);
            uint32_t block_sz = 1024u << lsz;
            char label[17]; memset(label,0,sizeof(label));
            memcpy(label, sb + 120, 16);
            DM_LOGF("fs probe (#%d): ext_magic=0x%04x  block=%u  label=\"%s\"\n",
                    i+1, magic, block_sz, label);
            return;
        }
        remaining -= count;
        lba++;
    }
}

// -----------------------------------------------------------------------------
// Presentation helpers
// -----------------------------------------------------------------------------
void disk_print_list(void) {
    for (int i=0;i<MAX_DISKS;i++) {
        const DiskSlot *d = &g_disks[i];
        if (!d->present) {
            DM_LOGF("disk%d: empty\n", i);
            continue;
        }
        size_t nsec = d->size_bytes / SECTOR_SIZE;
        DM_LOGF("disk%d: %s, %s, size=%zu bytes (%zu sec) path=%s\n",
                i, d->present?"present":"empty",
                d->readonly?"ro":"rw",
                d->size_bytes, nsec,
                d->path[0]? d->path : "(unnamed)");
    }
}

void disk_print_info(int slot) {
    if (!slot_ok(slot)) {
        DM_LOGF("disk%d: invalid slot\n", slot);
        return;
    }
    const DiskSlot *d = &g_disks[slot];
    if (!d->present) {
        DM_LOGF("disk%d: empty\n", slot);
        return;
    }
    size_t nsec = d->size_bytes / SECTOR_SIZE;
    DM_LOGF("disk%d: present, %s\n", slot, d->readonly ? "ro" : "rw");
    DM_LOGF("  path     : %s\n", d->path[0]? d->path : "(unnamed)");
    DM_LOGF("  size     : %zu bytes (%zu sectors)\n", d->size_bytes, nsec);
    DM_LOGF("  capacity : %zu LBA (512-byte sectors)\n", nsec);
    DM_LOGF("  flags    : present%s\n", d->readonly? ", readonly" : "");

    // Partitioning summary
    bool is_mbr = probe_mbr(d->img, d->size_bytes);
    gpt_header_t gh;
    bool is_gpt = probe_gpt_header(d->img, d->size_bytes, &gh);

    if (is_gpt) {
        print_gpt(d->img, d->size_bytes);
        print_ext_probe_from_linux_parts_gpt(d->img, d->size_bytes);
    } else if (is_mbr) {
        print_mbr(d->img);
        print_ext_probe_from_linux_parts_mbr(d->img, d->size_bytes);
    } else {
        DM_LOGF("partitioning: none/unknown\n");
    }
}