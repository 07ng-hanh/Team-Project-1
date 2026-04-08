// backtest.c - Backtesting engine with technical indicator calculations.
// Implements three trading strategies using MACD, Moving Average Crossover,
// and Bollinger Bands indicators.

#include "backtest.h"
#include "trade.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void CalculateAllIndicators(Stock* stock) {
  if (!stock || !stock->price_list) return;

  // Circular buffers for SMA calculations (shared across strategies)
  float prices_buffer[20];  // For SMA9 and SMA20
  float volumes_buffer[20]; // For volume SMA20
  int buffer_index = 0;
  int buffer_size = 0;

  StockDay* day = stock->price_list;
  int count = 0;

  float ema12 = 0.0f;
  float ema26 = 0.0f;
  float signal = 0.0f;

  float alpha12 = 2.0f / 13.0f;
  float alpha26 = 2.0f / 27.0f;
  float alpha9 = 2.0f / 10.0f;

  while (day) {
    // EMA12 and EMA26 for MACD
    if (count == 0) {
      ema12 = day->close;
      ema26 = day->close;
    } else {
      ema12 = alpha12 * day->close + (1 - alpha12) * ema12;
      ema26 = alpha26 * day->close + (1 - alpha26) * ema26;
    }

    day->macd = ema12 - ema26;

    // Signal Line (EMA9 of MACD)
    if (count == 0) {
      signal = day->macd;
    } else {
      signal = alpha9 * day->macd + (1 - alpha9) * signal;
    }
    day->signal = signal;

    // Histogram
    day->histogram = day->macd - day->signal;

    // Add prices to circular buffer for SMA calculations
    prices_buffer[buffer_index] = day->close;
    volumes_buffer[buffer_index] = day->volume;
    buffer_index = (buffer_index + 1) % 20;
    if (buffer_size < 20) buffer_size++;

    // Calculate SMA9 (for Strategy 2)
    if (count >= 8) {  // At least 9 data points (count starts at 0)
      float sum9 = 0.0f;
      int count9 = (buffer_size < 9) ? buffer_size : 9;
      for (int i = 0; i < count9; i++) {
        int idx = (buffer_index + 20 - 1 - i) % 20;
        sum9 += prices_buffer[idx];
      }
      day->ema9 = sum9 / count9;
    } else {
      day->ema9 = day->close;
    }

    // Calculate SMA20 (shared for Strategy 2 and 3 - Bollinger Bands)
    if (count >= 19) {  // At least 20 data points
      float sum20 = 0.0f;
      for (int i = 0; i < 20; i++) {
        sum20 += prices_buffer[i];
      }
      day->ema20 = sum20 / 20.0f;
    } else {
      day->ema20 = day->close;
    }

    // Calculate Volume SMA20 (for Strategy 2)
    if (count >= 19) {  // At least 20 data points
      float sum_vol = 0.0f;
      for (int i = 0; i < 20; i++) {
        sum_vol += volumes_buffer[i];
      }
      day->volume_sma20 = sum_vol / 20.0f;
    } else {
      day->volume_sma20 = 0.0f;
    }

    // Bollinger Bands (for Strategy 3) - uses shared SMA20 and its standard deviation
    if (count >= 19) {  // At least 20 data points
      // Calculate standard deviation from the prices in buffer
      float sma20 = day->ema20;  // Use the SMA20 we just calculated
      float sum_sq_diff = 0.0f;
      
      for (int i = 0; i < 20; i++) {
        float diff = prices_buffer[i] - sma20;
        sum_sq_diff += diff * diff;
      }
      float variance = sum_sq_diff / 20.0f;
      float std_dev = sqrt(variance);
      
      day->bb_middle = sma20;
      day->bb_upper = sma20 + (2.0f * std_dev);
      day->bb_lower = sma20 - (2.0f * std_dev);
    } else {
      // Not enough data yet
      day->bb_middle = day->close;
      day->bb_upper = day->close;
      day->bb_lower = day->close;
    }

    count++;
    day = day->next;
  }
}

void RunStrategy(int strategy_id, Stock* stock) {
  if (!stock || !stock->price_list) return;

  stock->cash = 100000000.0f;
  CalculateAllIndicators(stock);

  StockDay* prev_day = stock->price_list;
  if (!prev_day) return;
  StockDay* curr_day = prev_day->next;

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

      case 2:  // MA Crossover + Volume Spike
        if (curr_day->ema9 > curr_day->ema20 &&
            prev_day->ema9 <= prev_day->ema20 &&
            curr_day->volume > 1.5f * curr_day->volume_sma20) {
          BuyStock(stock, curr_day->close, curr_day->date,
                   curr_day->day_index);
        }
        if (curr_day->ema9 < curr_day->ema20 &&
            prev_day->ema9 >= prev_day->ema20) {
          SellStock(stock, curr_day->close, curr_day->date,
                    curr_day->day_index);
        }
        break;

      case 3:  // Breakout + Bollinger Bands
        if (curr_day->close > curr_day->bb_upper &&
            prev_day->close <= prev_day->bb_upper) {
          BuyStock(stock, curr_day->close, curr_day->date,
                   curr_day->day_index);
        }
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
  }

  // Calculate final portfolio value
  stock->final_value = stock->cash;

  // Find the last trading day's closing price
  StockDay* last_day = stock->price_list;
  while (last_day && last_day->next) {
    last_day = last_day->next;
  }
  float last_day_close = last_day ? last_day->close : 0.0f;

  // Sum all open holding quantities (BUY trades that haven't been sold)
  long long total_holding_quantity = 0;
  Trade* trade = stock->trade_list;
  while (trade) {
    if (strcmp(trade->action, "BUY") == 0) {
      // Add quantity if this position hasn't been sold (buy_day_index != -1)
      if (trade->buy_day_index != -1) {
        total_holding_quantity += trade->quantity;
      }
    }
    trade = trade->next;
  }

  // Value all open holdings at last trading day close price
  if (total_holding_quantity > 0 && last_day_close > 0) {
    stock->final_value += total_holding_quantity * last_day_close * 1000;
  }

  float profit = stock->final_value - 100000000.0f;
  stock->profit_pct = (profit / 100000000.0f) * 100.0f;
}

void SortStocksByProfit(Stock** head) {
  if (!*head) return;

  int swapped;
  Stock* ptr1;
  Stock* lptr = NULL;

  do {
    swapped = 0;
    ptr1 = *head;
    while (ptr1->next != lptr) {
      if (ptr1->profit_pct < ptr1->next->profit_pct) {
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
    }
    lptr = ptr1;
  } while (swapped);
}