// trade.h - Trading execution API definitions.

#ifndef TRADE_H
#define TRADE_H
#include "stock.h"

void BuyStock(Stock* stock, float price, const char* date, int day_index);
void SellStock(Stock* stock, float price, const char* date, int day_index);
int CanSell(Trade* trade, int current_day_index);

#endif