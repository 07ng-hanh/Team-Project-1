#ifndef BACKTEST_H
#define BACKTEST_H
#include "stock.h"

void CalculateAllIndicators(Stock* stock);
void RunStrategy(int strategy_id, Stock* stock);
void SortStocksByProfit(Stock** head);

#endif