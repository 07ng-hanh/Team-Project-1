// utils.c - Utility functions for string manipulation and date handling.

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Safely copies a string with length limit.
// Args:
//   dest: Destination buffer.
//   src: Source string.
//   max_len: Maximum length to copy (excluding null terminator).
// Why use: Cannot copy a string (a string is a pointer to the first
// element of a char array) --> Copy to a buffer string.
void CopyString(char* dest, const char* src, int max_len) {
  strncpy(dest, src, max_len - 1);
  dest[max_len - 1] = '\0';
}

// Converts date string to integer representation.
// Format: "YYYY-MM-DD" becomes YYYYMMDD (e.g., "2022-05-07" -> 20220507).
// Args:
//   date: Date string in "YYYY-MM-DD" format.
// Returns: Integer representation of the date.
// Why use: Faster comparison than passing string with format YYYY-MM-DD
int DateToInt(const char* date) {
  int y = 0;
  int m = 0;
  int d = 0;
  sscanf(date, "%d-%d-%d", &y, &m, &d);
  return y * 10000 + m * 100 + d;
}
