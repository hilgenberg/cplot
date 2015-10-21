#pragma once

#include <string>

/**
 * Read a constant from a substring.
 * Format is const(<name>) or const_<name>
 *
 * @param s The entire string
 * @param i0 Start of substring
 * @param i1 End of substring (non-inclusive, i.e. i1==i0 is the empty string)
 * @param x The constant's value if any
 *
 * @return Length of the read constant or 0 if none found.
 */

size_t readconst(const std::string &s, size_t i0, size_t i1, double &x); // read from s[i0..i1)

/**
 * Find the name of a constant.
 * Inverse of readconst.
 *
 * @param value The number to look up
 *
 * @return Its name (without the "const_" part) or the empty string if not found.
 */

std::string const_name(double value);
