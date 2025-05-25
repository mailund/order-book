import argparse
import random
from dataclasses import dataclass


@dataclass
class SimulatorState:
    """State of the simulator."""

    largest_id: int = 0


@dataclass
class CreateOrder:
    """Create order message."""

    side: str
    quantity: int
    price: float

    def __str__(self) -> str:
        return f"CREATE {self.side} {self.quantity} {self.price}"


@dataclass
class UpdateOrder:
    """Update order message."""

    order_id: int
    price: float

    def __str__(self) -> str:
        return f"UPDATE {self.order_id} {self.price}"


@dataclass
class RemoveOrder:
    """Remove order message."""

    order_id: int

    def __str__(self) -> str:
        return f"REMOVE {self.order_id}"


@dataclass
class Bids:
    def __str__(self) -> str:
        return "BIDS"


@dataclass
class Asks:
    def __str__(self) -> str:
        return "ASKS"


Event = CreateOrder | UpdateOrder | RemoveOrder | Bids | Asks


def sample_side(state: SimulatorState) -> str:
    """Sample a side for an order."""
    return ["Buy", "Sell"][random.choice([0, 1])]


def sample_quantity(state: SimulatorState) -> int:
    """Sample a quantity for an order."""
    return random.randint(1, 1000000)


def sample_price(state: SimulatorState) -> float:
    """Sample a price for an order."""
    sign = random.choice([-1, 1])
    price = random.randint(1, 10000)
    return sign * price


def sample_new_order(state: SimulatorState) -> CreateOrder:
    """Sample a new order."""
    side = sample_side(SimulatorState())
    quantity = sample_quantity(SimulatorState())
    price = sample_price(SimulatorState())
    state.largest_id += 1
    return CreateOrder(side, quantity, price)


def sample_update_order(state: SimulatorState) -> UpdateOrder:
    """Sample an update order."""
    order_id = random.randint(0, state.largest_id)
    price = sample_price(state)
    return UpdateOrder(order_id, price)


def sample_remove_order(state: SimulatorState) -> RemoveOrder:
    """Sample a remove order."""
    order_id = random.randint(0, state.largest_id)
    return RemoveOrder(order_id)


def sample_event(state: SimulatorState) -> Event:
    """Sample an event."""
    event_type = random.choice(
        [
            "CREATE",
            "UPDATE",
            "REMOVE",
            "BIDS",
            "ASKS",
        ]
    )
    match event_type:
        case "CREATE":
            return sample_new_order(state)
        case "UPDATE":
            return sample_update_order(state)
        case "REMOVE":
            return sample_remove_order(state)
        case "BIDS":
            return Bids()
        case "ASKS":
            return Asks()
        case _:
            raise ValueError(f"Unknown event type: {event_type}")


def main():
    parser = argparse.ArgumentParser(
        description="Simulates a sequence of order events.",
    )
    parser.add_argument(
        "-n",
        "--num-updates",
        type=int,
        default=10,
        help="Number of updates to simulate",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=argparse.FileType("w"),
        metavar="FILE",
        nargs="?",
        default="-",
        help="Output file for events",
    )
    args = parser.parse_args()

    state = SimulatorState()
    for _ in range(args.num_updates):
        print(sample_event(state), file=args.output)


if __name__ == "__main__":
    main()
