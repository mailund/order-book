from dataclasses import dataclass


@dataclass
class Order:
    """Order message."""

    order_id: int
    order_type: str
    price: int
    quantity: int

    def __str__(self) -> str:
        return f"{self.order_type} {self.price} {self.quantity}"


class OrderBookError(Exception):
    """Custom exception for OrderBook errors."""

    def __init__(self, message: str) -> None:
        super().__init__(message)
        self.message = message


class UnknownOrder(OrderBookError):
    """Exception raised when an order is not found."""

    def __init__(self, order_id: int) -> None:
        super().__init__(f"Order with ID {order_id} not found.")
        self.order_id = order_id

    def __str__(self) -> str:
        return f"UnknownOrder: {self.message}"


class OrderBook:
    def __init__(self) -> None:
        self.__orders: dict[int, Order] = {}
        self.__largest_id: int = -1

    def create_order(self, order_type: str, price: int, quantity: int) -> Order:
        """Create a new order."""
        self.__largest_id += 1
        order = Order(self.__largest_id, order_type, price, quantity)
        self.__orders[self.__largest_id] = order
        return order

    def update_order(self, order_id: int, price: int) -> Order:
        """Update an existing order."""
        if order_id not in self.__orders:
            raise UnknownOrder(order_id)
        order = self.__orders[order_id]
        order.price = price
        return order

    def remove_order(self, order_id: int) -> Order:
        """Remove an order."""
        if order_id not in self.__orders:
            raise UnknownOrder(order_id)
        order = self.__orders[order_id]
        del self.__orders[order_id]
        return order

    def bids(self) -> list[Order]:
        """Get all bids."""
        orders = [
            order for order in self.__orders.values() if order.order_type == "Buy"
        ]
        return sorted(orders, key=lambda x: x.price, reverse=True)

    def asks(self) -> list[Order]:
        """Get all asks."""
        orders = [
            order for order in self.__orders.values() if order.order_type == "Sell"
        ]
        return sorted(orders, key=lambda x: x.price)
