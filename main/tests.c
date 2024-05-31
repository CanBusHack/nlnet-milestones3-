#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "test.h"

void app_main(void);

static void test_libvin(void) {
    jmp_buf env;
    if (!setjmp(env)) {
        app_main();
    }
    fprintf(stderr, "%s failed\n", __func__);
    exit(1);
}

int main(int argc, char** argv) {
    test_libvin();
}
