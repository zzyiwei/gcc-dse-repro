/*
 * Copyright 2026 Google LLC
 * SPDX-License-Identifier: MIT
 */

#include <stdint.h>
#include <stdio.h>

#define CHAIN_TYPE_A 7
#define CHAIN_TYPE_B 13

typedef uint32_t ChainType;

typedef struct ChainBase {
    ChainType sType;
    const void *pNext;
} ChainBase;

typedef struct A {
    ChainType sType;
    const void *pNext;
    uint32_t dummy;
} A;

typedef struct B {
    ChainType sType;
    const void *pNext;
    uint32_t dummy;
} B;

/* dummy helpers */
static inline size_t
sizeof_uint32_t(const uint32_t *)
{
    return 4;
}

static inline size_t
sizeof_simple_pointer(const void *)
{
    return 8;
}

static inline size_t
sizeof_B_self(const B *val)
{
    return sizeof_uint32_t(&val->dummy);
}

/* walk through the pNext chain */
static __attribute__((noinline)) size_t
sizeof_pnext(const void *val)
{
    const ChainBase *pnext = val;
    size_t size = 0;
    while (pnext) {
        switch ((int32_t)pnext->sType) {
        case CHAIN_TYPE_B:
            size += sizeof_simple_pointer(pnext);
            size += sizeof_uint32_t(&pnext->sType);
            size += sizeof_pnext(pnext->pNext);
            size += sizeof_B_self((const B *)pnext);
            return size;
        default:
            break;
        }
        pnext = pnext->pNext;
    }
    return sizeof_simple_pointer(NULL);
}

static __attribute__((noinline)) size_t
sizeof_A(const A *val)
{
    size_t size = 0;
    size += sizeof_uint32_t(&val->sType);
    size += sizeof_pnext(val->pNext);
    size += sizeof_uint32_t(&val->dummy);
    return size;
}

int
main(void)
{
    const B b = {
        .sType = CHAIN_TYPE_B,
        .pNext = NULL,
        .dummy = 47,
    };
    const A a = {
        .sType = CHAIN_TYPE_A,
        .pNext = &b, /* This is falsely DSE'ed. */
        .dummy = 11,
    };
    //__asm__ volatile("" : : "g"(a.pNext) : "memory");
    printf("sizeof_A=%zu\n", sizeof_A(&a));
    return 0;
}
