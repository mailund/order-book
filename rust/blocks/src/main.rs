use std::cmp::Ordering;
use std::env;
use std::io::{self, BufRead};
use std::marker::PhantomData;

use shared::{parse_events, BuyCmp, Event, Order, OrderComparator, OrderType, SellCmp};

/// Single-chunk sorted collection.
pub struct SortedOrders<C: OrderComparator> {
    chunks: Vec<Vec<Order>>,
    max_chunk_size: usize,
    _cmp: PhantomData<C>,
}

impl<C: OrderComparator> SortedOrders<C> {
    /// Start with exactly one empty chunk.
    pub fn new() -> Self {
        SortedOrders {
            chunks: vec![],
            max_chunk_size: 2, // FIXME: arbitrary chunk size
            _cmp: PhantomData,
        }
    }

    fn locate_chunk_index(chunks: &[Vec<Order>], order: &Order) -> usize {
        // Find the chunk where the order should go. This is the first chunk
        // where the last order in the chunk is greater than the new order.
        // If no such chunk exists, return the length of the chunks.
        chunks
            .iter()
            .position(|chunk| {
                chunk
                    .last()
                    .map_or(false, |last| C::cmp(last, order) == Ordering::Greater)
            })
            .unwrap_or(chunks.len())
    }

    fn locate_by_id<'a>(
        chunks: &'a mut [Vec<Order>],
        order_id: i32,
    ) -> Option<(&'a mut Vec<Order>, usize)> {
        for chunk in chunks.iter_mut() {
            if let Some(pos) = chunk.iter().position(|order| order.order_id == order_id) {
                return Some((chunk, pos));
            }
        }
        None
    }

    /// Insert into chunk, shifting up to keep sorted.
    fn insert_into_chunk(chunk: &mut Vec<Order>, order: Order) {
        chunk.push(order);
        let mut i = chunk.len() - 1;
        while i > 0 && C::cmp(&chunk[i], &chunk[i - 1]) == Ordering::Less {
            chunk.swap(i, i - 1);
            i -= 1;
        }
    }

    pub fn insert(&mut self, order: Order) {
        let idx = Self::locate_chunk_index(&self.chunks, &order);
        if idx >= self.chunks.len() {
            // If no suitable chunk found, create a new one
            self.chunks.push(vec![order]);
            return;
        }

        Self::insert_into_chunk(&mut self.chunks[idx], order);

        // Split chunk if necessary
        if self.chunks[idx].len() >= self.max_chunk_size {
            // If chunk is full, split it into two and replace the existing chunk
            let mid = self.chunks[idx].len() / 2;
            let second_chunk = self.chunks[idx].split_off(mid);
            self.chunks.insert(idx + 1, second_chunk); // FIXME: Index
        }
    }

    /// Find by id in chunk[0], update price, then reorder that slot.
    pub fn update(&mut self, order_id: i32, new_price: i32) {
        if let Some((chunk, idx)) = Self::locate_by_id(&mut self.chunks, order_id) {
            let mut order = chunk[idx].clone();
            self.remove(order_id);
            order.price = new_price;
            self.insert(order);
        }
    }

    /// Remove by id in chunk[0].
    pub fn remove(&mut self, order_id: i32) {
        if let Some((chunk, idx)) = Self::locate_by_id(&mut self.chunks, order_id) {
            chunk.remove(idx);
            if chunk.is_empty() {
                // If the chunk is empty after removal, remove it
                self.chunks.retain(|c| !c.is_empty());
            }
        }
    }

    /// Print chunk[0].
    pub fn print(&self) {
        for chunk in &self.chunks {
            for order in chunk {
                println!(
                    "\t{:?} {} {}",
                    order.order_type, order.price, order.quantity
                );
            }
        }
    }

    /// Empty?
    pub fn is_empty(&self) -> bool {
        self.chunks.is_empty() || self.chunks.iter().all(|chunk| chunk.is_empty())
    }
}

fn main() {
    // parse args
    let args: Vec<String> = env::args().collect();
    let silent = args.contains(&"--silent".to_string());
    let input: Box<dyn BufRead> = Box::new(io::BufReader::new(io::stdin()));

    // instantiate buy/sell sorted containers
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
