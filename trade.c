// ==================== trade.c ====================
#include "trade.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Executes a BUY transaction at the specified price and date.
// Args:
//   stock: Pointer to Stock struct to execute trade on
//   price: Stock price per share (in VND)
//   date: Transaction date as string
//   day_index: Sequential trading day index for holding period calculation
void BuyStock(Stock* stock, float price, const char* date, int day_index) {
  // Validate: price must be positive and sufficient cash available
  if (price <= 0.0f || stock->cash < price * 1000.0f) return;

  // Use 100% of available cash per trade
  float allocation = stock->cash;
  int quantity = (int)(allocation / (price * 1000.0f));
  if (quantity < 1) return; // Not enough money

  // Deduct purchased amount from cash balance
  float actual_value = quantity * price * 1000.0f;
  stock->cash -= actual_value;

  // Allocate memory for new Trade record
  Trade* buy_trade = (Trade*)malloc(sizeof(Trade));
  if (!buy_trade) return;

  // CopyString: Safely copy strings with length protection
  CopyString(buy_trade->symbol, stock->symbol, 10);
  CopyString(buy_trade->date, date, 12);
  CopyString(buy_trade->action, "BUY", 10);
  // Initiate data for buy_trade
  buy_trade->price = price;
  buy_trade->quantity = quantity;
  // If BUY, pnl automatically = 0
  buy_trade->pnl = 0.0f;
  buy_trade->pnl_pct = 0.0f;
  
  // buy_day_index: Marks when position was opened (for 3-day holding rule)
  buy_trade->buy_day_index = day_index;
  buy_trade->next = NULL;

  // Append new trade to linked list
  if (stock->trade_list == NULL) {
    stock->trade_list = buy_trade;  // If first trade, set as head
  } else {
    Trade* t = stock->trade_list;
    while (t->next != NULL) t = t->next;  // Traverse to end of linked list
    t->next = buy_trade;  // Link new trade to tail
  }
}

// Checks if a BUY position is eligible for selling.
// Rule: Cannot sell until 3+ days after purchase.
// Args:
//   trade: Pointer to Trade record (BUY transaction)
//   current_day_index: Current trading day index
// Returns: 1 if sellable, 0 if not
int CanSell(Trade* trade, int current_day_index) {
  // Validate: trade must exist and be a BUY action
  // strcmp: String comparison (returns 0 if equal)
  if (!trade || strcmp(trade->action, "BUY") != 0) return 0;
  
  // buy_day_index = -1 means this position was already sold (marked for removal)
  if (trade->buy_day_index == -1) return 0;
  
  // Check holding period: (current day - buy day) >= 3
  return (current_day_index - trade->buy_day_index) >= 3;
}

// Executes a SELL transaction for the first eligible BUY position.
// Args:
//   stock: Pointer to Stock struct to execute trade on
//   price: Stock price per share at time of sale
//   date: Transaction date as string
//   day_index: Current trading day index
void SellStock(Stock* stock, float price, const char* date, int day_index) {
  // Validate: stock and trade list must exist
  if (!stock || !stock->trade_list) return;

  Trade* curr = stock->trade_list;

  // Iterate through trade list to find first sellable BUY position
  while (curr) {
    if (CanSell(curr, day_index)) {
      // Calculate sale proceeds and profit/loss
      float value = curr->quantity * price * 1000.0f;  // Total sale revenue
      // PnL = (Sell Price - Buy Price) * Quantity
      float pnl = (price - curr->price) * curr->quantity * 1000.0f;
      stock->cash += value;  // Add sale revenue to cash balance

      // Create new Trade record to log the SELL transaction
      Trade* sell_trade = (Trade*)malloc(sizeof(Trade));
      if (sell_trade) {      
        CopyString(sell_trade->symbol, stock->symbol, 10);
        CopyString(sell_trade->date, date, 12);
        sell_trade->price = price;  // Sale price per share
        sell_trade->quantity = curr->quantity;  // Quantity sold
        CopyString(sell_trade->action, "SELL", 10);
        sell_trade->pnl = pnl;  // Store profit/loss amount
        
        // Calculate return percentage: (PnL / Cost) * 100
        if (curr->price > 0) {
          sell_trade->pnl_pct = (pnl / (curr->price * curr->quantity * 1000.0f)) * 100.0f;
        }
        
        sell_trade->buy_day_index = 0;  // Not applicable for SELL records
        sell_trade->next = NULL;

        // Append SELL trade to linked list
        if (!stock->trade_list)
          stock->trade_list = sell_trade;
        else {
          Trade* t = stock->trade_list;
          while (t->next) t = t->next;
          t->next = sell_trade;
        }
      }

      // Mark original BUY position as sold (prevents duplicate sells)
      // Setting buy_day_index = -1 flags this position as closed
      curr->buy_day_index = -1;
      
      // Exit after first successful sell (one sell per function call)
      return;
    }
    curr = curr->next;  // Move to next trade in list
  }
}