#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    unsigned int values[6];
    unsigned int *cursor = values;
    unsigned int *end = values + 6;
    unsigned int total = 0;
    values[0] = 2;
    values[1] = 4;
    values[2] = 8;
    values[3] = 16;
    values[4] = 32;
    values[5] = 64;
    while (cursor != end) total += *cursor++;
    if (total == 126) TEST_PASS();
    TEST_FAIL();
}
