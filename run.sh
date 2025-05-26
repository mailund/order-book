#!/bin/zsh

autoload -Uz colors && colors

# Default settings
N=10000
keep_files=false
excluded=()
included=()
list_only=false
compare=true
baseline="py_brute"

# Parse options
while [[ $# -gt 0 ]]; do
  case $1 in
    -n|--num)
      N=$2
      shift 2
      ;;
    --keep-files)
      keep_files=true
      shift
      ;;
    --exclude)
      IFS=',' read -A excluded <<< "$2"
      shift 2
      ;;
    --include)
      IFS=',' read -A included <<< "$2"
      shift 2
      ;;
    --baseline)
      baseline=$2
      shift 2
      ;;
    --no-compare)
      compare=false
      shift
      ;;
    --list)
      list_only=true
      shift
      ;;
    --help)
      cat <<EOF
Usage: ./run.sh [options]

Options:
  -n, --num N           Number of simulated events (default: 1000)
  --keep-files          Preserve temporary files after execution
  --exclude A,B         Comma-separated list of implementations to exclude
  --include A,B         Comma-separated list of implementations to include
  --baseline NAME       Choose comparison baseline (default: py_brute)
  --no-compare          Skip output comparison against baseline
  --list                Print available implementation names and exit
  --help                Show this help message and exit

Examples:
  ./run.sh --include py_brute,py_sqlite --baseline py_sqlite
  ./run.sh --exclude py_lazy --keep-files --no-compare
EOF
      exit 0
      ;;
    -*)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
    *)
      break
      ;;
  esac
done

# Define all known implementations
typeset -A versions
versions=(
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
)

# Handle --list
if [[ $list_only == true ]]; then
  print -P "%F{cyan}📋 Available implementations:%f"
  for name in ${(k)versions}; do
    echo "  - $name"
  done
  exit 0
fi

# Validate baseline tool exists
if [[ -z "${versions[$baseline]}" ]]; then
  echo "❌ Error: Baseline '$baseline' is not a known implementation." >&2
  exit 1
fi

# If we need to compare output and baseline is excluded, error out
if [[ $compare == true ]]; then
  for ex in "${excluded[@]}"; do
    [[ "$baseline" == "$ex" ]] && {
      echo "❌ Error: Baseline '$baseline' is excluded. Remove it from --exclude." >&2
      exit 1
    }
  done
fi

# If --include used and baseline is not in it, add it
if [[ ${#included[@]} -gt 0 ]]; then
  if ! [[ " ${included[@]} " =~ " $baseline " ]]; then
    included+=($baseline)
  fi
fi

# Check if a tool should run
should_run() {
  local name=$1
  if [[ ${#included[@]} -gt 0 ]]; then
    for inc in "${included[@]}"; do
      [[ "$name" == "$inc" ]] && return 0
    done
    return 1
  fi
  for exc in "${excluded[@]}"; do
    [[ "$name" == "$exc" ]] && return 1
  done
  return 0
}

# Prepare temp dir
tempdir=$(mktemp -d)
test_data="$tempdir/test_data.txt"
cleanup_tempdir=true
if [[ $keep_files == true ]]; then
  cleanup_tempdir=false
fi

# 🧪 Generate test data
print -P "%F{cyan}🧪 Simulating data with N=$N...%f"
python3 simulator/simulate.py -n "$N" -o "$test_data"

# 🛠 Build the projects
echo
print -P "%F{cyan}🔨 Building project with make...%f"
if ! make -s; then
  print -P "%F{red}❌ Build failed!%f"
  exit 1
fi

typeset -A runtimes

# 🚀 Run selected versions
echo
print -P "%F{cyan}🚀 Running order book versions...%f"
for name in "${(@k)versions}"; do
  if ! should_run "$name"; then
    print -P "%F{yellow}  ⚠️  Skipping $name%f"
    continue
  fi

  printf "  🛠  Executing %-25s ... " "$name"
  start=$(python3 -c 'import time; print(time.time())')
  if [[ $compare == false ]]; then
    if ! eval "${versions[$name]} --silent < \"$test_data\"" > /dev/null 2>&1; then
      echo "❌ Error: Command for $name failed." >&2
      continue
    fi
  else
    if ! eval "${versions[$name]} < \"$test_data\"" > "$tempdir/${name}_output.txt" 2>&1; then
      echo "❌ Error: Command for $name failed." >&2
      continue
    fi
  fi
  end=$(python3 -c 'import time; print(time.time())')
  duration=$(printf "%.2f" "$(echo "$end - $start" | bc)")
  runtimes[$name]=$duration
  printf "%6.2f seconds\n" "$duration"
done

# 🔍 Diff against baseline
if [[ $compare == true ]]; then
  echo
  print -P "%F{cyan}🔍 Comparing the output of each version against $baseline...%f"
  for name in "${(@k)versions}"; do
    [[ "$name" == "$baseline" ]] && continue
    should_run "$name" || continue

    diff "$tempdir/${name}_output.txt" "$tempdir/${baseline}_output.txt" > "$tempdir/${name}_diff.txt"
    if [[ $? -eq 0 ]]; then
      print -P "%F{green}  ✅ $name output matches $baseline.%f"
    else
      cleanup_tempdir=false
      print -P "%F{red}  ✘ $name output differs from $baseline!%f"
      print -P "%F{red}↳ See diff: $tempdir/${name}_diff.txt%f"
      echo "First few lines of diff:"
      head -n 5 "$tempdir/${name}_diff.txt" | sed 's/^/    /'
    fi
  done
fi

# ⏱ Summary
echo
print -P "%F{cyan}⏱ Execution time summary:%f"
print "+---------------------------+-----------+"
print "| Version                   | Time (s)  |"
print "+---------------------------+-----------+"
for name in ${(k)versions}; do
  should_run "$name" || continue
  printf "| %-25s | %9s |\n" "$name" "$runtimes[$name]"
done
print "+---------------------------+-----------+"

# 📂 Final message or cleanup
if [[ $cleanup_tempdir == true ]]; then
  rm -rf "$tempdir"
else
  echo
  print -P "%F{yellow}📂 Temp files kept in: $tempdir%f"
fi
