#!/bin/zsh

autoload -Uz colors && colors

# Default number of events
N=1000
keep_files=false

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
    -*)
      echo "Unknown option: $1" >&2
      exit 1
      ;;
    *)
      break
      ;;
  esac
done

tempdir=$(mktemp -d)
test_data="$tempdir/test_data.txt"

if [[ $keep_files == false ]]; then
  trap 'rm -rf "$tempdir"' EXIT
fi

# 🧪 Generate test data
print -P "%F{cyan}🧪 Simulating data with N=$N...%f"
python3 simulator/simulate.py -n "$N" -o "$test_data"

# Define associative array of implementations
typeset -A versions
versions=(
  py_brute "PYTHONPATH=python/py_brute_force_lists python3 python/py_brute_force_lists/py_brute_force_lists.py"
  py_sqlite "PYTHONPATH=python/py_sqlite python3 python/py_sqlite/py_sqlite.py"
  py_sorted_list "PYTHONPATH=python/py_sorted_list python3 python/py_sorted_list/py_sorted_list.py"
  py_lazy "PYTHONPATH=python/py_lazy_sort python3 python/py_lazy_sort/py_lazy_sort.py"
  c_unsorted "c/unsorted_lists/unsorted_lists"
)
typeset -A runtimes

# 🚀 Run each version and store output
echo
print -P "%F{cyan}🚀 Running order book versions...%f"
for name in "${(@k)versions}"; do
  printf "  🛠  Executing %-15s ... " "$name"

  # Using Python here since macOS date doesn't get nanoseconds
  # GNU date would work, though. 
  start=$(python3 -c 'import time; print(time.time())')
  eval "${versions[$name]} < \"$test_data\"" > "$tempdir/${name}_output.txt"
  end=$(python3 -c 'import time; print(time.time())')
  duration=$(printf "%.2f" "$(echo "$end - $start" | bc)")
  runtimes[$name]=$duration

  printf "%6.2f seconds\n" "$duration"
done

# 🔍 Compare against brute output
echo
print -P "%F{cyan}🔍 Comparing the output of each version against py_brute...%f"
for name in "${(@k)versions}"; do
  [[ $name == "py_brute" ]] && continue  # Skip diffing brute against itself
  diff "$tempdir/${name}_output.txt" "$tempdir/py_brute_output.txt" > "$tempdir/${name}_diff.txt"
  if [[ $? -eq 0 ]]; then
    print -P "%F{green}  ✅ $name output matches py_brute.%f"
  else
    print -P "%F{red}  ✘ $name output differs from py_brute!%f"
    print -P "%F{red}↳ See diff: $tempdir/${name}_diff.txt%f"
    echo "First few lines of diff:"
    head -n 5 "$tempdir/${name}_diff.txt" | sed 's/^/    /'
  fi
done

# ⏱ Show timing summary
echo
print -P "%F{cyan}⏱ Execution time summary:%f"
print "+-----------------+-----------+"
print "| Version         | Time (s)  |"
print "+-----------------+-----------+"
for name in ${(k)versions}; do
  printf "| %-15s | %9s |\n" "$name" "$runtimes[$name]"
done
print "+-----------------+-----------+"

# Optionally preserve output
if [[ $keep_files == true ]]; then
  echo
  print -P "%F{yellow}📂 Temp files kept in: $tempdir%f"
fi
