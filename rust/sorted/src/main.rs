use std::cmp::Ordering;
use std::env;
use std::io::{self, BufRead};

use shared::{parse_events, BuyCmp, Event, Order, OrderComparator, OrderType, SellCmp};

/// A little sorted collection over `Order`, parameterized by
/// a zero‐sized comparator `C`.
pub struct SortedOrders<C: OrderComparator + Default> {
    orders: Vec<Order>,
    _cmp: std::marker::PhantomData<C>,
}

impl<C: OrderComparator + Default> SortedOrders<C> {
    /// Start empty. We never actually store a `C` value—only its type.
    pub fn new() -> Self {
        SortedOrders {
            orders: Vec::new(),
            _cmp: std::marker::PhantomData,
        }
    }

    /// Insert one order, bubbling it into its correct position.
    pub fn insert(&mut self, order: Order) {
        let mut i = self.orders.len();
        self.orders.push(order);
        while i > 0 {
            let prev = i - 1;
            if C::cmp(&self.orders[i], &self.orders[prev]) == Ordering::Less {
                self.orders.swap(i, prev);
                i = prev;
            } else {
                break;
            }
        }
    }

    /// Update price by id, then re‐bubble.
    pub fn update(&mut self, order_id: i32, new_price: i32) {
        if let Some(i) = self.orders.iter().position(|o| o.order_id == order_id) {
            self.orders[i].price = new_price;
            self.reorder(i);
        }
    }

    /// Remove by id.
    pub fn remove(&mut self, order_id: i32) {
        if let Some(i) = self.orders.iter().position(|o| o.order_id == order_id) {
            self.orders.remove(i);
        }
    }

    /// Print in sorted order.
    pub fn print(&self) {
        for o in &self.orders {
            println!("\t{:?} {} {}", o.order_type, o.price, o.quantity);
        }
    }

    /// Empty?
    pub fn is_empty(&self) -> bool {
        self.orders.is_empty()
    }

    /// Re‐bubble the element at `idx` up or down.
    fn reorder(&mut self, mut idx: usize) {
        // bubble up
        while idx > 0 {
            let prev = idx - 1;
            if C::cmp(&self.orders[idx], &self.orders[prev]) == Ordering::Less {
                self.orders.swap(idx, prev);
                idx = prev;
            } else {
                break;
            }
        }
        // bubble down
        let len = self.orders.len();
        while idx + 1 < len {
            let next = idx + 1;
            if C::cmp(&self.orders[idx], &self.orders[next]) == Ordering::Greater {
                self.orders.swap(idx, next);
                idx = next;
            } else {
                break;
            }
        }
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let silent = args.contains(&"--silent".to_string());
    let input: Box<dyn BufRead> = Box::new(io::BufReader::new(io::stdin()));

    // Now these both carry no runtime data, only their type
    let mut buys: SortedOrders<BuyCmp> = SortedOrders::new();
    let mut sells: SortedOrders<SellCmp> = SortedOrders::new();

    let mut next_id = 0;
    for event in parse_events(input) {
        match event {
            Event::Create {
                order_type,
                price,
                quantity,
            } => {
                let o = Order::new(next_id, order_type, price, quantity);
                next_id += 1;
                if o.order_type == OrderType::Buy {
                    buys.insert(o);
                } else {
                    sells.insert(o);
                }
            }
            Event::Update { id, new_price } => {
                buys.update(id, new_price);
                sells.update(id, new_price);
            }
            Event::Remove { id } => {
                buys.remove(id);
                sells.remove(id);
            }
            Event::Bids => {
                if !buys.is_empty() && !silent {
                    println!("Bids");
                    buys.print();
                    println!();
                }
            }
            Event::Asks => {
                if !sells.is_empty() && !silent {
                    println!("Asks");
                    sells.print();
                    println!();
                }
            }
        }
    }
}
