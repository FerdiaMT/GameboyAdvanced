#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    unsigned int value = 0xF0F0A55A;
    unsigned int count = 0;
    while (value != 0)
    {
        count += value & 1;
        value >>= 1;
    }
    if (count == 16) TEST_PASS();
    TEST_FAIL();
}
