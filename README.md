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

The stream of events is formatted as

```
CREATE Sell 821016 960816
UPDATE 0 -475438
UPDATE 0 273043
UPDATE 1 614347
UPDATE 0 622029
UPDATE 1 328684
CREATE Buy 20672 -730328
UPDATE 2 572979
REMOVE 0
REMOVE 2
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

There can be references to IDs that have either not been created yet or have been removed. The system should handle these gracefully. The system should not crash or throw an error. Instead, it should ignore the event and continue processing the rest of the stream.


Interspersed with the events will be querys of either

```
BIDS
```

or

```
ASKS
```

Implement functions

```haskell
bids :: OrderBook -> List[Order]
asks :: OrderBook -> List[Order]
```

where `bids` returns the list of buy orders and `asks` returns the list of sell orders. The orders should be sorted by price in descending order for bids and ascending order for asks.

When the events contain a `BIDS` query, and the `bids` query is non-empty, write `Bids` and the Side, Price and Quantity of the orders from the `bids` query, correctly sorted. When the events contain an `ASKS` query, write `Asks` and the Side, Price and Quantity of the orders from the `asks` query, correctly sorted.

Example output:

```
Asks
	Sell -59661 483886
	Sell 257327 654506

Asks
	Sell 57961 955277

Bids
	Buy 820192 501963

Bids
	Buy 820192 501963
	Buy -979546 451316

Asks
	Sell -203727 145187
	Sell 57961 955277
```