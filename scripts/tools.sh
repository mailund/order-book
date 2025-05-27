#!/bin/zsh

# Define all known implementations
# NB! -g makes it global so you can source the file in a function;
# if you remove it, sourcing from functions will make it a local variable,
# so be careful with that.
typeset -g -A tools
tools=(
  py_brute                 "PYTHONPATH=python/py_brute_force_lists python3 python/py_brute_force_lists/py_brute_force_lists.py"
  py_sqlite                "PYTHONPATH=python/py_sqlite python3 python/py_sqlite/py_sqlite.py"
  py_sorted_list           "PYTHONPATH=python/py_sorted_list python3 python/py_sorted_list/py_sorted_list.py"
  py_lazy                  "PYTHONPATH=python/py_lazy_sort python3 python/py_lazy_sort/py_lazy_sort.py"
  c_unsorted               "c/unsorted_lists/main"
  c_sorted                 "c/sorted_lists/main"
  c_unsorted_id_hash       "c/unsorted_id_hash/main"
  c_radix_on_query         "c/radix_sorted_on_query/main"
  c_radix_on_query_bytes   "c/radix_sorted_on_query/bytes"
  rust_sorted              "rust/target/release/sorted"
  rust_blocks              "rust/target/release/blocks"
  rust_blocks_and_table    "rust/target/release/blocks_and_table"
  rust_btree               "rust/target/release/btree"
)
