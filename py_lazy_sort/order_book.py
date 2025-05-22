import heapq
from dataclasses import dataclass
from typing import Any
from typing import Callable as Fn


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
        self.__key: Fn[[Order], Any] = key
        # the tables maps order_id to indices in the sorted and waiting lists
        self.__sorted_index: dict[int, int] = {}
        self.__waiting_index: dict[int, int] = {}
        self.__sorted: list[Order] = []
        self.__waiting: list[Order] = []

    def __contains__(self, order_id: int) -> bool:
        """Check if an order exists by ID."""
        return order_id in self.__sorted_index or order_id in self.__waiting_index

    def __getitem__(self, order_id: int) -> Order:
        """Get an order by ID."""
        if order_id in self.__waiting_index:
            return self.__waiting[self.__waiting_index[order_id]]
        elif order_id in self.__sorted_index:
            return self.__sorted[self.__sorted_index[order_id]]
        raise UnknownOrder(order_id)

    def __setitem__(self, order_id: int, order: Order) -> None:
        """Set an order by ID."""
        if order_id in self.__waiting_index:
            self.__waiting[self.__waiting_index[order_id]] = order
        elif order_id in self.__sorted_index:
            del self.__sorted_index[order_id]
            self.__waiting.append(order)
            self.__waiting_index[order_id] = len(self.__waiting) - 1
        else:
            self.__waiting.append(order)
            self.__waiting_index[order_id] = len(self.__waiting) - 1

    def __delitem__(self, order_id: int) -> None:
        """Delete an order by ID."""
        if order_id in self.__waiting_index:
            del self.__waiting_index[order_id]
        if order_id in self.__sorted_index:
            del self.__sorted_index[order_id]

    def __update(self) -> None:
        """Update the sorted list."""
        x = [
            (self.__key(o), o)
            for o in self.__sorted
            if o.order_id in self.__sorted_index
        ]
        y = [
            (self.__key(o), o)
            for o in self.__waiting
            if o.order_id in self.__waiting_index
        ]
        all_sorted = list(o for _, o in heapq.merge(x, sorted(y)))
        self.__sorted = all_sorted  # type: ignore
        self.__sorted_index = {o.order_id: i for i, o in enumerate(all_sorted)}
        self.__waiting = []
        self.__waiting_index = {}

    def items(self) -> list[Order]:
        """Iterate over the sorted orders."""
        self.__update()
        return self.__sorted  # type: ignore


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
