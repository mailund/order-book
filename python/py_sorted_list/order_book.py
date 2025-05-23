from dataclasses import dataclass
from typing import Any
from typing import Callable as Fn

from sortedcontainers import SortedList  # type: ignore[import]


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


class SortedTable:
    def __init__(self, key: Fn[[Order], Any]) -> None:
        self.__order_dict: dict[int, Order] = {}
        self.__order_list: SortedList = SortedList(key=key)

    def __contains__(self, order_id: int) -> bool:
        """Check if an order exists by ID."""
        return order_id in self.__order_dict

    def __getitem__(self, order_id: int) -> Order:
        try:
            return self.__order_dict[order_id]
        except KeyError:
            raise UnknownOrder(order_id)

    def __setitem__(self, order_id: int, order: Order) -> None:
        if order_id in self.__order_dict:
            old_order = self.__order_dict[order_id]
            self.__order_list.discard(old_order)  # type: ignore[no-untyped-call]
        self.__order_dict[order_id] = order
        self.__order_list.add(order)  # type: ignore[no-untyped-call]

    def __delitem__(self, order_id: int) -> None:
        if order_id not in self.__order_dict:
            raise UnknownOrder(order_id)
        order = self.__order_dict.pop(order_id)
        self.__order_list.discard(order)  # type: ignore[no-untyped-call]

    def items(self) -> list[Order]:
        return list(self.__order_list)


class OrderBook:
    def __init__(self) -> None:
        self.__buy_orders = SortedTable(lambda x: (-x.price, -x.quantity))
        self.__sell_orders = SortedTable(lambda x: (x.price, x.quantity))
        self.__largest_id: int = -1

    def create_order(self, order_type: str, price: int, quantity: int) -> int:
        """Create a new order."""
        self.__largest_id += 1
        tbl = self.__buy_orders if order_type == "Buy" else self.__sell_orders
        tbl[self.__largest_id] = Order(self.__largest_id, order_type, price, quantity)
        return self.__largest_id

    def _get_order(self, order_id: int) -> tuple[SortedTable, Order]:
        """Get an order by ID."""
        if order_id in self.__buy_orders:
            return self.__buy_orders, self.__buy_orders[order_id]
        elif order_id in self.__sell_orders:
            return self.__sell_orders, self.__sell_orders[order_id]
        else:
            raise UnknownOrder(order_id)

    def update_order(self, order_id: int, price: int) -> Order:
        """Update an existing order."""
        tbl, order = self._get_order(order_id)
        del tbl[order_id]
        order.price = price
        tbl[order_id] = order
        return order

    def remove_order(self, order_id: int) -> Order:
        """Remove an order."""
        tbl, order = self._get_order(order_id)
        del tbl[order_id]
        return order

    def bids(self) -> list[Order]:
        """Get all bids."""
        return self.__buy_orders.items()

    def asks(self) -> list[Order]:
        """Get all asks."""
        return self.__sell_orders.items()
