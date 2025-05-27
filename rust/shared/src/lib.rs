use std::cmp::Ordering;
use std::io::BufRead;
use std::marker::PhantomData;
use std::panic;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OrderType {
    Buy,
    Sell,
}

#[derive(Debug, Clone)]
pub struct Order {
    pub order_id: i32,
    pub order_type: OrderType,
    pub price: i32,
    pub quantity: i32,
}

impl Order {
    pub fn new(order_id: i32, order_type: OrderType, price: i32, quantity: i32) -> Self {
        Self {
            order_id,
            order_type,
            price,
            quantity,
        }
    }

    pub fn update(&self, new_price: i32) -> Order {
        // Create a new Order with the updated price, keeping other fields the same
        Order {
            order_id: self.order_id,
            order_type: self.order_type,
            price: new_price,
            quantity: self.quantity,
        }
    }
}

pub enum Event {
    Create {
        order_type: OrderType,
        price: i32,
        quantity: i32,
    },
    Update {
        id: i32,
        new_price: i32,
    },
    Remove {
        id: i32,
    },
    Bids,
    Asks,
}

pub fn parse_events<T: BufRead>(reader: T) -> impl Iterator<Item = Event> {
    reader.lines().map(|line| {
        let line = line.expect("Failed to read line");
        let tokens: Vec<&str> = line.trim().split_whitespace().collect();
        match tokens.as_slice() {
            ["CREATE", side, quantity, price] => {
                let order_type = if *side == "Buy" {
                    OrderType::Buy
                } else {
                    OrderType::Sell
                };
                Event::Create {
                    order_type,
                    price: price.parse().expect("Invalid price"),
                    quantity: quantity.parse().expect("Invalid quantity"),
                }
            }
            ["UPDATE", id, new_price] => Event::Update {
                id: id.parse().expect("Invalid id"),
                new_price: new_price.parse().expect("Invalid price"),
            },
            ["REMOVE", id] => Event::Remove {
                id: id.parse().expect("Invalid id"),
            },
            ["BIDS"] => Event::Bids,
            ["ASKS"] => Event::Asks,
            _ => panic!("Invalid command: {:?}", tokens),
        }
    })
}

/// A comparator trait for Orders.  Must be `Default` so we can do `C::default()`.
pub trait OrderComparator: Default {
    fn cmp(a: &Order, b: &Order) -> Ordering;
}

/// A ZST comparator for descending (buy) order: primary by price desc, secondary by quantity desc.
#[derive(Clone, Copy, Default)]
pub struct BuyCmp;

impl OrderComparator for BuyCmp {
    fn cmp(a: &Order, b: &Order) -> Ordering {
        // we want highest price first, then highest quantity
        b.price
            .cmp(&a.price)
            .then_with(|| b.quantity.cmp(&a.quantity))
            .then_with(|| a.order_id.cmp(&b.order_id))
    }
}

/// A ZST comparator for ascending (sell) order: price asc, then quantity asc.
#[derive(Clone, Copy, Default)]
pub struct SellCmp;

impl OrderComparator for SellCmp {
    fn cmp(a: &Order, b: &Order) -> Ordering {
        a.price
            .cmp(&b.price)
            .then_with(|| a.quantity.cmp(&b.quantity))
            .then_with(|| a.order_id.cmp(&b.order_id))
    }
}

/// A tiny newtype around `Order` that implements `Ord` via `C::cmp`.
#[derive(Clone)]
pub struct CmpOrder<C: OrderComparator>(pub Order, pub PhantomData<C>);

impl<C: OrderComparator> PartialEq for CmpOrder<C> {
    fn eq(&self, other: &Self) -> bool {
        // we also tie‐break on order_id to ensure a total order
        self.0.order_id == other.0.order_id
    }
}
impl<C: OrderComparator> Eq for CmpOrder<C> {}

impl<C: OrderComparator> PartialOrd for CmpOrder<C> {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}
impl<C: OrderComparator> Ord for CmpOrder<C> {
    fn cmp(&self, other: &Self) -> Ordering {
        // first use the user‐provided comparator...
        let primary = C::cmp(&self.0, &other.0);
        if primary != Ordering::Equal {
            primary
        } else {
            // then tie‐break by id so we never treat two distinct orders as equal
            self.0.order_id.cmp(&other.0.order_id)
        }
    }
}
