use std::cmp::Ordering;
use std::collections::HashMap;
use std::env;
use std::io::{self, BufRead};
use std::marker::PhantomData;

use shared::{parse_events, BuyCmp, Event, Order, OrderComparator, OrderType, SellCmp};

/// Chunked, sorted order book with embedded id→Order map.
pub struct SortedOrders<C: OrderComparator> {
    /// Each chunk is individually sorted.
    chunks: Vec<Vec<Order>>,
    /// Map from order_id → Order, for O(1) lookup.
    map: HashMap<i32, Order>,
    /// Maximum number of orders per chunk before splitting.
    max_chunk_size: usize,
    _cmp: PhantomData<C>,
}

impl<C: OrderComparator> SortedOrders<C> {
    /// Create an empty book.
    pub fn new(max_chunk_size: usize) -> Self {
        SortedOrders {
            chunks: Vec::new(),
            map: HashMap::new(),
            max_chunk_size,
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

    /// Insert into a single chunk, keeping it sorted and splitting if too large.
    fn insert_in_chunk(&mut self, idx: usize, order: Order) {
        if idx == self.chunks.len() {
            // The order is the largest we have see so far, put it in a new chunk
            // past all the existing chunks.
            self.chunks.push(vec![order]);
        } else {
            // Insert the order into the appropriate chunk.
            let chunk = &mut self.chunks[idx];
            let pos = chunk
                .binary_search_by(|o| C::cmp(o, &order))
                .unwrap_or_else(|x| x);
            chunk.insert(pos, order);

            // split the chunk if it is now too big
            if chunk.len() > self.max_chunk_size {
                let mid = chunk.len() / 2;
                let sibling = chunk.split_off(mid);
                self.chunks.insert(idx + 1, sibling);
            }
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
    pub fn insert(&mut self, order: Order) {
        self.map.insert(order.order_id, order.clone());
        self.insert_in_chunk(self.find_chunk(&order), order);
    }

    /// Remove an order by id.
    pub fn remove_by_id(&mut self, order_id: i32) -> Option<Order> {
        // lookup in map to get the existing Order, then remove it from its chunk
        self.map.remove(&order_id).map(|order| {
            self.remove_from_chunk(self.find_chunk(&order), &order);
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

    // choose your chunk size; 1024 is just an example; profiling should guide this choice
    let max_chunk_size = 1024;
    let mut buys = SortedOrders::<BuyCmp>::new(max_chunk_size);
    let mut sells = SortedOrders::<SellCmp>::new(max_chunk_size);

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
