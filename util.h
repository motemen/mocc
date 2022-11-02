_Noreturn void error(char *fmt, ...) __attribute__((format(printf, 1, 2)));
_Noreturn void error_at(char *loc, char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
