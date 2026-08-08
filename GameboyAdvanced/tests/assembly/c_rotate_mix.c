#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    unsigned int value = 0x12345678;
    unsigned int rotated = (value << 8) | (value >> 24);
    if ((rotated ^ 0xA5A5A5A5) == 0x91F3DDB7) TEST_PASS();
    TEST_FAIL();
}
