use std::collections::BTreeSet;
use std::collections::HashMap;
use std::env;
use std::io::{self, BufRead};
use std::marker::PhantomData;

use shared::{parse_events, BuyCmp, CmpOrder, Event, Order, OrderComparator, OrderType, SellCmp};

/// Chunked, sorted order book with embedded id→Order map.
pub struct SortedOrders<C: OrderComparator> {
    tree: BTreeSet<CmpOrder<C>>,
    map: HashMap<i32, Order>,
    _cmp: PhantomData<C>,
}

impl<C: OrderComparator> SortedOrders<C> {
    /// Create an empty book.
    pub fn new() -> Self {
        SortedOrders {
            tree: BTreeSet::new(),
            map: HashMap::new(),
            _cmp: PhantomData,
        }
    }

    /// Insert a new order into the book.
    pub fn insert(&mut self, order: Order) {
        self.map.insert(order.order_id, order.clone());
        self.tree.insert(CmpOrder::<C>(order, PhantomData));
    }

    /// Remove an order by id.
    pub fn remove_by_id(&mut self, order_id: i32) -> Option<Order> {
        // lookup in map to get the existing Order, then remove it from its chunk
        self.map.remove(&order_id).map(|order| {
            self.tree.remove(&CmpOrder::<C>(order.clone(), PhantomData));
            order // Return order for use in update.
        })
    }

    /// Update an order’s price.
    /// If there is an existing order with the given id, remove it and then
    /// insert a new order with the updated price.
    pub fn update(&mut self, order_id: i32, new_price: i32) {
        self.remove_by_id(order_id)
            .map(|order| self.insert(order.update(new_price)));
    }

    /// Print all orders in ascending “chunks” order.
    pub fn print(&self) {
        for CmpOrder(ref o, _) in &self.tree {
            println!("\t{:?} {} {}", o.order_type, o.price, o.quantity);
        }
    }

    /// True iff empty.
    pub fn is_empty(&self) -> bool {
        self.map.is_empty()
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let silent = args.contains(&"--silent".to_string());
    let input: Box<dyn BufRead> = {
        if let Some(pos) = args.iter().position(|a| a == "-i" || a == "--input") {
            let file = std::fs::File::open(&args[pos + 1]).expect("Failed to open input file");
            Box::new(io::BufReader::new(file))
        } else {
            Box::new(io::BufReader::new(io::stdin()))
        }
    };

    let mut buys = SortedOrders::<BuyCmp>::new();
    let mut sells = SortedOrders::<SellCmp>::new();

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
                buys.remove_by_id(id);
                sells.remove_by_id(id);
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
