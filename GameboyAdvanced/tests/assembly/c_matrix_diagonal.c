#define TEST_PASS() do { __asm__ volatile ("swi #0"); __builtin_unreachable(); } while (0)
#define TEST_FAIL() do { __asm__ volatile ("swi #1"); __builtin_unreachable(); } while (0)
__attribute__((noreturn)) void _start(void)
{
    unsigned int matrix[3][3];
    unsigned int row;
    unsigned int column;
    unsigned int diagonal = 0;
    for (row = 0; row < 3; ++row)
    {
        for (column = 0; column < 3; ++column) matrix[row][column] = row * 10 + column;
        diagonal += matrix[row][row];
    }
    if (diagonal == 33) TEST_PASS();
    TEST_FAIL();
}
