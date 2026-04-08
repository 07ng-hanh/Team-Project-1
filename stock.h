// stock.h - stock.c data structures and function definitions.

#ifndef STOCK_H
#define STOCK_H

typedef struct StockDay {
  char date[12]; // Date as a string (e.g. "2026-12-31")
  int day_index;
  // Used to keep track of the number of days holding a stock
  // Because: VNINDEX only allow to sell 2.5 days after buying
  // (here I round to 3 days).
  float open;
  float high;
  float low;
  float close;
  float volume;
  float ema9;
  float ema20;
  float bb_upper;
  float bb_lower;
  float bb_middle;
  float rsi;
  float volume_sma20;
  float macd;
  float signal;
  float histogram;
  struct StockDay* next;
} StockDay;
// StockDay is a linked list
// It stores daily data of a stock IN THE MARKET: 
// date, date_index, technical indicators

typedef struct Trade {
  char symbol[10]; // Stock symbol as a string (e.g. VIC, VHM,...)
  char date[12]; // Date as a string (e.g. 2026-12-31)
  float price; 
  int quantity; // BUY or SELL amount
  char action[10]; // BUY or SELL
  float pnl; // Stands for "Profit and Loss"
  float pnl_pct; // Percentage of pnl
  int buy_day_index; // Same as day_index
  struct Trade* next;
} Trade;
// Trade is a linked list
// It stores daily data of a stock IN THE PORTFOLIO

typedef struct Stock {
  char symbol[10]; // Stock ticker/symbol (e.g. VIC, VHM)
  StockDay* price_list; 
  // Pointer to StockDay linked list: all market price data for this stock
  Trade* trade_list; 
  // Pointer to Trade linked list: all buy/sell transactions made for this stock
  float cash; 
  // Cash allocated or remaining for this stock position
  float final_value; // Final portfolio value after all trades for this stock
  float profit_pct; // Total profit/loss percentage for this stock
  struct Stock* next; // Pointer to next stock in the portfolio
} Stock;
// Stock is a linked list
// It acts as a portfolio account for one stock:
// - Contains ALL market data (historical prices & indicators) via price_list (StockDay)
// - Contains ALL transactions (buy/sell orders) via trade_list (Trade)
// - Tracks cash and final profit for this individual stock

Stock* LoadStockData(const char* filename, const char* start_date,
                     const char* end_date);
// Load from the .csv file from start_date to end_date.
void FreeAllStocks(Stock* head);
// Free memory in the "Stock" linked list
#endif