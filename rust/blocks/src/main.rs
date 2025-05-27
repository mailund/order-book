use std::cmp::Ordering;
use std::env;
use std::io::{self, BufRead};
use std::marker::PhantomData;

use shared::{parse_events, BuyCmp, Event, Order, OrderComparator, OrderType, SellCmp};

/// Chunked, sorted order book.
pub struct SortedOrders<C: OrderComparator> {
    chunks: Vec<Vec<Order>>,
    max_chunk_size: usize,
    _cmp: PhantomData<C>,
}

impl<C: OrderComparator> SortedOrders<C> {
    /// Start with no chunks; the first insert will create one.
    pub fn new() -> Self {
        SortedOrders {
            chunks: Vec::new(),
            max_chunk_size: 1024, // FIXME: Adapt this
            _cmp: PhantomData,
        }
    }

    /// Find the chunk index where `order` belongs (first chunk whose last > order),
    /// or `chunks.len()` if it belongs at the end.
    fn find_chunk(&self, order: &Order) -> usize {
        self.chunks
            .iter()
            .position(|chunk| {
                chunk
                    .last()
                    .map_or(false, |last| C::cmp(last, order) == Ordering::Greater)
            })
            .unwrap_or(self.chunks.len())
    }

    /// Find and remove an order by ID from whichever chunk it’s in,
    /// returning it.  Returns `None` if not found.
    fn take_by_id(&mut self, order_id: i32) -> Option<Order> {
        for i in 0..self.chunks.len() {
            if let Some(pos) = self.chunks[i].iter().position(|o| o.order_id == order_id) {
                let order = self.chunks[i].remove(pos);
                if self.chunks[i].is_empty() {
                    self.chunks.remove(i);
                }
                return Some(order);
            }
        }
        None
    }

    /// Insert `order` into chunk `idx`, keeping that chunk sorted,
    /// splitting it if it grows too large.
    fn insert_in_chunk(&mut self, idx: usize, order: Order) {
        // if idx==len, push new chunk; else insert
        if idx == self.chunks.len() {
            self.chunks.push(vec![order]);
            return;
        }
        let chunk = &mut self.chunks[idx];
        chunk.push(order);
        // bubble up
        let mut j = chunk.len() - 1;
        while j > 0 && C::cmp(&chunk[j], &chunk[j - 1]) == Ordering::Less {
            chunk.swap(j, j - 1);
            j -= 1;
        }

        // split if needed
        if chunk.len() > self.max_chunk_size {
            let mid = chunk.len() / 2;
            let sibling = chunk.split_off(mid);
            self.chunks.insert(idx + 1, sibling);
        }
    }

    /// Public insert.
    pub fn insert(&mut self, order: Order) {
        let idx = self.find_chunk(&order);
        self.insert_in_chunk(idx, order);
    }

    /// Public update: remove, adjust, re-insert.
    pub fn update(&mut self, order_id: i32, new_price: i32) {
        if let Some(mut ord) = self.take_by_id(order_id) {
            ord.price = new_price;
            let idx = self.find_chunk(&ord);
            self.insert_in_chunk(idx, ord);
        }
    }

    /// Public remove.
    pub fn remove(&mut self, order_id: i32) {
        let _ = self.take_by_id(order_id);
    }

    /// Print all chunks in order.
    pub fn print(&self) {
        for chunk in &self.chunks {
            for o in chunk {
                println!("\t{:?} {} {}", o.order_type, o.price, o.quantity);
            }
        }
    }

    /// Empty if no chunks or all chunks empty.
    pub fn is_empty(&self) -> bool {
        self.chunks.iter().all(Vec::is_empty)
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let silent = args.contains(&"--silent".to_string());
    let input: Box<dyn BufRead> = Box::new(io::BufReader::new(io::stdin()));

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
