#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    unsigned int value;
    unsigned int total = 0;
    for (value = 1; value <= 12; ++value) total += value * (value + 3);
    if (total == 884) TEST_PASS();
    TEST_FAIL();
}
