#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    unsigned short values[4];
    values[0] = 0x1234;
    values[1] = 0x5678;
    values[2] = 0x0F0F;
    values[3] = 0x1111;
    if (((values[0] + values[1]) ^ values[2]) + values[3] == 0x78B4) TEST_PASS();
    TEST_FAIL();
}
