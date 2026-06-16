/////////////////////////// TEST /////////////////////////// ASSERT THE CONSISTENCY OF GENERATED MASK BETWEEN NEON64 MODE AND GENERIC MODE
//------------------------------------------------------//
// EXPECT: BOTH MODES MUST GENERATE MATCHING MASK
//////////////////////////////////////////////////////////

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <assert.h>
#include <time.h>
#include <chrono>
#include "../../src/include/arch/neon/64/simd.h"

#define LEN 32768

static inline void fill(uint8_t *b, int c, size_t n)
{
    std::memset(b, c, n);
}

static inline void fill_random_front(uint8_t *b, size_t n)
{
    while (n) b[--n] = rand() & 127;
}

static inline void fill_random_back(uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++) b[i] = std::rand() % 127;
}

static void test(uint8_t *buf)
{
    static uint64_t n_mask[LEN / MM_NGROUP];
    static uint64_t b_mask[LEN / MM_NGROUP];
    
    for (int k = 0; k < 256; k++)
    {
        {
            uint64_t  *mp   = n_mask;
            uint8x8_t dup   = mm_dup(k);

            for (size_t i = 0; i < LEN; i+=MM_NGROUP, mp++)
            {
                uint8x8_t grp = mm_load(buf+i);
                *mp  = mm_cmp(grp, dup);
            }
        }

        {
#include "../../src/include/undef.h"
#include "../../src/include/arch/generic/simd.h"
            uint64_t *mp   = b_mask;
            uint64_t dup   = mm_dup(k);

            for (size_t i = 0; i < LEN; i+=MM_NGROUP, mp++)
            {
                uint64_t grp = mm_load(buf+i);
                *mp  = mm_cmp(grp, dup);
            }
        }

        for (int i = 0; i < LEN/MM_NGROUP; i++)
            assert(n_mask[i] == b_mask[i]);
#     if 0

        std::printf("test <char %d on %d size> - All masks generated from NEON64 and GENERIC64 are consistent with each other\n", k, LEN);
        
#     endif
    }
}

int main(void)
{
    uint8_t buf[LEN];

    fill_random_front(buf, LEN);
    test(buf);
    fill_random_back(buf, LEN);
    test(buf);

    for (int i = 0; i < 256; i++)
    {
        fill(buf, i, LEN);
        test(buf);
    }

    return 0;
}
