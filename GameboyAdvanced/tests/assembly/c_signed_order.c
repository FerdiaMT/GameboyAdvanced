#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    int values[5];
    int index;
    int negative_sum = 0;
    int positive_count = 0;
    values[0] = -14;
    values[1] = 7;
    values[2] = -3;
    values[3] = 22;
    values[4] = -1;
    for (index = 0; index < 5; ++index)
    {
        if (values[index] < 0) negative_sum += values[index];
        else positive_count += 1;
    }
    if (negative_sum == -18 && positive_count == 2) TEST_PASS();
    TEST_FAIL();
}
