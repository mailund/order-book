#!/usr/bin/env zsh

autoload -Uz colors && colors

# ────────────────────────────────────────────────────
# Setup
# ────────────────────────────────────────────────────
script_dir="$(cd "$(dirname "$0")" && pwd)"
load_script() {
  [[ -f "$1" ]] || { echo "❌ $1 not found" >&2; exit 1 }
  source "$1"
}
load_script "$script_dir/tools.sh"    # defines typeset -A tools
load_script "$script_dir/display.sh"  # defines display_inline

# ────────────────────────────────────────────────────
# Configuration
# ────────────────────────────────────────────────────

jobs_limit=6  # max concurrent jobs

small=( ${(k)tools:#py_sqlite} )
medium=(
  c_sorted
  c_unsorted_id_hash
  c_radix_on_query
  c_radix_on_query_bytes
  py_sorted_list
  rust_sorted
  rust_blocks
  rust_blocks_and_table
  rust_btree
)
large=(
  c_sorted
  py_sorted_list
  rust_sorted
  rust_blocks
  rust_blocks_and_table
  rust_btree
)
huge=(
  rust_blocks
  rust_blocks_and_table
  rust_btree
)

run_small=true;   plot_small=false;  plot_small_explicit=false
run_medium=true;  plot_medium=false; plot_medium_explicit=false
run_large=false;  plot_large=false;  plot_large_explicit=false
run_huge=false;   plot_huge=false;   plot_huge_explicit=false

verbose=false
list_only=false

small_csv="small.csv";   small_start=1000;    small_end=10000;    small_step=500
medium_csv="medium.csv"; medium_start=20000;  medium_end=200000;  medium_step=10000
large_csv="large.csv";   large_start=500000;  large_end=1000000;  large_step=100000
huge_csv="huge.csv";     huge_start=2000000;  huge_end=6000000;   huge_step=2000000


# ────────────────────────────────────────────────────
# Parse command-line options
# ────────────────────────────────────────────────────

# split comma-separated list into Zsh array
split_csv_to_array(){
  local IFS=',' str="$1"
  echo $=("${(@s/,/)str}")
}

while [[ $# -gt 0 ]]; do
  case $1 in
    --run-small)    run_small=true;     shift;;
    --no-small)     run_small=false;    shift;;
    --run-medium)   run_medium=true;    shift;;
    --no-medium)    run_medium=false;   shift;;
    --run-large)    run_large=true;     shift;;
    --no-large)     run_large=false;    shift;;
    --run-huge)     run_huge=true;      shift;;
    --no-huge)      run_huge=false;     shift;;

    --plot-small)   plot_small=true;   plot_small_explicit=true;  shift;;
    --no-plot-small)plot_small=false;  plot_small_explicit=true;  shift;;
    --plot-medium)  plot_medium=true;  plot_medium_explicit=true; shift;;
    --no-plot-medium)plot_medium=false;plot_medium_explicit=true; shift;;
    --plot-large)   plot_large=true;   plot_large_explicit=true;  shift;;
    --no-plot-large)plot_large=false;  plot_large_explicit=true;  shift;;
    --plot-huge)    plot_huge=true;    plot_huge_explicit=true;   shift;;
    --no-plot-huge) plot_huge=false;   plot_huge_explicit=true;   shift;;

    --small-csv)    small_csv=$2;    shift 2;;
    --medium-csv)   medium_csv=$2;   shift 2;;
    --large-csv)    large_csv=$2;    shift 2;;
    --huge-csv)     huge_csv=$2;     shift 2;;

    --small-list)   small=( $(split_csv_to_array $2) );  shift 2;;
    --medium-list)  medium=( $(split_csv_to_array $2) ); shift 2;;
    --large-list)   large=( $(split_csv_to_array $2) );  shift 2;;
    --huge-list)    huge=( $(split_csv_to_array $2) );   shift 2;;

    --verbose)      verbose=true;    shift;;
    --no-verbose)   verbose=false;   shift;;

    --list)         list_only=true;  shift;;
    --help)
      cat <<'EOF'
Usage: ./time.sh [options]

Four sizes: small, medium, large, huge.

Options:
  --run-small           run small benchmarks (default)
  --no-small            skip small
  --run-medium          run medium benchmarks (default)
  --no-medium           skip medium
  --run-large           run large benchmarks
  --no-large            skip large (default)
  --run-huge            run huge benchmarks
  --no-huge             skip huge (default)

  --plot-small          plot small results
  --no-plot-small       don’t plot small
  --plot-medium         plot medium results
  --no-plot-medium      don’t plot medium
  --plot-large          plot large results
  --no-plot-large       don’t plot large
  --plot-huge           plot huge results
  --no-plot-huge        don’t plot huge

  --small-csv <file>    CSV output for small (default: small.csv)
  --medium-csv <file>   CSV output for medium (default: medium.csv)
  --large-csv <file>    CSV output for large (default: large.csv)
  --huge-csv <file>     CSV output for huge (default: huge.csv)

  --small-list <csv>    comma-separated tools for small
  --medium-list <csv>   comma-separated tools for medium
  --large-list <csv>    comma-separated tools for large
  --huge-list <csv>     comma-separated tools for huge

  --verbose             show per-tool timing (default: progress bars)
  --no-verbose          hide per-tool timing (show progress bars)

  --list                list available tool names
  --help                show this help message

Examples:
  # run only small- and medium-N benchmarks, then plot them
  ./time.sh --no-large --no-huge

  # run everything but only plot the huge-N results
  ./time.sh --run-large --run-huge --no-plot-small --no-plot-medium --no-plot-large

  # override which tools to use for the medium set
  ./time.sh --medium-list c_sorted,rust_sorted,py_sorted_list

  # use custom CSV filenames
  ./time.sh --small-csv quick.csv --large-csv big.csv

EOF
      exit 0;;
    -*)
      echo "Unknown option: $1" >&2
      exit 1;;
    *) break;;
  esac
done


# ────────────────────────────────────────────────────
# Helpers
# ────────────────────────────────────────────────────

# one‐line progress bar
draw_progress(){
  local label=$1 cur=$2 tot=$3
  local width=40
  local filled=$(( cur * width / tot ))
  local empty=$(( width - filled ))
  local bar_filled=$(printf '%*s' $filled '' | tr ' ' '#')
  local bar_empty=$(printf '%*s' $empty '' | tr ' ' '.')
  printf "\r\033[K%s [%s%s] %3d%% (%d/%d)" \
    "$label" "$bar_filled" "$bar_empty" $(( cur * 100 / tot )) $cur $tot
}

# spawn measurement job for one (tool,N) and write to tmpf
measure_tool() {
  local tool=$1 N=$2 tmpf=$3 t0 t1 dt

  [[ -f "$test_data" ]] || { echo "❌ Test data file '$test_data' not found!" >&2; exit 1; }

  t0=$(python3 - <<<'import time;print(time.time())')
  output=$(eval "${tools[$tool]} --silent < $test_data" 2>&1)
  if (( $? != 0 )); then
    echo "❌ Error: ${tools[$tool]} failed for tool '$tool' at N=$N" >&2
    echo "Captured output:" >&2
    echo "$output" >&2
    exit 1
  fi
  t1=$(python3 - <<<'import time;print(time.time())')
  dt=$(printf "%.2f" "$(echo "$t1 - $t0" | bc)")

  echo "$tool,$N,$dt" >| "$tmpf"

  [[ $verbose == true ]] && printf "  %-30s (N=%7d): %6.2f s\n" "$tool" "$N" "$dt"

  return 0 # Success
}


# throttle so we never exceed $jobs_limit concurrent tasks
throttle_jobs() {
  while (( ${#tmpjobs} >= jobs_limit )); do
    sleep 0.05
    local -a alive=()
    for pid in "${tmpjobs[@]}"; do
      if kill -0 "$pid" 2>/dev/null; then
        alive+=("$pid")
      else
        wait "$pid" || { echo "❌ Job $pid failed — aborting." >&2; exit 1; }
      fi
    done
    tmpjobs=("${alive[@]}")
  done
}

# ────────────────────────────────────────────────────
# Run & Plot functions
# ────────────────────────────────────────────────────

run_generic() {
  local label=$1
  local -a tool_list=("${(@P)2}")   # don’t shadow the global “tools”
  local start=$3 end=$4 step=$5 csv=$6

  print -P "%F{cyan}⚙️  Measuring ${label}…%f"

  echo "tool,N,duration" >| "$csv"
  local Ns=( {${start}..${end}..${step}} )
  local total=$(( ${#Ns} * ${#tool_list} ))
  local count=0

  for N in "${Ns[@]}"; do
    python3 simulator/simulate.py -n "$N" -o "$test_data"

    # launch each tool measurement in the background
    tmpjobs=()
    for tool in "${tool_list[@]}"; do
      (( count++ ))
      [[ $verbose == false ]] && draw_progress $label $count $total

      throttle_jobs

      local tmpf="$tempdir/${label}.${tool}.${N}.csv"
      measure_tool "$tool" "$N" "$tmpf" & 
      tmpjobs+=($!)
    done

    # wait for all of them, failing fast on any error
    for pid in "${tmpjobs[@]}"; do
      wait "$pid" || {
        echo "❌ Job $pid failed — aborting." >&2
        exit 1
      }
    done

    # collect the results
    for tool in "${tool_list[@]}"; do
      local f="$tempdir/${label}.${tool}.${N}.csv"
      [[ -f "$f" ]] || { 
        echo "❌ Error: expected '$f' not found — aborting." >&2
        exit 1
      }
      cat "$f" >>| "$csv"
      rm  "$f"
    done
  done

  (( verbose == false )) && echo
}

plot_generic() {
  local csv=$1 png=$2 title=$3
  [[ -s "$csv" ]] || { print -P "%F{yellow}⚠️ $csv empty, skipping plot.%f"; return }
  Rscript - <<EOF
library(ggplot2)
d <- read.csv("$csv", stringsAsFactors=FALSE)
p <- ggplot(d, aes(x=N,y=duration,color=tool)) +
     geom_line() + geom_point() +
     labs(title="$title", x="N", y="Duration (s)") +
     theme_minimal() +
     theme(
       plot.background  = element_rect(fill="black", color=NA),
       panel.background = element_rect(fill="black", color=NA),
       plot.title       = element_text(color="white", face="bold"),
       axis.title       = element_text(color="white"),
       axis.text        = element_text(color="white"),
       legend.text      = element_text(color="white")
     )
print(p); ggsave("$png", p, width=8, height=6)
EOF
  display_inline $png
}

measure_small()  { run_generic small  small  $small_start  $small_end  $small_step  $small_csv;  }
measure_medium() { run_generic medium medium $medium_start $medium_end $medium_step $medium_csv; }
measure_large()  { run_generic large  large  $large_start  $large_end  $large_step  $large_csv;  }
measure_huge()   { run_generic huge   huge   $huge_start   $huge_end   $huge_step   $huge_csv;   }

plot_small()  { plot_generic $small_csv  small.png  "Small-N Performance";  }
plot_medium() { plot_generic $medium_csv medium.png "Medium-N Performance"; }
plot_large()  { plot_generic $large_csv  large.png  "Large-N Performance";  }
plot_huge()   { plot_generic $huge_csv   huge.png   "Huge-N Performance";   }

# ────────────────────────────────────────────────────
# Main
# ────────────────────────────────────────────────────

# build
echo; print -P "%F{cyan}🔨 Building project...%f"
make -s || { print -P "%F{red}❌ Build failed%f"; exit 1 }

# warm-up
print -P "%F{cyan}🔄 Cold-start warm-up…%f"
unique_tools=($(echo "${small[@]}" "${medium[@]}" "${large[@]}" "${huge[@]}" | tr ' ' '\n' | sort -u))
count=0
total=${#unique_tools[@]}
for t in "${unique_tools[@]}"; do
  count=$((count+1))
  eval "${tools[$t]}" < /dev/null &>/dev/null
  [[ $verbose == false ]] && draw_progress "Warm-up" $count $total
  [[ $verbose == true ]] && { printf "  %-30s … " "$t" ; print -P "%F{green}ok%f"; }
done
echo

# temp data scratch
tempdir=$(mktemp -d)
test_data="$tempdir/test_data.txt"
# trap 'echo NUKING ; rm -rf "$tempdir"' EXIT

# defaults for plotting
(( ! plot_small_explicit ))  && plot_small=$run_small
(( ! plot_medium_explicit )) && plot_medium=$run_medium
(( ! plot_large_explicit ))  && plot_large=$run_large
(( ! plot_huge_explicit ))   && plot_huge=$run_huge

# run & plot each

[ $run_small  = true ] && measure_small
[ $plot_small = true ] && plot_small

[ $run_medium  = true ] && measure_medium
[ $plot_medium = true ] && plot_medium

[ $run_large  = true ] && measure_large
[ $plot_large = true ] && plot_large

[ $run_huge  = true ] && measure_huge
[ $plot_huge = true ] && plot_huge

rm -rf "$tempdir"
print -P "%F{green}✅ All done!%f"