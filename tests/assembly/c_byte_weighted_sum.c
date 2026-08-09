#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    unsigned char bytes[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    unsigned int index;
    unsigned int total = 0;
    for (index = 0; index < 8; ++index) total += bytes[index] * (index + 1);
    if (total == 168) TEST_PASS();
    TEST_FAIL();
}
