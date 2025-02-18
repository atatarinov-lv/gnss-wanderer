#include "stdio.h"

#include "dbg.h"

int main(int argc, char *argv[]) {
    debug("argc: %d", argc);
    check(argc > 1, "no commands ar the input");

    char *symb;
    int sum;

    for (int i = 1; i < argc; i++) {
        debug("argv[%d]: %s", i, argv[i]);

        symb = argv[i];
        sum = 0;

        for (symb = argv[i]; *symb != '\0'; symb++) {
            sum  = sum ^ *symb;
        }

        printf("%x\n", sum);
    }

    return 0;

error:
    return 1;
}
