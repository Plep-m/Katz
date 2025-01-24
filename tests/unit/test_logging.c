#include "utils/logging.h"
#include <stdio.h>
#include <string.h>

void test_logging() {
    log_set_level(LOG_LEVEL_DEBUG);
    log_debug("This is a debug message");
    log_info("This is an info message");
    log_warn("This is a warning message");
    log_error("This is an error message");
}

int main() {
    test_logging();
    return 0;
}