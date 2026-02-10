#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "types.h"
#include "io_ports.h"


void *memset(void *dst, char c, uint32 n) {
    char *temp = dst;
    for (; n != 0; n--) *temp++ = c;
    return dst;
}

void *memcpy(void *dst, const void *src, uint32 n) {
    char *ret = dst;
    char *p = dst;
    const char *q = src;
    while (n--)
        *p++ = *q++;
    return ret;
}

int memcmp(uint8 *s1, uint8 *s2, uint32 n) {
    while (n--) {
        if (*s1 != *s2)
            return 0;
        s1++;
        s2++;
    }
    return 1;
}

int strlen(const char *s) {
    int len = 0;
    while (*s++)
        len++;
    return len;
}

int strcmp(const char *s1, char *s2) {
    int i = 0;

    while ((s1[i] == s2[i])) {
        if (s2[i++] == 0)
            return 0;
    }
    return 1;
}

int strcpy(char *dst, const char *src) {
    int i = 0;
    while ((*dst++ = *src++) != 0)
        i++;
    return i;
}

void strcat(char *dest, const char *src) {
    char *end = (char *)dest + strlen(dest);
    memcpy((void *)end, (void *)src, strlen(src));
    end = end + strlen(src);
    *end = '\0';
}

int isspace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int isalpha(char c) {
    return (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')));
}

char upper(char c) {
    if ((c >= 'a') && (c <= 'z'))
        return (c - 32);
    return c;
}

char lower(char c) {
    if ((c >= 'A') && (c <= 'Z'))
        return (c + 32);
    return c;
}

void itoa(char *buf, int base, int d) {
    char *p = buf;
    char *p1, *p2;
    unsigned long ud = d;
    int divisor = 10;

    if (base == 'd' && d < 0) {
        *p++ = '-';
        buf++;
        ud = -d;
    } else if (base == 'x')
        divisor = 16;

    do {
        int remainder = ud % divisor;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= divisor);

    *p = 0;

    p1 = buf;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}

int strncmp(const char *s1, const char *s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0;
}


char *strncpy(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

const char *strchr(const char *s, int c) {
    while (*s != '\0') {
        if (*s == (char)c) {
            return s;
        }
        s++;
    }
    if ((char)c == '\0') {
        return s;
    }
    return NULL;
}
