use fxhash::FxHashMap as HashMap;
use std::cmp::Ordering;
use std::env;
use std::io::{self, BufRead};
use std::marker::PhantomData;

use shared::{parse_events, BuyCmp, Event, Order, OrderComparator, OrderType, SellCmp};

/// Chunked, sorted order book with embedded id→Order map.
pub struct SortedOrders<C: OrderComparator, const MAX_CHUNK_SIZE: usize = 128> {
    /// Each chunk is individually sorted.
    chunks: Vec<Vec<Order>>,
    /// Map from order_id → Order, for O(1) lookup.
    map: HashMap<i32, Order>,
    _cmp: PhantomData<C>,
}

impl<C: OrderComparator, const MAX_CHUNK_SIZE: usize> SortedOrders<C, MAX_CHUNK_SIZE> {
    /// Create an empty book.
    pub fn new() -> Self {
        SortedOrders {
            chunks: Vec::new(),
            map: HashMap::default(),
            _cmp: PhantomData,
        }
    }

    #[cfg(debug_assertions)]
    pub fn debug_print_chunks(&self) {
        for (i, chunk) in self.chunks.iter().enumerate() {
            println!("Chunk {}:", i);
            for order in chunk {
                println!("\t{:?}", order);
            }
        }
    }

    /// Locate which chunk an order should go into.
    fn find_chunk(&self, order: &Order) -> usize {
        self.chunks
            .partition_point(|chunk| C::cmp(chunk.last().unwrap(), order) == Ordering::Less)
    }

    /// Split a chunk if it exceeds the maximum size.
    /// Returns the (new) index where the order belongs. This depends on the break point
    /// of the chunk and can either be the input idx or idx + 1 if we added a new
    /// chunk and the order is larger than the break point.
    fn maybe_split_chunk(&mut self, idx: usize, order: &Order) -> usize {
        if self.chunks[idx].len() >= MAX_CHUNK_SIZE {
            let chunk = &mut self.chunks[idx];
            let mid = chunk.len() / 2;

            // Perform the split manually to control capacity
            let mut sibling = Vec::with_capacity(MAX_CHUNK_SIZE);
            sibling.extend(chunk.drain(mid..));

            self.chunks.insert(idx + 1, sibling);

            // Decide where the order should go
            if C::cmp(self.chunks[idx].last().unwrap(), order) == Ordering::Less {
                return idx + 1;
            }
        }
        idx
    }

    /// Insert into a single chunk, keeping it sorted and splitting if too large.
    fn insert_in_chunk(&mut self, mut idx: usize, order: &Order) {
        if self.chunks.is_empty() {
            // The entire book is empty, so create a new single chunk for it.
            let mut chunk = Vec::with_capacity(MAX_CHUNK_SIZE);
            chunk.push(order.clone());
            self.chunks.push(chunk);
            return;
        }

        if idx == self.chunks.len() {
            // Order is the largest we have so far. It goes at the end of the last chunk
            // which is at idx - 1.
            idx = self.maybe_split_chunk(idx - 1, order);
            self.chunks[idx].push(order.clone()); // Insert at end of last chunk
        } else {
            //Perform a binary search to find the insertion point and then insert the element there.
            idx = self.maybe_split_chunk(idx, order);
            let pos = self.chunks[idx]
                // This search might be faster with linear search
                .binary_search_by(|o| C::cmp(o, order))
                .unwrap_or_else(|x| x);
            self.chunks[idx].insert(pos, order.clone());
        }
    }

    /// Insert an order into the appropriate chunk.
    fn remove_from_chunk(&mut self, idx: usize, order: &Order) {
        assert!(idx < self.chunks.len(), "chunk index out of bounds!");
        let chunk = &mut self.chunks[idx];
        if let Ok(pos) = chunk.binary_search_by(|o| C::cmp(o, order)) {
            chunk.remove(pos);
            if chunk.is_empty() {
                self.chunks.remove(idx);
            }
        }
    }

    /// Insert a new order into the book.
    pub fn insert(&mut self, order: &Order) {
        self.map.insert(order.order_id, order.clone());
        self.insert_in_chunk(self.find_chunk(order), order);
    }

    /// Remove an order by ID.
    pub fn remove_by_id(&mut self, order_id: i32) -> Option<Order> {
        // lookup in the map to get the existing Order, then remove it from its chunk
        self.map.remove(&order_id).map(|order| {
            self.remove_from_chunk(self.find_chunk(&order), &order);
            order // Return order for use in update.
        })
    }

    /// Update an order’s price.
    /// If there is an existing order with the given ID, remove it and then
    /// insert a new order with the updated price.
    pub fn update(&mut self, order_id: i32, new_price: i32) -> Option<()> {
        self.remove_by_id(order_id)
            .map(|order| self.insert(&order.update(new_price)))
    }

    /// Print all orders in ascending “chunks” order.
    pub fn print(&self) {
        for chunk in &self.chunks {
            for o in chunk {
                println!("\t{:?} {} {}", o.order_type, o.price, o.quantity);
            }
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
                    buys.insert(&o);
                } else {
                    sells.insert(&o);
                }
            }
            Event::Update { id, new_price } => {
                buys.update(id, new_price)
                    .or_else(|| sells.update(id, new_price));
            }
            Event::Remove { id } => {
                buys.remove_by_id(id).or_else(|| sells.remove_by_id(id));
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
