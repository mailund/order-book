use std::cmp::Ordering;
use std::env;
use std::fs::File;
use std::io::{self, BufRead, BufReader};
use std::marker::PhantomData;

use shared::{parse_events, BuyCmp, Event, Order, OrderComparator, OrderType, SellCmp};

/// Single-chunk sorted collection.
pub struct SortedOrders<C: OrderComparator> {
    chunks: Vec<Vec<Order>>,
    _cmp: PhantomData<C>,
}

impl<C: OrderComparator> SortedOrders<C> {
    /// Start with exactly one empty chunk.
    pub fn new() -> Self {
        SortedOrders {
            chunks: vec![Vec::new()],
            _cmp: PhantomData,
        }
    }

    /// Insert into chunk[0], shifting up to keep sorted.
    pub fn insert(&mut self, order: Order) {
        let chunk = &mut self.chunks[0];
        chunk.push(order);
        let mut i = chunk.len() - 1;
        let cmp = C::default();
        // bubble upward
        while i > 0 && cmp.cmp(&chunk[i], &chunk[i - 1]) == Ordering::Less {
            chunk.swap(i, i - 1);
            i -= 1;
        }
    }

    /// Find by id in chunk[0], update price, then reorder that slot.
    pub fn update(&mut self, order_id: i32, new_price: i32) {
        let chunk = &mut self.chunks[0];
        if let Some(idx) = chunk.iter().position(|o| o.order_id == order_id) {
            chunk[idx].price = new_price;
            Self::reorder_chunk::<C>(chunk, idx);
        }
    }

    /// Remove by id in chunk[0].
    pub fn remove(&mut self, order_id: i32) {
        let chunk = &mut self.chunks[0];
        if let Some(idx) = chunk.iter().position(|o| o.order_id == order_id) {
            chunk.remove(idx);
        }
    }

    /// Print chunk[0].
    pub fn print(&self) {
        for order in &self.chunks[0] {
            println!(
                "\t{:?} {} {}",
                order.order_type, order.price, order.quantity
            );
        }
    }

    /// Empty?
    pub fn is_empty(&self) -> bool {
        self.chunks[0].is_empty()
    }

    /// Helper to bubble an element at `index` up or down to restore ordering.
    fn reorder_chunk<Cmp: OrderComparator>(chunk: &mut [Order], mut index: usize) {
        let cmp = Cmp::default();
        // bubble up
        while index > 0 && cmp.cmp(&chunk[index], &chunk[index - 1]) == Ordering::Less {
            chunk.swap(index, index - 1);
            index -= 1;
        }
        // bubble down
        let len = chunk.len();
        while index + 1 < len && cmp.cmp(&chunk[index], &chunk[index + 1]) == Ordering::Greater {
            chunk.swap(index, index + 1);
            index += 1;
        }
    }
}

fn main() {
    // parse args
    let args: Vec<String> = env::args().collect();
    let silent = args.contains(&"--silent".to_string());

    // open input
    let reader: Box<dyn BufRead> = if args.len() > 1 && args[1] != "--silent" {
        Box::new(BufReader::new(
            File::open(&args[1]).expect("cannot open file"),
        ))
    } else {
        Box::new(BufReader::new(io::stdin()))
    };

    // instantiate buy/sell sorted containers
    let mut buys: SortedOrders<BuyCmp> = SortedOrders::new();
    let mut sells: SortedOrders<SellCmp> = SortedOrders::new();

    let mut next_id = 0;
    for event in parse_events(reader) {
        match event {
            Event::Create {
                order_type,
                price,
                quantity,
            } => {
                let order = Order::new(next_id, order_type, price, quantity);
                next_id += 1;
                if order.order_type == OrderType::Buy {
                    buys.insert(order);
                } else {
                    sells.insert(order);
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
