use std::collections::BTreeMap;
use std::env;
use std::fs::File;
use std::io::{self, BufRead};

use shared::parse_events;
use shared::Event;
use shared::Order;
use shared::OrderType;

#[derive(Default)]
struct SortedOrders {
    map: BTreeMap<(i32, i32, i32), Order>,
    descending: bool,
}

impl SortedOrders {
    fn new(descending: bool) -> Self {
        Self {
            map: BTreeMap::new(),
            descending,
        }
    }

    fn insert(&mut self, order: Order) {
        let key = if self.descending {
            (-order.price, -order.quantity, order.order_id)
        } else {
            (order.price, order.quantity, order.order_id)
        };
        self.map.insert(key, order);
    }

    fn update(&mut self, order_id: i32, new_price: i32) {
        if let Some((key, mut order)) = self
            .map
            .iter()
            .find(|(_, o)| o.order_id == order_id)
            .map(|(k, v)| (k.clone(), v.clone()))
        {
            self.map.remove(&key);
            order.price = new_price;
            self.insert(order);
        }
    }

    fn remove(&mut self, order_id: i32) {
        if let Some((key, _)) = self
            .map
            .iter()
            .find(|(_, o)| o.order_id == order_id)
            .map(|(k, v)| (k.clone(), v.clone()))
        {
            self.map.remove(&key);
        }
    }

    fn print(&self) {
        for order in self.map.values() {
            println!(
                "\t{:?} {} {}",
                order.order_type, order.price, order.quantity
            );
        }
    }

    fn is_empty(&self) -> bool {
        self.map.is_empty()
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let silent = args.contains(&"--silent".to_string());
    let input: Box<dyn BufRead> = if args.len() > 1 && args[1] != "--silent" {
        Box::new(io::BufReader::new(
            File::open(&args[1]).expect("Failed to open input file"),
        ))
    } else {
        Box::new(io::BufReader::new(io::stdin()))
    };

    let mut buys = SortedOrders::new(true);
    let mut sells = SortedOrders::new(false);
    let mut order_id_counter = 0;

    for event in parse_events(input) {
        match event {
            Event::Create {
                order_type,
                price,
                quantity,
            } => {
                let order = Order::new(order_id_counter, order_type, price, quantity);
                order_id_counter += 1;
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
