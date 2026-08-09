#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    unsigned int left = 0xFFFFFFFF;
    unsigned int right = 2;
    unsigned int result = left + right;
    if (result == 1 && result < left) TEST_PASS();
    TEST_FAIL();
}
