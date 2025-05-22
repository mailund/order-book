# Order book exercise

The task is to implement a simple orderbook. The orderbook should be able to handle a stream of order events and maintain the state of the orderbook. The orderbook should be able to provide the state of the orderbook at any given time.

## Formal Definition

We define a limit order as an element of

```
Order = Id x Side x Quantity x Price
```

where

```
Id = Natural number ({1, 2, 3, ...})
Side = {Buy, Sell}
Quantity = Natural number
Price = Integers in the range (-1_000_000, 1_000_000)
```

An `OrderBook` contains a set of orders.


## Orders from Venue

Orders are received from a venue in the form of a stream of delta events.
The events delta are defined as follows

```haskell
-- Create a new order
createOrder (Side, Quantity, Price)

-- Update an existing order's price
updateOrder (Id, Price)

-- Remove an existing order
removeOrder (Id)
```

Implement operations handling these events:

```haskell
-- Create a new order and return its Id
createOrder :: OrderBook -> (Side, Quantity, Price) -> (OrderBook, Id)

-- Update an existing order's price and returns the updated order
updateOrder :: OrderBook -> (Id, Price) -> (OrderBook, Order)

-- Remove an existing order and return the removed order
removeOrder :: OrderBook -> Id -> (OrderBook, Order)
```

Implement functions

```haskell
bids :: OrderBook -> List[Order]
asks :: OrderBook -> List[Order]
```

where `bids` returns the list of buy orders and `asks` returns the list of sell orders. The orders should be sorted by price in descending order for bids and ascending order for asks.

