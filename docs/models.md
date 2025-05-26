# Models and Algorithms

This document provides detailed information about the models and algorithms used in the Trade Simulator.

## Almgren-Chriss Market Impact Model

The Almgren-Chriss model is a framework for optimal execution that balances the trade-off between market impact and timing risk. It divides price impact into two components:

1. **Permanent Impact**: Affects all future trades and remains after the execution is complete
2. **Temporary Impact**: Only affects the current trade and dissipates immediately

### Model Parameters

The model uses the following parameters:

- `permanentImpactFactor`: The coefficient for the permanent impact component
- `temporaryImpactFactor`: The coefficient for the temporary impact component
- `volatility`: Market volatility (standard deviation of returns)
- `timeHorizon`: Time horizon for execution (in seconds)
- `riskAversion`: Risk aversion parameter (higher values prefer faster execution)

### Implementation

Our implementation uses the following formula for total market impact:

```
total_impact = permanent_impact + temporary_impact
```

where:

- `permanent_impact = gamma * orderSize * (1 + volatility)`
- `temporary_impact = epsilon * orderSize^0.5 * liquidityFactor * imbalanceFactor`

The liquidityFactor and imbalanceFactor adjust the impact based on current market conditions.

### Optimal Execution Schedule

For large orders, the model calculates an optimal execution schedule that minimizes the total expected cost. This schedule is determined by:

1. Market impact costs
2. Timing risk (volatility)
3. Risk aversion of the trader

A hyperbolic trading strategy is implemented, which gives more weight to early trades when volatility is high or the trader is risk-averse.

## Slippage Estimation Model

Slippage is estimated using a linear regression model that considers:

1. Relative order size compared to available liquidity
2. Market volatility
3. Order book imbalance

The model formula is:

```
slippage = intercept + (volumeFactor * relativeSizeToDepth) + (volatilityFactor * priceVolatility) + (imbalanceFactor * (orderImbalance - 1.0))
```

A minimum slippage equal to half the spread is enforced to reflect realistic market conditions.

## Maker/Taker Proportion Model

For market orders, we estimate the proportion that might be filled as maker vs. taker using a logistic regression approach. For market orders, this is typically very low (close to 0), but can be higher in certain market conditions.

The model considers:

1. Relative order size compared to market depth
2. Market volatility

The implementation uses an exponential decay formula:

```
makerProportion = exp(-5.0 * relativeSizeToDepth) * exp(-2.0 * priceVolatility)
```

This value is clamped to the range [0, 0.1] for market orders, as they rarely achieve more than 10% maker proportion.

## Fee Model

The fee model is rule-based and depends on:

1. Fee tier (determined by trading volume or VIP level)
2. Maker vs. taker proportion of the order

Fees are calculated as:

```
fees = (notionalValue * makerProportion * makerFeeRate) + (notionalValue * takerProportion * takerFeeRate)
```

where notionalValue is the product of order size and price. 