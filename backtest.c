// backtest.c

// This file contains the core logic of the stock trading simulator.
// It calculates technical indicators for every stock and then runs
// one of three chosen trading strategies on historical daily data.

#include "backtest.h"
#include "trade.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void CalculateAllIndicators(Stock* stock) {
  // This self-defined function calculates all technical indicators
  // needed by the three trading strategies for a single stock.
  // It walks through every daily price record (StockDay linked list)
  // and fills in fields like sma9, sma20, macd, histogram, Bollinger Bands, etc.
  // Called once per stock before any trading simulation begins.
  
  if (!stock || !stock->price_list) return;

  // use dynamically allocated arrays that grow with malloc/realloc as we process each day.
  // This stores the complete price and volume history up to the current day.
  // At the end of the function we call free() to prevent memory leaks.
  float* prices = NULL;
  float* volumes = NULL;
  int history_size = 0;
  

  StockDay* day = stock->price_list;
  int count = 0;

  float ema12 = 0.0f;
  float ema26 = 0.0f;
  float signal = 0.0f;

  // Alpha values are smoothing factors used in EMA calculations
  // EMA gives more weight to recent prices than older ones.
  // Formula: alpha = 2 / (period + 1)
  float alpha12 = 2.0f / 13.0f;
  float alpha26 = 2.0f / 27.0f;
  float alpha9 = 2.0f / 10.0f;

  while (day) {
    // EMA12 and EMA26 for MACD
    if (count == 0) {
      // On the very first day, we simply use the closing price as the starting EMA
      ema12 = day->close;
      ema26 = day->close;
    } else {
      // Formula: EMA = alpha * price + (1 - alpha) * previous_EMA
      ema12 = alpha12 * day->close + (1 - alpha12) * ema12;
      ema26 = alpha26 * day->close + (1 - alpha26) * ema26;
    }

    day->macd = ema12 - ema26;

    // Signal Line (SMA9 of MACD)
    if (count == 0) {
      signal = day->macd;
    } else {
      signal = alpha9 * day->macd + (1 - alpha9) * signal;
    }
    day->signal = signal;

    // Histogram = MACD Line - Signal Line
    // Positive and rising = bullish momentum
    // Negative and falling = bearish momentum
    // This is the key value used in Strategy 1.
    day->histogram = day->macd - day->signal;

    // Dynamically allocate and store
    // price and volume history
    history_size++;
    prices = (float*)realloc(prices, history_size * sizeof(float));
    volumes = (float*)realloc(volumes, history_size * sizeof(float));
    prices[history_size - 1] = day->close;
    volumes[history_size - 1] = day->volume;

    // Calculate SMA9 (for Strategy 2)
    // SMA is the arithmetic average of the last N prices.
    // Unlike EMA, every price has equal weight.
    if (history_size >= 1) {
      int n9 = (history_size < 9) ? history_size : 9;
      float sum9 = 0.0f;
      for (int i = 0; i < n9; i++) {
        sum9 += prices[history_size - 1 - i];   // most recent prices first
      }
      day->sma9 = sum9 / (float)n9; 
    } else {
      day->sma9 = day->close;
    }

    // Calculate SMA20 (shared for Strategy 2 and 3 - Bollinger Bands)
    if (history_size >= 1) {
      int n20 = (history_size < 20) ? history_size : 20;
      float sum20 = 0.0f;
      for (int i = 0; i < n20; i++) {
        sum20 += prices[history_size - 1 - i];
      }
      day->sma20 = sum20 / (float)n20;   // stored in sma20 field (but it is SMA20)
    } else {
      day->sma20 = day->close;
    }

    // Calculate Volume SMA20 (for Strategy 2)
    if (history_size >= 1) {
      int n20 = (history_size < 20) ? history_size : 20;
      float sum_vol = 0.0f;
      for (int i = 0; i < n20; i++) {
        sum_vol += volumes[history_size - 1 - i];
      }
      day->volume_sma20 = sum_vol / (float)n20;
    } else {
      day->volume_sma20 = 0.0f;
    }

    // Bollinger Bands (for Strategy 3) - uses shared SMA20 and its standard deviation
    // Bollinger Bands are volatility bands placed above and below a moving average.
    // Middle band = 20-day SMA
    // Upper band = middle + 2 * standard deviation
    // Lower band = middle - 2 * standard deviation
    // They expand when volatility increases and contract when volatility decreases.
    if (history_size >= 20) {
      float sma20 = day->sma20;   // already computed as SMA20
      float sum_sq_diff = 0.0f;
      for (int i = 0; i < 20; i++) {
        float diff = prices[history_size - 1 - i] - sma20;
        sum_sq_diff += diff * diff;
      }
      float variance = sum_sq_diff / 20.0f;
      float std_dev = sqrt(variance); // Standard deviation measures price volatility
      
      day->bb_middle = sma20;
      day->bb_upper = sma20 + (2.0f * std_dev);
      day->bb_lower = sma20 - (2.0f * std_dev);
    } else {
      // Not enough data yet - set all bands to current close price
      day->bb_middle = day->close;
      day->bb_upper = day->close;
      day->bb_lower = day->close;
    }

    count++;
    day = day->next;
    // move to next trading day in the linked list
  }

  // Clean up memory
  // free() releases the memory we allocated with realloc.
  // This prevents memory leaks when processing many stocks.
  free(prices);
  free(volumes);
}

void RunStrategy(int strategy_id, Stock* stock) {
  // This self-defined function executes one of the three trading strategies
  // on a single stock. It uses the indicators calculated above and calls
  // BuyStock() / SellStock() from trade.c when signals occur.
  // At the end it computes the final portfolio value including any open positions.
  if (!stock || !stock->price_list) return;

  stock->cash = 100000000.0f;
  CalculateAllIndicators(stock);
  // Pre-compute all technical indicators

  StockDay* prev_day = stock->price_list;
  // The first day
  if (prev_day == NULL) return;
  // No first day --> Nothing to do
  StockDay* curr_day = prev_day->next;
  // The second day (We need at least 2 days to calculate)

  while (curr_day) {
    switch (strategy_id) {
      case 1:  // MACD Histogram Reversal
        // Buy when histogram is negative but increasing (turning up)
        if (curr_day->histogram < 0 &&
            curr_day->histogram > prev_day->histogram) {
          BuyStock(stock, curr_day->close, curr_day->date,
                   curr_day->day_index);
        }
        // Sell when histogram is positive but decreasing (turning down)
        if (curr_day->histogram > 0 &&
            curr_day->histogram < prev_day->histogram) {
          SellStock(stock, curr_day->close, curr_day->date,
                    curr_day->day_index);
        }
        break;

      case 2:  // SMA Crossover + Volume Spike Strategy  (REWRITTEN)
        // Buy when SMA9 crosses above SMA20 AND volume is 50% above average
        // Sell when SMA9 crosses below SMA20
        if (curr_day->sma9 > curr_day->sma20 &&
            prev_day->sma9 <= prev_day->sma20 &&
            curr_day->volume > 1.5f * curr_day->volume_sma20) {
          BuyStock(stock, curr_day->close, curr_day->date,
                   curr_day->day_index);
        }
        if (curr_day->sma9 < curr_day->sma20 &&
            prev_day->sma9 >= prev_day->sma20) {
          SellStock(stock, curr_day->close, curr_day->date,
                    curr_day->day_index);
        }
        break;

      case 3:  // Breakout + Bollinger Bands
        if (curr_day->close > curr_day->bb_upper &&
            prev_day->close <= prev_day->bb_upper) {
          BuyStock(stock, curr_day->close, curr_day->date,
                   curr_day->day_index);
        }// Buy on breakout above upper Bollinger Band (strong upward volatility)
        // Sell when price falls back below middle band (mean reversion)
        if (curr_day->close < curr_day->bb_lower &&
            prev_day->close >= prev_day->bb_lower) {
          SellStock(stock, curr_day->close, curr_day->date,
                    curr_day->day_index);
        }
        break;
    }

    prev_day = curr_day;
    curr_day = curr_day->next;
    // Move to the next day
    // Stop when curr_day == NULL (the last day)
  }

  // Calculate final portfolio value
  // 1. Start with remaining cash
  stock->final_value = stock->cash;

  // 2. Find the last trading day's closing price
  StockDay* last_day = stock->price_list;
  while (last_day && last_day->next) {
    last_day = last_day->next;
  }
  float last_day_close = last_day ? last_day->close : 0.0f;

  // 3. Sum up any shares still held (BUY trades that were never sold)
  //    These are marked with buy_day_index != -1 in trade.c
  long long total_holding_quantity = 0;
  Trade* trade = stock->trade_stack;
  while (trade) {
    if (strcmp(trade->action, "BUY") == 0) {
      // Add quantity if this position hasn't been sold (buy_day_index != -1)
      if (trade->buy_day_index != -1) {
        total_holding_quantity += trade->quantity;
      }
    }
    trade = trade->next;
  }

  // 4. Value the open holdings at the final day's closing price
  // This gives unrealized profit/loss on positions still held at the end.
  if (total_holding_quantity > 0 && last_day_close > 0) {
    stock->final_value += total_holding_quantity * last_day_close * 1000;
  }

  // 5. Calculate overall profit percentage
  float profit = stock->final_value - 100000000.0f;
  stock->profit_pct = (profit / 100000000.0f) * 100.0f;
}

void SortStocksByProfit(Stock** head) {
  // This self-defined function sorts the linked list of stocks
  // from highest profit_pct to lowest using a simple bubble sort.
  // It is called once after all strategies have run on every stock.
  // The purpose is to make results.txt easy to read (best performers first).
  if (!*head) return;

  int swapped;
  Stock* ptr1;
  Stock* lptr = NULL; // Tracks the last sorted

  do {
    swapped = 0;
    ptr1 = *head;
    while (ptr1->next != lptr) {
      // if ptr1->profit_pct < ptr1->next->profit_pct THEN swap
      if (ptr1->profit_pct < ptr1->next->profit_pct) {
        // Swap only symbol, profit_pct and final_value
        char temp_sym[10];
        CopyString(temp_sym, ptr1->symbol, 10);
        CopyString(ptr1->symbol, ptr1->next->symbol, 10);
        CopyString(ptr1->next->symbol, temp_sym, 10);

        float tmp = ptr1->profit_pct;
        ptr1->profit_pct = ptr1->next->profit_pct;
        ptr1->next->profit_pct = tmp;

        tmp = ptr1->final_value;
        ptr1->final_value = ptr1->next->final_value;
        ptr1->next->final_value = tmp;

        swapped = 1;
      }
      ptr1 = ptr1->next;
      // Move to the next element
    }
    lptr = ptr1;
  } while (swapped);
}