.PHONY: all clean build test

all: build

build:
	cargo build --release --workspace

debug:
	cargo build --workspace

test:
	cargo test --workspace

clean:
	cargo clean
	rm -rf target/
