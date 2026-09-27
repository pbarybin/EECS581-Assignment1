#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int isDigit(char c)
{
    return c >= '0' && c <= '9';
}

static int isTokenCharacter(char c)
{
    return isDigit(c) || c == '.' || c == ':';
}

/* Consume one decimal field, rejecting excess digits and leading zeroes. */
static int parseNumber(const char **cursor, const char *end,
                       unsigned int maxDigits, unsigned int maximum,
                       unsigned int *result)
{
    const char *start = *cursor;
    unsigned int value = 0;
    unsigned int digits = 0;

    while (*cursor < end && isDigit(**cursor)) {
        if (digits == maxDigits)
            return 0;
        value = value * 10U + (unsigned int)(**cursor - '0');
        ++digits;
        ++*cursor;
    }

    if (digits == 0 || (digits > 1 && *start == '0') || value > maximum)
        return 0;

    *result = value;
    return 1;
}

static int parseToken(const char *start, const char *end,
                      unsigned long *address, int *port)
{
    const char *cursor = start;
    unsigned long value = 0;
    unsigned int number;
    int parsedPort = -1;
    int octet;

    for (octet = 0; octet < 4; ++octet) {
        if (!parseNumber(&cursor, end, 3, 255, &number))
            return 0;
        value = (value << 8) | number;
        if (octet < 3) {
            if (cursor == end || *cursor != '.')
                return 0;
            ++cursor;
        }
    }

    if (cursor < end) {
        if (*cursor != ':')
            return 0;
        ++cursor;
        if (!parseNumber(&cursor, end, 5, 65535, &number))
            return 0;
        parsedPort = (int)number;
    }

    if (cursor != end)
        return 0;

    *address = value;
    *port = parsedPort;
    return 1;
}

/* Returns the first valid whole token. Output pointers must be non-NULL. */
int extractIPv4(const char* str, unsigned long* outAddress, int* outPort)
{
    const char *cursor = str;

    *outAddress = 0;
    *outPort = -1;
    if (str == NULL)
        return 0;

    while (*cursor != '\0') {
        const char *start;

        if (!isTokenCharacter(*cursor)) {
            ++cursor;
            continue;
        }

        start = cursor;
        while (isTokenCharacter(*cursor))
            ++cursor;

        if (parseToken(start, cursor, outAddress, outPort))
            return 1;
    }
    return 0;
}

/* Read a complete line: 1 = success, 0 = EOF, -1 = allocation/I/O error. */
static int readLine(char **line, size_t *capacity)
{
    size_t length = 0;
    int c;

    while ((c = getchar()) != '\n' && c != EOF) {
        if (length + 1 >= *capacity) {
            size_t newCapacity;
            char *resized;

            if (*capacity > (size_t)-1 / 2)
                return -1;
            newCapacity = *capacity == 0 ? 128 : *capacity * 2;
            resized = realloc(*line, newCapacity);
            if (resized == NULL)
                return -1;
            *line = resized;
            *capacity = newCapacity;
        }
        (*line)[length++] = (char)c;
    }
    if (ferror(stdin))
        return -1;
    if (c == EOF && length == 0)
        return 0;

    if (*line == NULL) {
        *line = malloc(1);
        if (*line == NULL)
            return -1;
        *capacity = 1;
    }
    /* Accept CRLF line endings as well as LF. */
    if (length > 0 && (*line)[length - 1] == '\r')
        --length;
    (*line)[length] = '\0';
    return 1;
}

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    int exitStatus = EXIT_SUCCESS;

    for (;;) {
        unsigned long address;
        int port;
        int status;

        printf("Enter text (END to quit): ");
        fflush(stdout);
        status = readLine(&line, &capacity);
        if (status < 0) {
            fprintf(stderr, "Error: unable to read input.\n");
            exitStatus = EXIT_FAILURE;
            break;
        }
        if (status == 0 || strcmp(line, "END") == 0) {
            puts("Program terminated.");
            break;
        }

        if (extractIPv4(line, &address, &port)) {
            printf("Extracted IPv4 address: %lu.%lu.%lu.%lu "
                   "(decimal value: %lu, port: ",
                   (address >> 24) & 255UL, (address >> 16) & 255UL,
                   (address >> 8) & 255UL, address & 255UL, address);
            if (port == -1)
                printf("none");
            else
                printf("%d", port);
            puts(")");
        } else {
            puts("Invalid input: no valid IPv4 address found.");
        }
    }

    free(line);
    return exitStatus;
}
