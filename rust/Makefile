.PHONY: all clean build test

all: build

build:
	cargo build --release --workspace

test:
	cargo test --workspace

clean:
	cargo clean
	rm -rf target/
