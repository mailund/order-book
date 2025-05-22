#!/bin/zsh

autoload -Uz colors && colors

# Default number of events
N=1000

# Parse options
while [[ $# -gt 0 ]]; do
  case $1 in
    -n|--num)
      N=$2
      shift 2
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
trap 'rm -rf "$tempdir"' EXIT

# Generate test data
print -P "%F{cyan}Simulating data with N=$N...%f"
python3 simulator/simulate.py -n "$N" -o "$test_data"

# Define associative array of implementations
typeset -A versions
versions=(
  brute "PYTHONPATH=py_brute_force_lists python3 py_brute_force_lists/py_brute_force_lists.py"
  sqlite "PYTHONPATH=py_sqlite python3 py_sqlite/py_sqlite.py"
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
  print -P "%F{cyan}Running diff for $name vs brute...%f"
  diff "$tempdir/${name}_output.txt" "$tempdir/brute_output.txt" > "$tempdir/${name}_diff.txt"
  if [[ $? -eq 0 ]]; then
    print -P "%F{green}✅ $name output matches brute.%f"
  else
    print -P "%F{red}✘ $name output differs from brute!%f"
    print -P "%F{red}↳ See diff: $tempdir/${name}_diff.txt%f"
    echo "First few lines of diff:"
    head -n 5 "$tempdir/${name}_diff.txt" | sed 's/^/    /'
  fi
done

# Show timing summary
echo
print -P "%F{cyan}⏱ Execution time summary:%f"
print "+---------+-----------+"
print "| Version | Time (s)  |"
print "+---------+-----------+"
for name in ${(k)versions}; do
  printf "| %-7s | %9s |\n" "$name" "$runtimes[$name]"
done
print "+---------+-----------+"

