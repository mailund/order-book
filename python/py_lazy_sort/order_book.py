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
        self.__sorted: list[Order] = []
        self.__waiting: dict[int, Order] = {}

    def __contains__(self, order_id: int) -> bool:
        """Check if an order exists by ID."""
        # O(1)
        return order_id in self.__sorted_index or order_id in self.__waiting

    def __getitem__(self, order_id: int) -> Order:
        """Get an order by ID."""
        # O(1)
        if order_id in self.__waiting:
            return self.__waiting[order_id]
        elif order_id in self.__sorted_index:
            return self.__sorted[self.__sorted_index[order_id]]
        raise UnknownOrder(order_id)

    def __setitem__(self, order_id: int, order: Order) -> None:
        """Set an order by ID."""
        # O(1)
        if order_id in self.__sorted_index:
            del self.__sorted_index[order_id]
        self.__waiting[order_id] = order

    def __delitem__(self, order_id: int) -> None:
        """Delete an order by ID."""
        # O(1)
        if order_id in self.__waiting:
            del self.__waiting[order_id]
        if order_id in self.__sorted_index:
            del self.__sorted_index[order_id]

    def __merge(self, x: list[Order], y: list[Order]) -> list[Order]:
        """Merge two sorted lists."""
        merged: list[Order] = []
        i, j = 0, 0
        while i < len(x) and j < len(y):
            if self.__key(x[i]) < self.__key(y[j]):
                merged.append(x[i])
                i += 1
            else:
                merged.append(y[j])
                j += 1
        merged.extend(x[i:])
        merged.extend(y[j:])
        return merged

    def items(self) -> list[Order]:
        """Update the sorted list."""
        # O(n log n), or
        #
        # O(n[dirty] + n[sorted]          -- for cleaning dirty
        #   + n[waiting] log n[waiting]   -- for sorting waiting
        #   + n[sorted] + n[waiting])     -- for merging
        #
        # but with a sizable overhead for the Python implementation

        clean_sorted = [o for o in self.__sorted if o.order_id in self.__sorted_index]
        waiting = sorted(self.__waiting.values(), key=self.__key)
        merged = self.__merge(clean_sorted, waiting)

        self.__sorted = merged
        self.__sorted_index = {o.order_id: i for i, o in enumerate(self.__sorted)}
        self.__waiting = {}

        return self.__sorted


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
