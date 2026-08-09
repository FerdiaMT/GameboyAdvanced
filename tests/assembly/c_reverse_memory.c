#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    unsigned int values[6];
    unsigned int index;
    values[0] = 1;
    values[1] = 3;
    values[2] = 5;
    values[3] = 7;
    values[4] = 9;
    values[5] = 11;
    for (index = 0; index < 3; ++index)
    {
        unsigned int temporary = values[index];
        values[index] = values[5 - index];
        values[5 - index] = temporary;
    }
    if (values[0] == 11 && values[1] == 9 && values[2] == 7 && values[5] == 1) TEST_PASS();
    TEST_FAIL();
}
