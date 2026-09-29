/* archive.c - see archive.h; the same steps as tools/illfiles.py */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "archive.h"
#include "sys.h"

#define LIMIT 0x1F3F            /* the LZW table's size; full: start again */

static uint16_t sw(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t sd(const uint8_t *p) { return p[0] | p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

int arc_open(Archive *a, const char *path, char *err, size_t n)
{
    size_t p, end, last, pages;
    int i;

    memset(a, 0, sizeof *a);
    a->data = sys_load(path, &a->len);
    if (!a->data) {
        snprintf(err, n, "%s cannot be read.", path);
        return -1;
    }
    if (a->len < 0x20 || a->data[0] != 'M' || a->data[1] != 'Z')
        goto bad;
    last = sw(a->data + 2);
    pages = sw(a->data + 4);
    a->base = pages * 512 - (last ? 512 - last : 0);
    if (a->base + 4 > a->len)
        goto bad;
    a->count = sw(a->data + a->base);
    p = a->base + 4;
    end = p + sw(a->data + a->base + 2);
    if (end > a->len)
        goto bad;
    a->entries = calloc((size_t)a->count, sizeof *a->entries);
    if (!a->entries)
        goto bad;
    for (i = 0; i < a->count; i++) {
        ArcEntry *e = &a->entries[i];
        size_t k = 0;
        /* the name's bytes are the clear text rotated left by 3 */
        for (; p < end && a->data[p]; p++) {
            uint8_t b = a->data[p];
            if (k + 1 < sizeof e->name)
                e->name[k++] = (char)((b >> 3 | b << 5) & 0xFF);
        }
        if (p + 13 > end)
            goto bad;
        e->off = sd(a->data + p + 1);
        e->size = sd(a->data + p + 5);
        e->packed = sd(a->data + p + 9);
        p += 13;
        if (a->base + (size_t)e->off + 4 + e->packed > a->len)
            goto bad;
    }
    if (p != end)
        goto bad;
    return 0;
bad:
    snprintf(err, n, "%s: its resource archive is not as expected.", path);
    arc_close(a);
    return -1;
}

void arc_close(Archive *a)
{
    free(a->data);
    free(a->entries);
    memset(a, 0, sizeof *a);
}

const ArcEntry *arc_find(const Archive *a, const char *name)
{
    int i;
    for (i = 0; i < a->count; i++) {
        const char *x = a->entries[i].name, *y = name;
        for (; *x && *y; x++, y++) {
            char cx = *x == '/' ? '\\' : (char)toupper((unsigned char)*x);
            char cy = *y == '/' ? '\\' : (char)toupper((unsigned char)*y);
            if (cx != cy)
                break;
        }
        if (!*x && !*y)
            return &a->entries[i];
    }
    return NULL;
}

/* ---- NLZW: a uniform arithmetic coder whose alphabet grows by one a
 * symbol, carrying LZW codes */

typedef struct {
    const uint8_t *data;
    size_t len, pos;
    uint32_t low, high, total, code, bits;
    int bit_count, pending;
} Dec;

static uint32_t word(Dec *d)
{
    size_t p = d->pos;
    d->pos += 2;
    if (p + 2 > d->len)
        return 0;               /* the DOS decoder reads padding past the end */
    return (uint32_t)(d->data[p] << 8 | d->data[p + 1]);
}

static void refill(Dec *d)
{
    d->bit_count -= 16;
    d->bits |= word(d) << d->bit_count;      /* bit_count 0..16 here */
}

static void shift(Dec *d, int n)
{
    if (n == 0)
        return;
    if (n >= 32) {
        /* a whole word: when high and low agree in all their bits */
        d->code = d->bits;
        d->bits = 0;
        d->high = 0xFFFFFFFFu;
        d->low = 0;
        return;
    }
    d->code = d->code << n | d->bits >> (32 - n);
    d->bits <<= n;
    d->high = d->high << n | 0xFFFFFFFFu >> (32 - n);
    d->low <<= n;
}

static void consume(Dec *d, int n)
{
    if (d->bit_count + n >= 32) {
        int first = 32 - d->bit_count;
        shift(d, first);
        d->bit_count = 32;
        refill(d);
        refill(d);
        d->bit_count = n - first;
        shift(d, n - first);
    } else {
        d->bit_count += n;
        shift(d, n);
    }
    if (d->bit_count >= 16)
        refill(d);
}

static int top_bit(uint32_t v)
{
    int b = -1;
    while (v) {
        b++;
        v >>= 1;
    }
    return b;
}

static void normalize(Dec *d)
{
    for (;;) {
        int equal = 31 - top_bit(d->high ^ d->low), top, k;
        if (equal > 32)
            equal = 32;
        if (equal) {
            consume(d, equal);
            return;
        }
        if (d->pending) {
            consume(d, 1);
            d->pending = 0;
            continue;
        }
        top = top_bit(~d->low << 1);
        k = top_bit(d->high << 1);
        if (k > top)
            top = k;
        if (top < 0)
            top = 0;
        k = 31 - top;
        if (k == 0)
            return;
        d->pending += k;
        consume(d, k);
        d->code ^= 0x80000000u;
        d->high ^= 0x80000000u;
        d->low ^= 0x80000000u;
        return;
    }
}

/* the next symbol, or -1 if out of range */
static int symbol(Dec *d)
{
    uint64_t span = (uint64_t)d->high - d->low + 1;
    uint64_t s = (((uint64_t)d->code - d->low + 1) * d->total - 1) / span;
    uint32_t low = d->low;

    if (s >= d->total)
        return -1;
    d->high = (uint32_t)(low + span * (s + 1) / d->total - 1);
    d->low = (uint32_t)(low + span * s / d->total);
    normalize(d);
    d->total++;
    return (int)s;
}

uint8_t *arc_unpack(const Archive *a, const ArcEntry *e, char *err, size_t n)
{
    static int16_t parent[LIMIT];
    static uint8_t suffix[LIMIT], stack[LIMIT];
    uint8_t *out = malloc(e->size ? e->size : 1);
    size_t got = 0;
    Dec d;
    int i;

    if (!out) {
        snprintf(err, n, "%s: out of memory", e->name);
        return NULL;
    }
    memset(&d, 0, sizeof d);
    d.data = a->data + a->base + e->off + 4;
    d.len = a->len - (a->base + e->off + 4);   /* as illfiles.py: to the file's end */
    d.high = 0xFFFFFFFFu;
    d.total = 256;
    d.code = word(&d) << 16;
    d.code |= word(&d);
    d.bits = word(&d) << 16;
    d.bits |= word(&d);
    for (i = 0; i < LIMIT; i++) {
        parent[i] = -1;
        suffix[i] = (uint8_t)(i < 256 ? i : 0);
    }
    while (got < e->size) {
        int next = 256, prev = symbol(&d);
        if (prev < 0 || prev >= next)
            goto bad;
        out[got++] = suffix[prev];
        while (got < e->size) {
            int c = symbol(&d), k = 0, x;
            uint8_t first;
            if (c < 0 || c > next)
                goto bad;
            /* the string of c (of prev and its first byte when c is new) */
            for (x = c < next ? c : prev; x != -1; x = parent[x])
                stack[k++] = suffix[x];
            first = stack[k - 1];
            while (k && got < e->size)
                out[got++] = stack[--k];
            if (c == next && got < e->size)
                out[got++] = first;
            parent[next] = (int16_t)prev;
            suffix[next] = first;
            prev = c;
            if (++next == LIMIT) {
                d.total = 256;
                break;
            }
        }
    }
    return out;
bad:
    snprintf(err, n, "%s: its packed data is damaged.", e->name);
    free(out);
    return NULL;
}
