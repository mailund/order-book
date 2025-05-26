use std::env;
use std::io::{self, BufRead};

use shared::parse_events;
use shared::Event;
use shared::Order;
use shared::OrderType;

#[derive(Default)]
struct SortedOrders {
    orders: Vec<Order>,
    descending: bool,
}

impl SortedOrders {
    fn new(descending: bool) -> Self {
        Self {
            orders: Vec::new(),
            descending,
        }
    }

    fn cmp(&self, a: &Order, b: &Order) -> std::cmp::Ordering {
        let (pa, qa, id_a) = (a.price, a.quantity, a.order_id);
        let (pb, qb, id_b) = (b.price, b.quantity, b.order_id);

        let primary = if self.descending {
            pb.cmp(&pa)
        } else {
            pa.cmp(&pb)
        };

        if primary != std::cmp::Ordering::Equal {
            return primary;
        }

        let secondary = if self.descending {
            qb.cmp(&qa)
        } else {
            qa.cmp(&qb)
        };

        if secondary != std::cmp::Ordering::Equal {
            return secondary;
        }

        id_a.cmp(&id_b)
    }

    fn insert(&mut self, order: Order) {
        self.orders.push(order);
        let mut i = self.orders.len() - 1;
        while i > 0 && self.cmp(&self.orders[i], &self.orders[i - 1]) == std::cmp::Ordering::Less {
            self.orders.swap(i, i - 1);
            i -= 1;
        }
    }

    fn update(&mut self, order_id: i32, new_price: i32) {
        if let Some(i) = self.orders.iter().position(|o| o.order_id == order_id) {
            self.orders[i].price = new_price;
            self.reorder(i);
        }
    }

    fn reorder(&mut self, mut index: usize) {
        while index > 0
            && self.cmp(&self.orders[index], &self.orders[index - 1]) == std::cmp::Ordering::Less
        {
            self.orders.swap(index, index - 1);
            index -= 1;
        }
        while index + 1 < self.orders.len()
            && self.cmp(&self.orders[index], &self.orders[index + 1]) == std::cmp::Ordering::Greater
        {
            self.orders.swap(index, index + 1);
            index += 1;
        }
    }

    fn remove(&mut self, order_id: i32) {
        if let Some(i) = self.orders.iter().position(|o| o.order_id == order_id) {
            self.orders.remove(i);
        }
    }

    fn print(&self) {
        for order in &self.orders {
            println!(
                "\t{:?} {} {}",
                order.order_type, order.price, order.quantity
            );
        }
    }

    fn is_empty(&self) -> bool {
        self.orders.is_empty()
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let silent = args.contains(&"--silent".to_string());
    let input: Box<dyn BufRead> = Box::new(io::BufReader::new(io::stdin()));

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
