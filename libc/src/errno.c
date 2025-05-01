/* errno is not yet implemented, this is just a little stub really */

int errno = 22; // EINVAL
int *__errno_location(void) {
    return &errno;
}
