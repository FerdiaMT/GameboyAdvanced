/* Nested loops, byte loads, data-dependent branches, shifts, and XOR. */

#define TEST_PASS() \
    do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() \
    do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)

__attribute__((noreturn)) void _start(void)
{
    unsigned char bytes[4] = { 0, 0, 0, 0 };
    unsigned int state = 0xFFFFFFFF;
    unsigned int byte_index;

    for (byte_index = 0; byte_index < 4; ++byte_index)
    {
        unsigned int bit_index;
        state ^= bytes[byte_index];

        for (bit_index = 0; bit_index < 8; ++bit_index)
        {
            if (state & 1)
            {
                state = (state >> 1) ^ 0xEDB88320;
            }
            else
            {
                state >>= 1;
            }
        }
    }

    /* Internal IEEE CRC-32 state after four zero bytes. */
    if (state == 0xDEBB20E3)
    {
        TEST_PASS();
    }

    TEST_FAIL();
}
