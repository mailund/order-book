import sqlite3
from dataclasses import dataclass
from sqlite3 import Error
from typing import Any, Self


@dataclass
class Order:
    """Order message."""

    order_id: int
    order_type: str
    price: int
    quantity: int

    def __str__(self) -> str:
        return f"{self.order_type} {self.price} {self.quantity}"

    @classmethod
    def from_row(cls, row: tuple[int, str, int, int]) -> Self:
        """
        Create an Order instance from a database row.
        """
        return cls(
            # SQLite uses 1-based indexing; we use 0-based indexing
            order_id=row[0] - 1,
            order_type=row[1],
            price=row[2],
            quantity=row[3],
        )


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
    def __init__(self, db_file: str = "order_book.db") -> None:
        try:
            self.__conn = sqlite3.connect(db_file)
        except Error as e:
            print(f"Error connecting to database: {e}")
            raise
        self.__create_order_book_table()

    def __execute_sql(self, sql: str, params: tuple[Any, ...] = ()) -> None:
        """
        Execute a SQL command with optional parameters.
        """
        try:
            cursor = self.__conn.cursor()
            cursor.execute(sql, params)
            self.__conn.commit()
        except Error as e:
            print(f"Error executing SQL: {e}")
            raise

    def __create_order_book_table(self) -> None:
        """
        Create the order_book table in the SQLite database.
        """
        create_table_sql = """
        CREATE TABLE IF NOT EXISTS order_book (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_type TEXT NOT NULL CHECK(order_type IN ('Buy', 'Sell')),
            price INTEGER NOT NULL,
            quantity INTEGER NOT NULL
        );
        """
        self.__execute_sql(create_table_sql)
        self.__execute_sql(
            """
            CREATE INDEX IF NOT EXISTS idx_order_type_price 
            ON order_book(order_type, price);
            """
        )

    def __del__(self):
        if self.__conn:
            self.__conn.close()

    def __enter__(self) -> Self:
        return self

    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> None:
        if self.__conn:
            self.__conn.close()
        if exc_type is not None:
            print(f"Exception occurred: {exc_val}")
            raise exc_val

    def create_order(self, order_type: str, price: int, quantity: int) -> int:
        """
        Create a new order in the order_book table.
        """
        insert_sql = """
        INSERT INTO order_book (order_type, price, quantity)
        VALUES (?, ?, ?);
        """
        cursor = self.__conn.cursor()
        cursor.execute(insert_sql, (order_type, price, quantity))
        self.__conn.commit()
        assert cursor.lastrowid is not None, "Failed to get last row ID"
        return cursor.lastrowid - 1  # Convert to 0-based indexing

    def get_order(self, order_id: int) -> Order:
        """
        Get an order from the order_book table by its ID.
        """
        select_sql = """
        SELECT id, order_type, price, quantity
        FROM order_book
        WHERE id = ?;
        """
        cursor = self.__conn.cursor()
        # SQLite uses 1-based indexing
        cursor.execute(select_sql, (order_id + 1,))
        row = cursor.fetchone()
        if not row:
            raise UnknownOrder(order_id)
        return Order.from_row(row)

    def update_order(self, order_id: int, price: int) -> Order:
        """
        Update an existing order in the order_book table.
        """
        update_sql = """
        UPDATE order_book
        SET price = ?
        WHERE id = ?;
        """
        self.__execute_sql(update_sql, (price, order_id + 1))
        cursor = self.__conn.cursor()
        cursor.execute(
            "SELECT id, order_type, price, quantity FROM order_book WHERE id = ?",
            (order_id + 1,),
        )
        return self.get_order(order_id)

    def remove_order(self, order_id: int) -> Order:
        """
        Remove an order from the order_book table.
        """
        delete_sql = """
        DELETE FROM order_book
        WHERE id = ?;
        """
        cursor = self.__conn.cursor()
        order = self.get_order(order_id)
        cursor.execute(delete_sql, (order_id + 1,))
        self.__conn.commit()

        return order

    def bids(self) -> list[Order]:
        """
        Get all buy orders from the order_book table.
        """
        select_sql = """
        SELECT id, order_type, price, quantity
        FROM order_book
        WHERE order_type = 'Buy'
        ORDER BY price DESC, quantity DESC;
        """
        cursor = self.__conn.cursor()
        cursor.execute(select_sql)
        rows = cursor.fetchall()
        return [Order.from_row(row) for row in rows]

    def asks(self) -> list[Order]:
        """
        Get all sell orders from the order_book table.
        """
        select_sql = """
        SELECT id, order_type, price, quantity
        FROM order_book
        WHERE order_type = 'Sell'
        ORDER BY price ASC, quantity ASC;
        """
        cursor = self.__conn.cursor()
        cursor.execute(select_sql)
        rows = cursor.fetchall()
        return [Order.from_row(row) for row in rows]

    def all_orders(self) -> list[Order]:
        """
        Get all orders from the order_book table sorted by id.
        """
        select_sql = """
        SELECT id, order_type, price, quantity
        FROM order_book
        ORDER BY id ASC;
        """
        cursor = self.__conn.cursor()
        cursor.execute(select_sql)
        rows = cursor.fetchall()
        return [Order.from_row(row) for row in rows]
