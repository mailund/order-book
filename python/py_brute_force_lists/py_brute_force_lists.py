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
    parser.add_argument(
        "-s",
        "--silent",
        action="store_true",
        help="Suppress output for bids and asks.",
    )

    args = parser.parse_args()

    order_book = OrderBook()
    for event in args.events_file:
        match events.parse_event(event.strip()):
            case events.CreateOrder(side, quantity, price):
                _new_id = order_book.create_order(side, price, quantity)

            case events.UpdateOrder(order_id, price):
                try:
                    order = order_book.update_order(order_id, price)
                except UnknownOrder:
                    pass

            case events.RemoveOrder(order_id):
                try:
                    order = order_book.remove_order(order_id)
                except UnknownOrder:
                    pass

            case events.Bids():
                bids = order_book.bids()
                if not bids or args.silent:
                    continue
                print(f"Bids")
                for order in bids:
                    print(f"\t{order}")
                print()

            case events.Asks():
                asks = order_book.asks()
                if not asks or args.silent:
                    continue
                print(f"Asks")
                for order in asks:
                    print(f"\t{order}")
                print()

            case _:
                print(f"Unknown event: {event.strip()}")


if __name__ == "__main__":
    main()
