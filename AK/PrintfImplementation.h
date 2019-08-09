#pragma once

#include <AK/Types.h>
#include <stdarg.h>

static constexpr const char* printf_hex_digits = "0123456789abcdef";

#ifdef __serenity__
extern "C" size_t strlen(const char*);
#else
#include <string.h>
#endif

template<typename PutChFunc, typename T>
[[gnu::always_inline]] inline int print_hex(PutChFunc putch, char*& bufptr, T number, u8 fields)
{
    int ret = 0;
    u8 shr_count = fields * 4;
    while (shr_count) {
        shr_count -= 4;
        putch(bufptr, printf_hex_digits[(number >> shr_count) & 0x0F]);
        ++ret;
    }
    return ret;
}

template<typename PutChFunc>
[[gnu::always_inline]] inline int print_number(PutChFunc putch, char*& bufptr, u32 number, bool leftPad, bool zeroPad, u32 fieldWidth)
{
    u32 divisor = 1000000000;
    char ch;
    char padding = 1;
    char buf[16];
    char* p = buf;

    for (;;) {
        ch = '0' + (number / divisor);
        number %= divisor;
        if (ch != '0')
            padding = 0;
        if (!padding || divisor == 1)
            *(p++) = ch;
        if (divisor == 1)
            break;
        divisor /= 10;
    }

    size_t numlen = p - buf;
    if (!fieldWidth || fieldWidth < numlen)
        fieldWidth = numlen;
    if (!leftPad) {
        for (unsigned i = 0; i < fieldWidth - numlen; ++i) {
            putch(bufptr, zeroPad ? '0' : ' ');
        }
    }
    for (unsigned i = 0; i < numlen; ++i) {
        putch(bufptr, buf[i]);
    }
    if (leftPad) {
        for (unsigned i = 0; i < fieldWidth - numlen; ++i) {
            putch(bufptr, ' ');
        }
    }

    return fieldWidth;
}

template<typename PutChFunc>
[[gnu::always_inline]] inline int print_u64(PutChFunc putch, char*& bufptr, u64 number, bool leftPad, bool zeroPad, u32 fieldWidth)
{
    u64 divisor = 10000000000000000000LLU;
    char ch;
    char padding = 1;
    char buf[16];
    char* p = buf;

    for (;;) {
        ch = '0' + (number / divisor);
        number %= divisor;
        if (ch != '0')
            padding = 0;
        if (!padding || divisor == 1)
            *(p++) = ch;
        if (divisor == 1)
            break;
        divisor /= 10;
    }

    size_t numlen = p - buf;
    if (!fieldWidth || fieldWidth < numlen)
        fieldWidth = numlen;
    if (!leftPad) {
        for (unsigned i = 0; i < fieldWidth - numlen; ++i) {
            putch(bufptr, zeroPad ? '0' : ' ');
        }
    }
    for (unsigned i = 0; i < numlen; ++i) {
        putch(bufptr, buf[i]);
    }
    if (leftPad) {
        for (unsigned i = 0; i < fieldWidth - numlen; ++i) {
            putch(bufptr, ' ');
        }
    }

    return fieldWidth;
}

template<typename PutChFunc>
[[gnu::always_inline]] inline int print_i64(PutChFunc putch, char*& bufptr, i64 number, bool leftPad, bool zeroPad, u32 fieldWidth)
{
    if (number < 0) {
        putch(bufptr, '-');
        return print_u64(putch, bufptr, 0 - number, leftPad, zeroPad, fieldWidth) + 1;
    }
    return print_u64(putch, bufptr, number, leftPad, zeroPad, fieldWidth);
}

template<typename PutChFunc>
[[gnu::always_inline]] inline int print_octal_number(PutChFunc putch, char*& bufptr, u32 number, bool leftPad, bool zeroPad, u32 fieldWidth)
{
    u32 divisor = 134217728;
    char ch;
    char padding = 1;
    char buf[32];
    char* p = buf;

    for (;;) {
        ch = '0' + (number / divisor);
        number %= divisor;
        if (ch != '0')
            padding = 0;
        if (!padding || divisor == 1)
            *(p++) = ch;
        if (divisor == 1)
            break;
        divisor /= 8;
    }

    size_t numlen = p - buf;
    if (!fieldWidth || fieldWidth < numlen)
        fieldWidth = numlen;
    if (!leftPad) {
        for (unsigned i = 0; i < fieldWidth - numlen; ++i) {
            putch(bufptr, zeroPad ? '0' : ' ');
        }
    }
    for (unsigned i = 0; i < numlen; ++i) {
        putch(bufptr, buf[i]);
    }
    if (leftPad) {
        for (unsigned i = 0; i < fieldWidth - numlen; ++i) {
            putch(bufptr, ' ');
        }
    }

    return fieldWidth;
}

template<typename PutChFunc>
[[gnu::always_inline]] inline int print_string(PutChFunc putch, char*& bufptr, const char* str, bool leftPad, u32 fieldWidth)
{
    size_t len = strlen(str);
    if (!fieldWidth || fieldWidth < len)
        fieldWidth = len;
    if (!leftPad) {
        for (unsigned i = 0; i < fieldWidth - len; ++i)
            putch(bufptr, ' ');
    }
    for (unsigned i = 0; i < len; ++i) {
        putch(bufptr, str[i]);
    }
    if (leftPad) {
        for (unsigned i = 0; i < fieldWidth - len; ++i)
            putch(bufptr, ' ');
    }
    return fieldWidth;
}

template<typename PutChFunc>
[[gnu::always_inline]] inline int print_signed_number(PutChFunc putch, char*& bufptr, int number, bool leftPad, bool zeroPad, u32 fieldWidth)
{
    if (number < 0) {
        putch(bufptr, '-');
        return print_number(putch, bufptr, 0 - number, leftPad, zeroPad, fieldWidth) + 1;
    }
    return print_number(putch, bufptr, number, leftPad, zeroPad, fieldWidth);
}

template<typename PutChFunc>
[[gnu::always_inline]] inline int printf_internal(PutChFunc putch, char* buffer, const char*& fmt, va_list ap)
{
    const char* p;

    int ret = 0;
    char* bufptr = buffer;

    for (p = fmt; *p; ++p) {
        bool left_pad = false;
        bool zeroPad = false;
        unsigned fieldWidth = 0;
        unsigned long_qualifiers = 0;
        bool size_qualifier = false;
        (void)size_qualifier;
        bool alternate_form = 0;
        if (*p == '%' && *(p + 1)) {
        one_more:
            ++p;
            if (*p == '-') {
                left_pad = true;
                if (*(p + 1))
                    goto one_more;
            }
            if (!zeroPad && !fieldWidth && *p == '0') {
                zeroPad = true;
                if (*(p + 1))
                    goto one_more;
            }
            if (*p >= '0' && *p <= '9') {
                fieldWidth *= 10;
                fieldWidth += *p - '0';
                if (*(p + 1))
                    goto one_more;
            }
            if (*p == '*') {
              fieldWidth = va_arg(ap, int);
              if (*(p + 1))
                goto one_more;
            }
            if (*p == 'l') {
                ++long_qualifiers;
                if (*(p + 1))
                    goto one_more;
            }
            if (*p == 'z') {
                size_qualifier = true;
                if (*(p + 1))
                    goto one_more;
            }
            if (*p == '#') {
                alternate_form = true;
                if (*(p + 1))
                    goto one_more;
            }
            switch (*p) {
            case 's': {
                const char* sp = va_arg(ap, const char*);
                ret += print_string(putch, bufptr, sp ? sp : "(null)", left_pad, fieldWidth);
            } break;

            case 'd':
                ret += print_signed_number(putch, bufptr, va_arg(ap, int), left_pad, zeroPad, fieldWidth);
                break;

            case 'u':
                ret += print_number(putch, bufptr, va_arg(ap, u32), left_pad, zeroPad, fieldWidth);
                break;

            case 'Q':
                ret += print_u64(putch, bufptr, va_arg(ap, u64), left_pad, zeroPad, fieldWidth);
                break;

            case 'q':
                ret += print_hex(putch, bufptr, va_arg(ap, u64), 16);
                break;

#ifndef KERNEL
            case 'g':
            case 'f':
                // FIXME: Print as float!
                ret += print_i64(putch, bufptr, (u64)va_arg(ap, double), left_pad, zeroPad, fieldWidth);
                break;
#endif

            case 'o':
                if (alternate_form) {
                    putch(bufptr, '0');
                    ++ret;
                }
                ret += print_octal_number(putch, bufptr, va_arg(ap, u32), left_pad, zeroPad, fieldWidth);
                break;

            case 'x':
                if (alternate_form) {
                    putch(bufptr, '0');
                    putch(bufptr, 'x');
                    ret += 2;
                }
                ret += print_hex(putch, bufptr, va_arg(ap, u32), 8);
                break;

            case 'w':
                ret += print_hex(putch, bufptr, va_arg(ap, int), 4);
                break;

            case 'b':
                ret += print_hex(putch, bufptr, va_arg(ap, int), 2);
                break;

            case 'c':
                putch(bufptr, (char)va_arg(ap, int));
                ++ret;
                break;

            case '%':
                putch(bufptr, '%');
                ++ret;
                break;

            case 'p':
                putch(bufptr, '0');
                putch(bufptr, 'x');
                ret += 2;
                ret += print_hex(putch, bufptr, va_arg(ap, u32), 8);
                break;
            }
        } else {
            putch(bufptr, *p);
            ++ret;
        }
    }
    return ret;
}
