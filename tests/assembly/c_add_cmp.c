/* A freestanding ARM7TDMI test: no headers, runtime, or linker script needed. */

#define TEST_PASS() \
    do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() \
    do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)

__attribute__((noreturn)) void _start(void)
{
    unsigned int left = 0x12;
    unsigned int right = 0x30;

    if (left + right == 0x42)
    {
        TEST_PASS();
    }

    TEST_FAIL();
}
