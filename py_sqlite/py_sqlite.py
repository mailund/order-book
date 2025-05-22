import argparse

import events
from order_book import OrderBook, UnknownOrder


def main():
    parser = argparse.ArgumentParser(
        description="Process a sequence of order events.",
    )
    parser.add_argument(
        "events_file",
        type=argparse.FileType("r"),
        nargs="?",
        default="-",
        help="File containing the events.",
    )

    args = parser.parse_args()

    order_book = OrderBook()
    for event in args.events_file:
        match events.parse_event(event.strip()):
            case events.CreateOrder(side, quantity, price):
                new_id = order_book.create_order(side, price, quantity)
                print(f"Creating order: {side} {quantity} @ {price} with ID {new_id}")

            case events.UpdateOrder(order_id, price):
                print(f"Updating order {order_id} to price {price}")
                try:
                    order = order_book.update_order(order_id, price)
                    print(f"Updated order: {order}")
                except UnknownOrder as e:
                    print(f"Error: {e}")

            case events.RemoveOrder(order_id):
                print(f"Removing order {order_id}")
                try:
                    order = order_book.remove_order(order_id)
                    print(f"Removed order: {order}")
                except UnknownOrder as e:
                    print(f"Error: {e}")

            case _:
                print(f"Unknown event: {event.strip()}")

    print("\nOrder Book:")
    for order in order_book.all_orders():
        print(order)
    print()

    print("Bids:")
    for order in order_book.bids():
        print(order)
    print()

    print("Asks:")
    for order in order_book.asks():
        print(order)
    print()


if __name__ == "__main__":
    main()
