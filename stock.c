// ==================== stock.c ====================
#include "stock.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Loads historical stock data from CSV file within a date range.
// Args:
//   filename: Path to CSV file containing stock data.
//   start_date: Start date in format "YYYY-MM-DD".
//   end_date: End date in format "YYYY-MM-DD".
// Returns: Pointer to linked list of Stock structs, or NULL on failure.
Stock* LoadStockData(const char* filename, const char* start_date,
                     const char* end_date) {
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    printf("Cannot open CSV file: %s\n", filename);
    return NULL;
  }

  // DateToInt: Utility function that converts "YYYY-MM-DD" to YYYYMMDD integer
  // (faster for date comparisons)
  int start_int = DateToInt(start_date);
  int end_int = DateToInt(end_date);

  char line[256];
  fgets(line, sizeof(line), fp);  // skip header

  Stock* head = NULL;
  Stock* last = NULL;
  int total_loaded = 0;

  // Read each line from CSV file
  while (fgets(line, sizeof(line), fp)) {
    char symbol[10];
    char time[12];
    float o, h, l, c, v;
    // Stands for open, high, low, close, volume
    
    // sscanf: Parse CSV formatted string
    // %3[^,]: Extract up to 3 chars before comma (stock symbol)
    // %10[^,]: Extract up to 10 chars before comma (date)
    // %f: Extract float values (o, h, l, c, v)
    if (sscanf(line, "%3[^,],%10[^,],%f,%f,%f,%f,%f", symbol, time, &o,
               &h, &l, &c, &v) != 7) {
      continue;
      // If < 7 rows are parsed -> skip
    }

    int current_int = DateToInt(time);
    if (current_int < start_int || current_int > end_int) continue;

    // Search for existing stock entry by symbol
    // strcmp: String comparison utility (returns 0 if strings match)
    Stock* s = head;
    while (s && strcmp(s->symbol, symbol) != 0) s = s->next;

    // If stock not found, create new stock node
    if (!s) {
      s = (Stock*)malloc(sizeof(Stock));  // Allocate memory for Stock struct
      if (!s) continue;
      CopyString(s->symbol, symbol, 10);  
      // Cannot directly assign s->symbol = symbol
      // because they're pointers
      s->price_list = NULL;
      s->trade_list = NULL;
      s->cash = 0.0f;
      s->final_value = 0.0f;
      s->profit_pct = 0.0f;
      s->next = NULL;

      if (!head)
        head = s;
      else
        last->next = s;
      last = s;
    }

    // Add day
    StockDay* day = (StockDay*)malloc(sizeof(StockDay));
    if (!day) continue;

    CopyString(day->date, time, 12);

    day->day_index = 0;  // Will be set properly after loading
    day->open = o;
    day->high = h;
    day->low = l;
    day->close = c;
    day->volume = v;
    
    // Technical indicators (initialized to 0, computed later by other functions)
    day->ema9 = day->ema20 = 0.0f;
    // EMA = Exponential Moving Average (9/20-day trend indicators)
    
    day->bb_upper = day->bb_lower = day->bb_middle = 0.0f;
    // Bollinger Bands = Volatility bands around moving average (trading signal)
    
    day->volume_sma20 = 0.0f;
    // SMA = Simple Moving Average of volume (20-day average trading volume)
    
    day->macd = day->signal = day->histogram = 0.0f;
    // MACD = Moving Average Convergence Divergence (momentum indicator)
    // - macd: Fast indicator line
    // - signal: Slow signal line
    // - histogram: Difference between MACD and signal
    
    day->next = NULL;

    // Add new date (new StockDay node) to the end of the price_list linked list
    if (s->price_list == NULL)
      s->price_list = day; 
      // If empty -> set the day as the first day
    else {
      StockDay* temp = s->price_list;
      while (temp->next != NULL){
        temp = temp->next;
      } 
      temp->next = day;
    }
    // If not empty: Add the new date to the last date (last node)
    total_loaded++;
  }

  fclose(fp);

  if (total_loaded == 0) {
    printf("No data found in the selected date range.\n");
    return NULL;
  }

  // Assign sequential day_index to each price_list
  // --> Calculate on day_index is easier than with char date[10]
  // (day_index tracks trading days for position holding rules)
  Stock* s = head;
  while (s != NULL) {
    StockDay* d = s->price_list;
    int idx = 0;
    while (d) {
      d->day_index = idx++;
      d = d->next;
    }
    s = s->next;
  }

  printf("Data loaded successfully (%d records).\n", total_loaded);
  return head;
}

// Frees all dynamically allocated memory for stocks and their data.
// Args:
//   head: Pointer to head of stock linked list.
// Purpose: Prevent memory leaks by deallocating all linked list nodes
void FreeAllStocks(Stock* head) {
  while (head) {
    Stock* s = head;
    head = head->next;

    // Free all StockDay nodes in price_list
    StockDay* d = s->price_list;
    while (d) {
      StockDay* tmp = d;
      d = d->next;
      free(tmp); // Free memory
    }

    // Free all Trade nodes in trade_list
    Trade* t = s->trade_list;
    while (t) {
      Trade* tmp = t;
      t = t->next;
      free(tmp);
    }

    // Free the Stock node itself
    free(s);
  }
}