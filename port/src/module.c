/* module.c - the table's code module (TABLE_MODULE, CODE:B22D): its name
 * made, SOURCE\T00n.BPC loaded and relocated, the resources its header
 * names loaded and the header copied (docs/bpc-module.md); OPTIONS_APPLY;
 * the table's high scores copied.
 */
#include "game.h"
#include "names.h"
#include "pmax.h"
#include "pmem.h"

/* ADC AL,src and DAA on the byte at DS:`dst`; *cf carries */
void adc_daa(uint32_t dst, uint32_t src, int *cf)
{
    uint8_t a = rb(dst), b = rb(src), al;
    unsigned sum = (unsigned)a + b + (unsigned)*cf;
    int af = ((a & 0xF) + (b & 0xF) + *cf) > 0xF, c = sum > 0xFF;

    al = (uint8_t)sum;
    if ((al & 0xF) > 9 || af) {
        c = c || al > 0xF9;
        al = (uint8_t)(al + 6);
    }
    if ((uint8_t)sum > 0x99 || sum > 0xFF) {
        al = (uint8_t)(al + 0x60);
        c = 1;
    } else {
        c = 0;
    }
    wb(dst, al);
    *cf = c;
}

/* SBB AL,src and DAS on the byte at DS:`dst`; *cf borrows (DAS keeps
 * the borrow of its first step, as the 386 and the runner do) */
void sbb_das(uint32_t dst, uint32_t src, int *cf)
{
    uint8_t a = rb(dst), b = rb(src), al, old;
    int d = (int)a - b - *cf;
    int af = (int)(a & 0xF) - (b & 0xF) - *cf < 0, oc = d < 0, c = 0;

    old = al = (uint8_t)d;
    if ((al & 0xF) > 9 || af) {
        c = oc || al < 6;
        al = (uint8_t)(al - 6);
    }
    if (old > 0x99 || oc) {
        al = (uint8_t)(al - 0x60);
        c = 1;
    }
    wb(dst, al);
    *cf = c;
}

/* the game's 12-digit packed-BCD add: the number whose end is DS:`src`
 * added to the one whose end is DS:`dst` (the low four bytes before the
 * end, then the two high bytes 8 before it, as the original's code does
 * it); the work cells that held the two ends are left to the caller */
void bcd12_add(uint32_t dst, uint32_t src)
{
    int cf = 0, i;

    for (i = 0; i < 4; i++)
        adc_daa(dst - 4 + (uint32_t)i, src - 4 + (uint32_t)i, &cf);
    for (i = 0; i < 2; i++)
        adc_daa(dst - 8 + (uint32_t)i, src - 8 + (uint32_t)i, &cf);
}

/* one nibble's step of DEC_TEXT: `table`'s BCD word n - 1 (for the
 * nibble n, 1..15) added to DEC_BCD, the work cells left as the original
 * leaves them */
static void dec_add(uint32_t table, uint32_t n)
{
    int cf = 0;

    wd(0x0004, table + 2 * n);
    adc_daa(rd(0x0008) - 2, rd(0x0004) - 2, &cf);
    adc_daa(rd(0x0008) - 1, rd(0x0004) - 1, &cf);
    wd(0x0004, rd(0x0004) - 2);
}

/* the low word of [0020] shifted a nibble right, the high word kept */
static uint32_t shr_si(uint32_t v)
{
    return (v & 0xFFFF0000u) | ((v & 0xFFFF) >> 4);
}

/* one digit: [0024]'s low byte the nibble + [002C], written before
 * [0000] */
static void dec_digit(uint32_t bx)
{
    uint32_t ecx = (rd(0x0024) & 0xFFFFFF00u) | (uint8_t)((bx & 0xF) + rb(0x002C));

    wd(0x0024, ecx);
    wd(0x0000, rd(0x0000) - 1);
    wb(rd(0x0000), (uint8_t)ecx);
}

/* CODE:305E3: the low three nibbles of [0020] as decimal, BCD through
 * DEC_ONES, DEC_SIXTEENS and DEC_256S into the word BIN_BCD_WORD ([0000]
 * the place, [0004] the table's); the BCD word to [0020]'s low word */
void BIN_BCD(void)
{
    static const uint32_t tables[3] = { N_DEC_ONES, N_DEC_SIXTEENS, N_DEC_256S };
    int i;

    ww(N_BIN_BCD_WORD, 0);
    wd(0x0000, N_BIN_BCD_WORD + 2);
    for (i = 0; i < 3; i++) {
        uint32_t n;
        int cf = 0;

        if (i)
            wd(0x0020, shr_si(rd(0x0020)));
        n = rd(0x0020) & 0xF;
        wd(0x0028, (i ? rd(0x0028) & 0xFFFFFF00u : 0) | n);
        if (!n)
            continue;
        wd(0x0004, tables[i] + 2 * n);
        adc_daa(rd(0x0000) - 2, rd(0x0004) - 2, &cf);
        adc_daa(rd(0x0000) - 1, rd(0x0004) - 1, &cf);
        wd(0x0004, rd(0x0004) - 2);
        wd(0x0000, rd(0x0000) - 2);
        if (i < 2)
            wd(0x0000, rd(0x0000) + 2);
    }
    wd(0x0020, (rd(0x0020) & 0xFFFF0000u) | rw(N_BIN_BCD_WORD));
}

/* CODE:30368, host vector +18h: the low three nibbles of [0020] as
 * decimal (BCD through the tables DEC_ONES, DEC_SIXTEENS, DEC_256S into
 * DEC_BCD), its digits ('0' = [002C] = 30h) written backwards before
 * [0000], at most three, no leading zeros */
void DEC_TEXT(void)
{
    static const uint32_t tables[3] = { N_DEC_ONES, N_DEC_SIXTEENS, N_DEC_256S };
    uint32_t bx;
    int i;

    wd(0x002C, 0x30);
    ww(N_DEC_BCD, 0);
    wd(0x0008, N_DEC_BCD + 2);
    for (i = 0; i < 3; i++) {
        uint32_t n;

        if (i)
            wd(0x0020, shr_si(rd(0x0020)));
        n = rd(0x0020) & 0xF;
        wd(0x0028, (i ? rd(0x0028) & 0xFFFFFF00u : 0) | n);
        if (n)
            dec_add(tables[i], n);
    }
    bx = (rd(0x0020) & 0xFFFF0000u) | rw(N_DEC_BCD);
    for (i = 0; i < 3; i++) {
        if (i) {
            bx = (bx & 0xFFFF0000u) | ((bx & 0xFFFF) >> 4);
            wd(0x0020, bx);
            if (!(bx & 0xFFFF))
                return;
        }
        dec_digit(bx);
    }
}

/* CODE:B743: each dword of the relocation table (REL_COUNT of them at
 * REL_SEL:REL_START) is an offset in the module, whose dword gets
 * REL_BASE (the module's DS offset) added */
static void RELOCATE(uint16_t fs, uint32_t esi, uint32_t ebx, uint32_t ecx)
{
    uint32_t at = pmax_base(fs) + esi;

    ww(N_REL_SEL, fs);
    wd(N_REL_START, esi);
    wd(N_REL_BASE, ebx);
    wd(N_REL_COUNT, ecx);
    while (ecx--) {
        uint32_t off = lrd(at);
        at += 4;
        wd(ebx + off, rd(ebx + off) + ebx);
    }
}

/* CODE:B768: every relocated dword that is `edx` becomes `eax` */
static void REL_REPLACE(uint32_t edx, uint32_t eax)
{
    uint32_t at = pmax_base(rw(N_REL_SEL)) + rd(N_REL_START);
    uint32_t ebx = rd(N_REL_BASE), n = rd(N_REL_COUNT);

    while (n--) {
        uint32_t off = lrd(at);
        at += 4;
        if (rd(ebx + off) == edx)
            wd(ebx + off, eax);
    }
}

/* CODE:B54E: a resource, the file named at DS:`name`, loaded ("DATALOAD")
 * and its selector added to BLOCKS; its DS offset, 0 when it failed */
static uint32_t dataload(uint32_t name)
{
    uint16_t sel;
    uint32_t p = rd(N_BLOCKS_END);

    pmax_name(0xB551, rw(N_TABLE_DS));       /* "DATALOAD" */
    sel = pmax_load_ds(name, NULL);

    ww(p, sel);
    wd(N_BLOCKS_END, p + 2);
    if (!sel)
        return 0;
    return pmax_base(sel) - rd(N_TABLE_BASE);
}

/* CODE:B5B6: two files, named one after the other at DS:`name`, read from
 * the top of the heap and joined in a block of their sizes ("DATALOAD 2",
 * INT 92h AH=9), whose linear address is added to ALLOCS; its DS offset,
 * 0 when a file failed */
static uint32_t dataload2(uint32_t name)
{
    uint32_t n1 = 0, n2 = 0, at, a1, a2, i;
    uint16_t s1, s2;

    wd(N_DATALOAD2_KEY, name);
    pmax_policy(1);
    pmax_name(0xB5D4, rw(N_TABLE_DS));       /* "TEMP DATALOAD 1" */
    s1 = pmax_load_ds(name, &n1);
    ww(N_DATALOAD2_TEMP1, s1);
    if (!s1)
        return 0;       /* policy 1 stays set, as in the original */
    while (rb(name++))
        ;
    pmax_name(0xB635, rw(N_TABLE_DS));       /* "TEMP DATALOAD 2" */
    s2 = pmax_load_ds(name, &n2);
    ww(N_DATALOAD2_TEMP2, s2);
    if (!s2)
        return 0;
    pmax_policy(0);
    pmax_name(0xB6A0, rw(N_TABLE_DS));       /* "DATALOAD 2" */
    at = pmax_alloc_linear_here(n1 + n2);
    if (!at)
        pi_stop("TABLE_MODULE: no room for DATALOAD 2 (its CF is not looked at)");
    wd(rd(N_ALLOCS_END), at);
    wd(N_ALLOCS_END, rd(N_ALLOCS_END) + 4);
    a1 = pmax_base(s1);
    a2 = pmax_base(s2);
    for (i = 0; i < n1; i++)
        lwb(at + i, lrb(a1 + i));
    for (i = 0; i < n2; i++)
        lwb(at + n1 + i, lrb(a2 + i));
    REL_REPLACE(rd(N_DATALOAD2_KEY), at - rd(N_TABLE_BASE));
    pmax_free(s1);
    pmax_free(s2);
    return at - rd(N_TABLE_BASE);
}

/* CODE:B3F5: 1 (CF) when a file failed */
static int MODULE_LOAD(void)
{
    uint32_t n = 0, base, head, i;
    uint16_t sel, rel;

    pmax_name(0xB3FC, rw(N_TABLE_DS));       /* "table bin file" */
    sel = pmax_load_ds(N_MODULE_NAME, &n);
    ww(N_MODULE_SEL, sel);
    wd(N_MODULE_SIZE, n);
    if (!sel)
        return 1;
    base = pmax_base(sel) - rd(N_TABLE_BASE);
    wd(N_MODULE_BASE, base);
    wb(N_MODULE_EXT, 'r');
    wb(N_MODULE_EXT + 1, 'e');
    wb(N_MODULE_EXT + 2, 'l');
    pmax_policy(1);
    n = 0;
    pmax_name(0xB485, rw(N_TABLE_DS));       /* "table bin file relocation table" */
    rel = pmax_load_ds(N_MODULE_NAME, &n);
    if (!rel)
        return 1;       /* policy 1 stays set, as in the original */
    ww(N_MODULE_REL_SEL, rel);
    pmax_policy(0);
    RELOCATE(rel, 0, base, n >> 2);

    /* the header's 2Dh dwords to MODULE_HEADER; one with bit 31 set names
     * a resource, bit 30 too two files to join */
    head = pmax_base(sel);
    for (i = 0; i < 0x2D; i++) {
        uint32_t v = lrd(head + 4 * i);

        if (v & 0x80000000u) {
            v &= 0x7FFFFFFFu;
            if (v & 0xC0000000u) {
                uint32_t to = dataload2(v & 0x3FFFFFFFu);
                if (!to)
                    return 1;
                v = to;
            } else {
                uint32_t to = dataload(v);
                if (!to)
                    return 1;
                REL_REPLACE(v, to);
                v = to;
            }
        }
        wd(N_MODULE_HEADER + 4 * i, v);
    }
    return 0;
}

/* CODE:B2E0: the option bytes through the word tables at CODE:B2C0..
 * (read from memory, so that a byte out of range reads what the original
 * does) */
static void OPTIONS_APPLY(void)
{
    ww(N_BALLS_PER_GAME, rw(0xB2C0 + 2 * (uint32_t)rb(N_OPTIONS)));
    ww(N_SLOPE_Y, rw(0xB2D6 + 2 * (uint32_t)rb(N_OPT_ANGLE)));
    ww(N_SCROLL_DIVISOR, rw(0xB2C4 + 2 * (uint32_t)rb(N_OPT_SCROLLING)));
    ww(N_TILT_STEP, rw(0xB2CA + 2 * (uint32_t)rb(N_OPT_TILT)));
    ww(N_SLOPE_X, 0);
    ww(N_SERVE_SECONDS, 0x0A);
    ww(N_RES_CODE, rw(0xB2CE + 2 * (uint32_t)rb(N_OPT_RESOLUTION)));
}

/* CODE:B366: the table's 32h bytes of HISCORES to TABLE_HISCORES; its CF
 * (the first byte 0) is not looked at by TABLE_MODULE */
static void HISCORES_GET(void)
{
    uint32_t from = N_HISCORES + 0x32 * (uint32_t)rb(N_TABLE_INDEX), i;

    for (i = 0; i < 0x32; i++)
        wb(N_TABLE_HISCORES + i, rb(from + i));
}

int TABLE_MODULE(void)
{
    uint32_t hook;

    wb(N_TABLE_HISCORES, 0);
    /* "source\t00n.bpc": the digits made again from the table's number */
    wb(N_MODULE_NAME + 8, '0');
    wb(N_MODULE_NAME + 9, '0');
    wb(N_MODULE_NAME + 10, '0');
    wb(N_MODULE_EXT, 'b');
    wb(N_MODULE_EXT + 1, 'p');
    wb(N_MODULE_EXT + 2, 'c');
    wd(0x0000, N_MODULE_EXT - 1);
    wd(0x0020, (uint32_t)rw(N_TABLE_INDEX) + 1);
    DEC_TEXT();
    if (MODULE_LOAD())
        return 1;
    OPTIONS_APPLY();
    ww(N_BALLS_ON_TABLE, 1);
    wb(N_TBL_DBF5, 0);
    /* the module's slot 39, a RET in all four (docs/bpc-module.md) */
    hook = rd(rd(N_MODULE_BASE) + 0x9C);
    if (rb(hook) != 0xC3)
        pi_stop("TABLE_MODULE: the module's slot 39 is not a RET");
    HISCORES_GET();
    return 0;
}
