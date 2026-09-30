/* codeint.c - the routines the original keeps as code with its pictures
 * in it (the balls' sprites per table, CODE:36320 on, and the marks they
 * leave in the hide-lights mask, CODE:2601D on), run by reading their
 * instructions from the loaded image: the pictures are the game's, so
 * they stay in the player's copy.  Only the instruction forms those
 * routines are made of are known here (docs/HANDOFF.md, "The balls'
 * sprites"); anything else stops the port.
 */
#include "codeint.h"
#include "game.h"
#include "image.h"
#include "pmem.h"
#include "vga.h"

enum { ES_, CS_, SS_, DS_, FS_, GS_ };

static CiCpu *c;
static uint32_t ip;
static uint8_t stack[64];
static unsigned sp;

static void bad(void)
{
    pi_stop("CODE_RUN: an instruction the sprite routines do not have");
}

static uint8_t fetch(void)
{
    return lrb(PI_IMAGE_BASE + ip++);
}

static uint32_t fetch32(void)
{
    uint32_t v = fetch();

    v |= (uint32_t)fetch() << 8;
    v |= (uint32_t)fetch() << 16;
    return v | (uint32_t)fetch() << 24;
}

/* memory through segment `s` */
static uint8_t mrb(int s, uint32_t off)
{
    if (c->seg[s] == CI_VIDEO)
        return vga_read((uint16_t)off);
    return lrb(c->seg[s] + off);
}

static void mwb(int s, uint32_t off, uint8_t v)
{
    if (c->seg[s] == CI_VIDEO)
        vga_write((uint16_t)off, v);
    else
        lwb(c->seg[s] + off, v);
}

static uint32_t mread(int s, uint32_t off, int size)
{
    uint32_t v = 0;
    int i;

    for (i = size - 1; i >= 0; i--)
        v = v << 8 | mrb(s, off + (uint32_t)i);
    return v;
}

static void mwrite(int s, uint32_t off, int size, uint32_t v)
{
    int i;

    for (i = 0; i < size; i++, v >>= 8)
        mwb(s, off + (uint32_t)i, (uint8_t)v);
}

/* registers by number: 8-bit AL CL DL BL AH CH DH BH, else the low word
 * or all of EAX ECX EDX EBX ESP EBP ESI EDI */
static uint32_t rget(int r, int size)
{
    if (size == 1)
        return r < 4 ? c->r[r] & 0xFF : c->r[r - 4] >> 8 & 0xFF;
    return size == 2 ? c->r[r] & 0xFFFF : c->r[r];
}

static void rset(int r, int size, uint32_t v)
{
    if (size == 1) {
        if (r < 4)
            c->r[r] = (c->r[r] & ~0xFFu) | (v & 0xFF);
        else
            c->r[r - 4] = (c->r[r - 4] & ~0xFF00u) | (v & 0xFF) << 8;
    } else if (size == 2) {
        c->r[r] = (c->r[r] & ~0xFFFFu) | (v & 0xFFFF);
    } else {
        c->r[r] = v;
    }
}

/* a ModRM operand: a register (mem 0) or memory at seg:off */
typedef struct {
    int mem, reg, seg;
    uint32_t off;
} Opnd;

static int modrm(Opnd *o, int seg)
{
    uint8_t m = fetch(), mod = m >> 6, rm = m & 7;
    uint32_t off = 0;

    if (mod == 3) {
        o->mem = 0;
        o->reg = rm;
        return m >> 3 & 7;
    }
    if (rm == 4) {
        uint8_t sib = fetch(), base = sib & 7, idx = sib >> 3 & 7;

        if (base == 5 && mod == 0)
            off = fetch32();
        else if (base == 4 || base == 5)
            bad();
        else
            off = c->r[base];
        if (idx != 4)
            off += c->r[idx] << (sib >> 6);
    } else if (rm == 5 && mod == 0) {
        off = fetch32();
    } else if (rm == 5) {
        bad();                          /* EBP-based: SS, not in the routines */
    } else {
        off = c->r[rm];
    }
    if (mod == 1)
        off += (uint32_t)(int32_t)(int8_t)fetch();
    else if (mod == 2)
        off += fetch32();
    o->mem = 1;
    o->seg = seg;
    o->off = off;
    return m >> 3 & 7;
}

static uint32_t oget(const Opnd *o, int size)
{
    return o->mem ? mread(o->seg, o->off, size) : rget(o->reg, size);
}

static void oset(const Opnd *o, int size, uint32_t v)
{
    if (o->mem)
        mwrite(o->seg, o->off, size, v);
    else
        rset(o->reg, size, v);
}

static uint32_t mask(int size)
{
    return size == 1 ? 0xFFu : size == 2 ? 0xFFFFu : 0xFFFFFFFFu;
}

static void zf(uint32_t v, int size)
{
    c->zf = (v & mask(size)) == 0;
}

/* ADD, ADC, SUB, CMP (1: the result kept) */
static uint32_t arith(int op, uint32_t a, uint32_t b, int size)
{
    uint64_t m = mask(size), r;

    a &= (uint32_t)m;
    b &= (uint32_t)m;
    switch (op) {
    case 0:                             /* ADD */
        r = (uint64_t)a + b;
        c->cf = r > m;
        break;
    case 2:                             /* ADC */
        r = (uint64_t)a + b + (unsigned)c->cf;
        c->cf = r > m;
        break;
    default:                            /* SUB, CMP */
        r = (uint64_t)a - b;
        c->cf = a < b;
        break;
    }
    zf((uint32_t)r, size);
    return (uint32_t)(r & m);
}

void code_run(uint32_t start, CiCpu *cpu)
{
    long n = 0;

    c = cpu;
    ip = start;
    sp = 0;
    for (;;) {
        int seg = DS_, size = 4, r;
        uint8_t op;
        Opnd o;
        uint32_t v;

        if (++n > 1000000)
            pi_stop("CODE_RUN: no RET");
        for (;;) {
            op = fetch();
            if (op == 0x26)
                seg = ES_;
            else if (op == 0x64)
                seg = FS_;
            else if (op == 0x65)
                seg = GS_;
            else if (op == 0x66)
                size = 2;
            else
                break;
        }
        switch (op) {
        case 0x88:                      /* MOV r/m8,r8 */
            r = modrm(&o, seg);
            oset(&o, 1, rget(r, 1));
            break;
        case 0x89:                      /* MOV r/m,r */
            r = modrm(&o, seg);
            oset(&o, size, rget(r, size));
            break;
        case 0x8A:                      /* MOV r8,r/m8 */
            r = modrm(&o, seg);
            rset(r, 1, oget(&o, 1));
            break;
        case 0x8B:                      /* MOV r,r/m */
            r = modrm(&o, seg);
            rset(r, size, oget(&o, size));
            break;
        case 0xC6:                      /* MOV r/m8,imm8 */
            if (modrm(&o, seg) != 0)
                bad();
            oset(&o, 1, fetch());
            break;
        case 0x84:                      /* TEST r/m8,r8 */
            r = modrm(&o, seg);
            zf(oget(&o, 1) & rget(r, 1), 1);
            c->cf = 0;
            break;
        case 0x80:                      /* AND r/m8,imm8 */
            if (modrm(&o, seg) != 4)
                bad();
            v = oget(&o, 1) & fetch();
            oset(&o, 1, v);
            zf(v, 1);
            c->cf = 0;
            break;
        case 0x81:                      /* ADD, SUB, CMP r/m,imm32 */
        case 0x83:                      /* ADC r/m,imm8 */
            r = modrm(&o, seg);
            if (op == 0x81 ? r != 0 && r != 5 && r != 7 : r != 2)
                bad();
            v = op == 0x81 ? (size == 2 ? (uint32_t)fetch() | (uint32_t)fetch() << 8 : fetch32())
                           : (uint32_t)(int32_t)(int8_t)fetch();
            v = arith(r, oget(&o, size), v, size);
            if (r != 7)
                oset(&o, size, v);
            break;
        case 0x03:                      /* ADD r,r/m */
            r = modrm(&o, seg);
            rset(r, size, arith(0, rget(r, size), oget(&o, size), size));
            break;
        case 0x33:                      /* XOR r,r/m */
            r = modrm(&o, seg);
            v = rget(r, size) ^ oget(&o, size);
            rset(r, size, v);
            zf(v, size);
            c->cf = 0;
            break;
        case 0x69:                      /* IMUL r,r/m,imm32 */
        case 0x6B:                      /* IMUL r,r/m,imm8 */
            if (size != 4)
                bad();
            r = modrm(&o, seg);
            v = op == 0x69 ? fetch32() : (uint32_t)(int32_t)(int8_t)fetch();
            {
                int64_t p = (int64_t)(int32_t)oget(&o, 4) * (int32_t)v;

                rset(r, 4, (uint32_t)p);
                c->cf = p != (int32_t)p;
            }
            break;
        case 0x0F:                      /* MOVZX r32,r/m8 or r/m16 */
            op = fetch();
            if (op != 0xB6 && op != 0xB7)
                bad();
            r = modrm(&o, seg);
            rset(r, 4, oget(&o, op == 0xB6 ? 1 : 2));
            break;
        case 0xD0:                      /* ROL, ROR r/m8,1 */
            r = modrm(&o, seg);
            v = oget(&o, 1);
            if (r == 0) {
                c->cf = v >> 7 & 1;
                v = (v << 1 | v >> 7) & 0xFF;
            } else if (r == 1) {
                c->cf = v & 1;
                v = v >> 1 | (v & 1) << 7;
            } else {
                bad();
            }
            oset(&o, 1, v);
            break;
        case 0xD1:                      /* SHR r/m,1 */
        case 0xD2:                      /* SHL, SHR r/m8,CL */
        case 0xC1:                      /* SHR r/m,imm8 */
            {
                int sz = op == 0xD2 ? 1 : size;
                unsigned k;

                r = modrm(&o, seg);
                k = op == 0xD1 ? 1 : op == 0xD2 ? (c->r[1] & 0x1F) : (fetch() & 0x1Fu);
                if ((r != 4 && r != 5) || (op != 0xD2 && r != 5))
                    bad();
                if (k == 0)
                    break;
                v = oget(&o, sz);
                if (r == 4) {
                    c->cf = (v << (k - 1)) >> (sz * 8 - 1) & 1;
                    v = (v << k) & mask(sz);
                } else {
                    c->cf = v >> (k - 1) & 1;
                    v >>= k;
                }
                oset(&o, sz, v);
                zf(v, sz);
            }
            break;
        case 0xFE:                      /* INC r/m8 */
            if (modrm(&o, seg) != 0)
                bad();
            v = (oget(&o, 1) + 1) & 0xFF;
            oset(&o, 1, v);
            zf(v, 1);
            break;
        case 0x40: case 0x41: case 0x42: case 0x43:
        case 0x45: case 0x46: case 0x47:        /* INC r */
            v = (rget(op - 0x40, size) + 1) & mask(size);
            rset(op - 0x40, size, v);
            zf(v, size);
            break;
        case 0x48: case 0x49: case 0x4A: case 0x4B:
        case 0x4D: case 0x4E: case 0x4F:        /* DEC r */
            v = (rget(op - 0x48, size) - 1) & mask(size);
            rset(op - 0x48, size, v);
            zf(v, size);
            break;
        case 0xB0: case 0xB1: case 0xB2: case 0xB3:
        case 0xB4: case 0xB5: case 0xB6: case 0xB7:     /* MOV r8,imm8 */
            rset(op - 0xB0, 1, fetch());
            break;
        case 0xB8: case 0xB9: case 0xBA: case 0xBB:
        case 0xBD: case 0xBE: case 0xBF:        /* MOV r,imm */
            v = size == 2 ? (uint32_t)fetch() | (uint32_t)fetch() << 8 : fetch32();
            rset(op - 0xB8, size, v);
            break;
        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x55: case 0x56: case 0x57:        /* PUSH r16 */
            if (size != 2 || sp + 2 > sizeof stack)
                bad();
            v = rget(op - 0x50, 2);
            stack[sp++] = (uint8_t)v;
            stack[sp++] = (uint8_t)(v >> 8);
            break;
        case 0x58: case 0x59: case 0x5A: case 0x5B:
        case 0x5D: case 0x5E: case 0x5F:        /* POP r16 */
            if (size != 2 || sp < 2)
                bad();
            sp -= 2;
            rset(op - 0x58, 2, stack[sp] | (uint32_t)stack[sp + 1] << 8);
            break;
        case 0xEF:                      /* OUT DX,AX */
            if (size != 2)
                bad();
            vga_outw((uint16_t)c->r[2], (uint16_t)c->r[0]);
            break;
        case 0x73:                      /* JAE */
        case 0x75:                      /* JNE */
            v = (uint32_t)(int32_t)(int8_t)fetch();
            if (op == 0x73 ? !c->cf : !c->zf)
                ip += v;
            break;
        case 0xC3:                      /* RET */
            if (sp != 0)
                bad();
            return;
        default:
            bad();
        }
    }
}
