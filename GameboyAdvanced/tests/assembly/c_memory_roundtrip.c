/* Exercises indexed stack memory stores and loads. */

#define TEST_PASS() \
    do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() \
    do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)

__attribute__((noreturn)) void _start(void)
{
    unsigned int values[3];
    values[0] = 7;
    values[1] = 11;
    values[2] = 19;

    if (values[0] + values[1] + values[2] == 37)
    {
        TEST_PASS();
    }

    TEST_FAIL();
}
