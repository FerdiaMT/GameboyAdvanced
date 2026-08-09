/* Exercises logical shifts, XOR, and immediate arithmetic. */

#define TEST_PASS() \
    do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() \
    do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)

__attribute__((noreturn)) void _start(void)
{
    unsigned int left = 0x12;
    unsigned int right = 0x30;
    unsigned int result = ((left << 2) ^ right) + 0x0A;

    if (result == 0x82)
    {
        TEST_PASS();
    }

    TEST_FAIL();
}
