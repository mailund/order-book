.PHONY: all clean build test

all: build

build:
	cargo build --release --workspace

debug:
	RUSTFLAGS="-C debuginfo=2" cargo build --workspace

test:
	cargo test --workspace

clean:
	cargo clean
	rm -rf target/
