int strlen(char *str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}
