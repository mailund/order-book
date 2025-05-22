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


# Generate test data
print -P "%F{cyan}Simulating data with N=$N...%f"
python3 simulator/simulate.py -n "$N" -o "$test_data"

# Define associative array of implementations
typeset -A versions
versions=(
  py_brute "PYTHONPATH=py_brute_force_lists python3 py_brute_force_lists/py_brute_force_lists.py"
  py_sqlite "PYTHONPATH=py_sqlite python3 py_sqlite/py_sqlite.py"
  py_lazy "PYTHONPATH=py_lazy_sort python3 py_lazy_sort/py_lazy_sort.py"
)
typeset -A runtimes

# Run each version and store output
echo
print -P "%F{cyan}Running order book versions...%f"
for name in "${(@k)versions}"; do
  echo "Executing $name command..."
  start=$(date +%s.%N) 
  eval "${versions[$name]} < \"$test_data\"" > "$tempdir/${name}_output.txt"
  end=$(date +%s.%N) 
  duration=$(printf "%.2f" "$(echo "$end - $start" | bc)")
  runtimes[$name]=$duration
done

# Run diffs against brute output
echo
for name in "${(@k)versions}"; do
  [[ $name == "brute" ]] && continue  # Skip diffing brute against itself
  print -P "%F{cyan}Running diff for $name vs py_brute...%f"
  diff "$tempdir/${name}_output.txt" "$tempdir/py_brute_output.txt" > "$tempdir/${name}_diff.txt"
  if [[ $? -eq 0 ]]; then
    print -P "%F{green}✅ $name output matches py_brute.%f"
  else
    print -P "%F{red}✘ $name output differs from py_brute!%f"
    print -P "%F{red}↳ See diff: $tempdir/${name}_diff.txt%f"
    echo "First few lines of diff:"
    head -n 5 "$tempdir/${name}_diff.txt" | sed 's/^/    /'
  fi
done

# Show timing summary
echo
print -P "%F{cyan}⏱ Execution time summary:%f"
print "+-----------------+-----------+"
print "| Version         | Time (s)  |"
print "+-----------------+-----------+"
for name in ${(k)versions}; do
  printf "| %-15s | %9s |\n" "$name" "$runtimes[$name]"
done
print "+-----------------+-----------+"

