// fileio.c - File I/O and output formatting functionality.
// Handles saving trade logs and backtest results to files with proper
// formatting.

#include "fileio.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Formats a number with thousands separators.
// Example: 1234567 becomes "1,234,567".
// Args:
//   buffer: Output string buffer for formatted number.
//   num: Numeric value to format.
// No return. Paste the formatted number to *buffer as string.
static void FormatNumber(char* buffer, double num) {
  long long value = (long long)num;
  bool negative = 0;
  if (value < 0) {
    negative = 1;
    value = -value;
  }

  char temp[32];
  int i = 0;
  int j = 0;

  do {
    if (i > 0 && i % 3 == 0) temp[j++] = ',';
    temp[j++] = (value % 10) + '0';
    value /= 10;
    i++;
  } while (value > 0);
  // Add thousands separators but in reverse (due to % 10).
  // e.g. 1234567 -> 765,432,1
  temp[j] = '\0';

  // Reverse the string
  for (int left = 0, right = j - 1; left < right; left++, right--) {
    char c = temp[left];
    temp[left] = temp[right];
    temp[right] = c;
  }
  // e.g. 765,432,1 -> 1,234,567

  if (negative) {
    sprintf(buffer, "-%s", temp);
  } else {
    strcpy(buffer, temp);
  }
  // Copy the temp string to buffer
}

void SaveTradeLog(Stock* head) {
  FILE* fp = fopen("trade_log.txt", "w");
  if (!fp) {
    printf("Cannot create trade_log.txt\n");
    return;
  }

  fprintf(fp, "===== TRADE LOG DETAIL =====\n\n");

  // Header
  fprintf(fp, "%-7s %-8s %-12s %10s %12s %18s %18s %10s\n", "Action",
          "Symbol", "Date", "Price", "Quantity", "Value (VND)", "PnL (VND)",
          "PnL%%");
  // %-7s prints a string with at least 7 characters wide, padding to the left
  // %-7s prints a string with at least 7 characters wide, padding to the right
  // Mainly used for beautiful format


  fprintf(fp,
          "-----------------------------------"
          "-----------------------------------\n");

  char num_str[32];

  Stock* s = head;
  while (s) {
    Trade* t = s->trade_list;
    while (t) {
      fprintf(fp, "%-7s %-8s %-12s %10.2f %12d ", t->action, t->symbol,
              t->date, t->price, t->quantity);

      // Value (VND)
      FormatNumber(num_str, t->quantity * t->price * 1000.0);
      fprintf(fp, "%18s ", num_str);

      // PnL (VND)
      FormatNumber(num_str, t->pnl);
      fprintf(fp, "%18s ", num_str);

      // PnL %
      fprintf(fp, "%9.2f%%\n", t->pnl_pct);

      t = t->next;
    }
    s = s->next;
  }

  fclose(fp);
  printf("Trade log saved to trade_log.txt\n");
}

void SaveResults(Stock* head) {
  FILE* fp = fopen("results.txt", "w");
  if (!fp) {
    printf("Cannot create results.txt\n");
    return;
  }

  fprintf(fp, "===== BACKTEST RESULTS (Sorted by Profit) =====\n\n");

  char num_str[32];
  Stock* s = head;
  while (s) {
    fprintf(fp, "%-6s : %8.2f%%   |   Final Value: ", s->symbol,
            s->profit_pct);
    FormatNumber(num_str, s->final_value);
    // No need to pass &num_str because num_str is an array of char,
    // which decays to a pointer to its first element
    fprintf(fp, "%s VND\n", num_str);
    s = s->next;
  }
  fclose(fp);
  printf("Results saved to results.txt\n");
}