#pragma once

#include <string>

/**
 * Read a double from a substring.
 * Format is [0x | 0b]nnn[.nnn][e[+-]nnn] with 0x for hex and 0b for binary. For hex numbers
 * the "e[+-]nnn" must be "q[+-]nnn" since 'e' is already used as a digit.
 * Note that this only reads nonnegative numbers (the '-' is read separately).
 *
 * @param s The entire string
 * @param i0 Start of substring
 * @param i1 End of substring (non-inclusive, i.e. i1==i0 is the empty string)
 * @param x The result if any
 *
 * @return Length of the read number or 0 if s[i0, ...] is not a numeric string.
 */

size_t readnum(const std::string &s, size_t i0, size_t i1, double &x); // read from s[i0..i1)

/**
 * Read a complex/integer exponent from a substring.
 * Format is [+-]([i]nnn | nnn[i]) ... in superscript unicode, no spaces
 *
 * @param s The entire string
 * @param i0 Start of substring
 * @param i1 End of substring (non-inclusive, i.e. i1==i0 is the empty string)
 * @param real_part,imag_part The result if any
 *
 * @return Length of the read number or 0 if s[i0, ...] is not a valid exponent.
 */

size_t readexp(const std::string &s, size_t i0, size_t i1, int &real_part, int &imag_part);


/**
 * Formats an exponent in superscript unicode.
 * Inverse of readexp.
 */

std::string format_exponent(int real_part, int imag_part = 0);

std::string format_subscript(int re);
