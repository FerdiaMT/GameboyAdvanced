/* Exercises loop control flow, comparisons, additions, and conditional branches. */

#define TEST_PASS() \
    do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() \
    do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)

__attribute__((noreturn)) void _start(void)
{
    unsigned int sum = 0;
    unsigned int value = 1;

    while (value <= 10)
    {
        sum += value;
        ++value;
    }

    if (sum == 55)
    {
        TEST_PASS();
    }

    TEST_FAIL();
}
