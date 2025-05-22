from dataclasses import dataclass


@dataclass
class CreateOrder:
    """Create order message."""

    side: str
    quantity: int
    price: int


@dataclass
class UpdateOrder:
    """Update order message."""

    order_id: int
    price: int


@dataclass
class RemoveOrder:
    """Remove order message."""

    order_id: int


@dataclass
class Bids:
    """Bids message."""


@dataclass
class Asks:
    """Asks message."""


Event = CreateOrder | UpdateOrder | RemoveOrder | Bids | Asks


def parse_event(event: str) -> Event:
    """Parse an event string into an event object."""
    # FIXME: Should be more defensive in a real program...
    parts = event.split()
    if parts[0] == "CREATE":
        if len(parts) != 4:
            raise ValueError(f"Invalid CREATE event: {event}")
        if parts[1] not in ["Buy", "Sell"]:
            raise ValueError(f"Invalid side in CREATE event: {parts[1]}")
        return CreateOrder(parts[1], int(parts[2]), int(parts[3]))
    elif parts[0] == "UPDATE":
        return UpdateOrder(int(parts[1]), int(parts[2]))
    elif parts[0] == "REMOVE":
        return RemoveOrder(int(parts[1]))
    elif parts[0] == "BIDS":
        return Bids()
    elif parts[0] == "ASKS":
        return Asks()
    else:
        raise ValueError(f"Unknown event type: {parts[0]}")
