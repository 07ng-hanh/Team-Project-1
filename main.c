// main.c - Stock trading strategy simulator entry point.
// Provides user interface for selecting stocks, date ranges, and trading
// strategies, then runs backtests and saves results.

#include "backtest.h"
#include "fileio.h"
#include "stock.h"
#include "trade.h"
#include <stdio.h>
#include <stdlib.h>

// Main entry point for the simulator.
int main() {
  printf("===== STOCK TRADING STRATEGY SIMULATOR =====\n\n");
  printf("Welcome to the Stock Trading Strategy Simulator!\n\n");
  printf("How it works:\n");
  printf("Starting Capital: 100,000,000 VND per symbol\n");
  printf("You will select: Stock group, date range, and trading strategy\n");
  printf("We will backtest each symbol in your chosen group\n");
  printf("Results will be saved to trade_log.txt and results.txt\n\n");

  // Choose stock group
  int group;
  printf("Choose stock group:\n");
  printf("1. VN30\n");
  printf("2. VNMidcap\n");
  printf("Enter choice (1 or 2): ");
  scanf("%d", &group);

  const char* filename =
      (group == 1) ? "vnstock_vn30_historical_data.csv"
                   : "vnstock_vnmidcap_historical_data.csv";

  // Choose time range
  char start_date[12];
  char end_date[12];
  printf("\nEnter start date (no earlier than 2022-01-01) (yyyy-mm-dd): ");
  scanf("%s", start_date);
  printf("Enter end date (no later than 2026-04-03) (yyyy-mm-dd): ");
  scanf("%s", end_date);

  // Choose strategy (1-3 only)
  int strategy;
  printf("\nChoose strategy:\n");
  printf("1. MACD Histogram Reversal\n");
  printf("2. MA Crossover + Volume Spike\n");
  printf("3. Breakout + Bollinger Bands\n");
  printf("Enter choice (1-3): ");
  scanf("%d", &strategy);
  if (strategy < 1 || strategy > 3) strategy = 1;
  // If input is out of range -> return to default (1)

  printf("\nLoading data and running Strategy %d...\n\n", strategy);

  Stock* stocks = LoadStockData(filename, start_date, end_date);
  if (!stocks) {
    printf("No data loaded. Exiting.\n");
    return 1;
  }

  Stock* s = stocks;
  while (s) {
    RunStrategy(strategy, s);
    s = s->next;
  }
  // Run each node (symbol), then move to the next one

  SortStocksByProfit(&stocks);

  SaveTradeLog(stocks);
  SaveResults(stocks);

  printf("\nCompleted! Check trade_log.txt and results.txt\n");

  FreeAllStocks(stocks);
  // Free memory
  return 0;
}