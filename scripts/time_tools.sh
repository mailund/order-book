#!/usr/bin/env zsh

autoload -Uz colors && colors

# Get the directory of this script
script_dir="$(cd "$(dirname "$0")" && pwd)"

# Load tools table & inline-display helper
load_script() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    echo "❌ Error: $(basename "$file") not found in $script_dir." >&2
    exit 1
  fi
  source "$file"
}

load_script "$script_dir/tools.sh"
load_script "$script_dir/display.sh"

# Default settings
small=( ${(k)tools:#py_sqlite} )
medium=( c_sorted c_unsorted_id_hash c_radix_on_query c_radix_on_query_bytes rust_sorted rust_blocks )
large=( c_sorted rust_sorted rust_blocks )

run_small=true
run_medium=true
run_large=false

plot_small=false
plot_medium=false
plot_large=false
plot_small_explicit=false
plot_medium_explicit=false
plot_large_explicit=false

verbose=false
list_only=false

small_csv="small.csv"
medium_csv="medium.csv"
large_csv="large.csv"

# Helper to split a comma-separated list into a zsh array
split_csv_to_array() {
  local IFS=',' str="$1"
  echo $=("${(@s/,/)str}")
}

# Draw a one-line progress bar
draw_progress() {
  local label=$1 cur=$2 tot=$3
  local width=40
  local filled=$(( cur * width / tot ))
  local empty=$(( width - filled ))
  local bar_filled=$(printf '%*s' $filled '' | tr ' ' '#')
  local bar_empty=$(printf '%*s' $empty '' | tr ' ' '.')
  printf "\r%s [%s%s] %3d%% (%d/%d)" \
    "$label" "$bar_filled" "$bar_empty" $(( cur * 100 / tot )) $cur $tot
}

# Parse options
while [[ $# -gt 0 ]]; do
  case $1 in
    --run-small)        run_small=true;           shift;;
    --no-small)         run_small=false;          shift;;
    --run-medium)       run_medium=true;          shift;;
    --no-medium)        run_medium=false;         shift;;
    --run-large)        run_large=true;           shift;;
    --no-large)         run_large=false;          shift;;

    --plot-small)       plot_small=true;  plot_small_explicit=true; shift;;
    --no-plot-small)    plot_small=false; plot_small_explicit=true; shift;;
    --plot-medium)      plot_medium=true; plot_medium_explicit=true; shift;;
    --no-plot-medium)   plot_medium=false;plot_medium_explicit=true; shift;;
    --plot-large)       plot_large=true;  plot_large_explicit=true; shift;;
    --no-plot-large)    plot_large=false; plot_large_explicit=true; shift;;

    --small-csv)        small_csv=$2;      shift 2;;
    --medium-csv)       medium_csv=$2;     shift 2;;
    --large-csv)        large_csv=$2;      shift 2;;

    --small-list)       small=( $(split_csv_to_array $2) ); shift 2;;
    --medium-list)      medium=( $(split_csv_to_array $2) ); shift 2;;
    --large-list)       large=( $(split_csv_to_array $2) ); shift 2;;

    --verbose)          verbose=true;      shift;;
    --no-verbose)       verbose=false;     shift;;

    --list)             list_only=true;    shift;;
    --help)
      cat <<EOF
Usage: ./time.sh [options]

Three benchmark sizes: small, medium, large.

Options:
  --run-small           run small benchmarks (default)
  --no-small            skip small
  --run-medium          run medium benchmarks (default)
  --no-medium           skip medium
  --run-large           run large benchmarks
  --no-large            skip large (default)

  --plot-small          plot small results
  --no-plot-small       don’t plot small
  --plot-medium         plot medium results
  --no-plot-medium      don’t plot medium
  --plot-large          plot large results
  --no-plot-large       don’t plot large

  --small-csv   <file>  CSV output for small (default small.csv)
  --medium-csv  <file>  CSV output for medium (default medium.csv)
  --large-csv   <file>  CSV output for large (default large.csv)

  --small-list  <csv>   tools to run for small, comma-separated
  --medium-list <csv>   tools to run for medium
  --large-list  <csv>   tools to run for large

  --verbose             show per-tool timing (default: progress bars)
  --no-verbose          hide per-tool timing (show progress bars)

  --list                list available tool names
  --help                this message
EOF
      exit 0;;
    -*)
      echo "Unknown option: $1" >&2
      exit 1;;
    *)
      break;;
  esac
done

# List only?
if [[ $list_only == true ]]; then
  print -P "%F{cyan}📋 Available implementations:%f"
  for name in ${(k)tools}; do echo "  - $name"; done
  exit 0
fi

# Default plot flags follow run flags unless explicitly set
(( ! plot_small_explicit ))  && plot_small=$run_small
(( ! plot_medium_explicit )) && plot_medium=$run_medium
(( ! plot_large_explicit ))  && plot_large=$run_large

# Build
echo
print -P "%F{cyan}🔨 Building project with make...%f"
if ! make -s; then
  print -P "%F{red}❌ Build failed!%f"
  exit 1
fi

# Cold-start warm-up
print -P "%F{cyan}🔄 Performing cold-start warm-up…%f"
for name in ${(k)tools}; do
  printf "  Warming up %-30s … " "$name"
  eval "${tools[$name]}" < /dev/null &>/dev/null
  print -P "%F{green}done%f"
done
echo

# Prepare temp dir
tempdir=$(mktemp -d)
test_data="$tempdir/test_data.txt"
trap 'rm -rf "$tempdir"' EXIT

# ────────────────────────────────────────────────────
# 🚀 small-N benchmarks
# ────────────────────────────────────────────────────
if [[ $run_small == true ]]; then
  echo
  print -P "%F{cyan}🚀 Running small-N benchmarks…%f"
  echo "tool,N,duration" >| "$small_csv"
  small_Ns=( $(seq 1000 500 10000) )
  total=$(( ${#small_Ns} * ${#small[@]} ))
  count=0

  for N in "${small_Ns[@]}"; do
    python3 simulator/simulate.py -n "$N" -o "$test_data"
    for name in "${small[@]}"; do
      count=$((count+1))
      if [[ $verbose == true ]]; then
        printf "  %-30s (N=%6d): " "$name" "$N"
      else
        draw_progress "small" $count $total
      fi

      t0=$(python3 - <<<'import time;print(time.time())')
      eval "${tools[$name]} --silent < \"$test_data\"" &>/dev/null
      t1=$(python3 - <<<'import time;print(time.time())')
      dt=$(printf "%.2f" "$(echo "$t1 - $t0" | bc)")

      [[ $verbose == true ]] && printf "%6.2f s\n" "$dt"
      echo "$name,$N,$dt" >>| "$small_csv"
    done
  done
  (( verbose == false )) && echo
fi

# 📈 small plot
if [[ $plot_small == true ]]; then
  if [[ -s "$small_csv" ]]; then
    Rscript - <<EOF
library(ggplot2)
d <- read.csv("$small_csv", stringsAsFactors=FALSE)
p <- ggplot(d, aes(x=N, y=duration, color=tool)) +
     geom_line() + geom_point() +
     labs(title="Small-N Performance", x="N", y="Duration (s)") +
     theme_minimal() +
     theme(
       plot.background   = element_rect(fill="black",   color=NA),
       panel.background  = element_rect(fill="black",   color=NA),
       panel.grid.major  = element_line(color="grey30"),
       panel.grid.minor  = element_line(color="grey20"),
       plot.title        = element_text(color="white", size=14, face="bold"),
       axis.title        = element_text(color="white", size=12),
       axis.text         = element_text(color="white", size=10),
       legend.title      = element_text(color="white", size=12),
       legend.text       = element_text(color="white", size=10)
     )
print(p)
ggsave("small.png", p, width=8, height=6)
EOF
    display_inline small.png
  else
    print -P "%F{yellow}⚠️ small CSV '$small_csv' empty, skipping plot.%f"
  fi
fi

# ────────────────────────────────────────────────────
# 🚀 medium-N benchmarks
# ────────────────────────────────────────────────────
if [[ $run_medium == true ]]; then
  echo
  print -P "%F{cyan}🚀 Running medium-N benchmarks…%f"
  echo "tool,N,duration" >| "$medium_csv"
  medium_Ns=( $(seq 20000 10000 200000) )
  total=$(( ${#medium_Ns} * ${#medium[@]} ))
  count=0

  for N in "${medium_Ns[@]}"; do
    python3 simulator/simulate.py -n "$N" -o "$test_data"
    for name in "${medium[@]}"; do
      count=$((count+1))
      if [[ $verbose == true ]]; then
        printf "  %-30s (N=%6d): " "$name" "$N"
      else
        draw_progress "medium" $count $total
      fi

      t0=$(python3 - <<<'import time;print(time.time())')
      eval "${tools[$name]} --silent < \"$test_data\"" &>/dev/null
      t1=$(python3 - <<<'import time;print(time.time())')
      dt=$(printf "%.2f" "$(echo "$t1 - $t0" | bc)")

      [[ $verbose == true ]] && printf "%6.2f s\n" "$dt"
      echo "$name,$N,$dt" >>| "$medium_csv"
    done
  done
  (( verbose == false )) && echo
fi

# 📈 medium plot
if [[ $plot_medium == true ]]; then
  if [[ -s "$medium_csv" ]]; then
    Rscript - <<EOF
library(ggplot2)
d <- read.csv("$medium_csv", stringsAsFactors=FALSE)
p <- ggplot(d, aes(x=N, y=duration, color=tool)) +
     geom_line() + geom_point() +
     labs(title="Medium-N Performance", x="N", y="Duration (s)") +
     theme_minimal() +
     theme(
       plot.background   = element_rect(fill="black",   color=NA),
       panel.background  = element_rect(fill="black",   color=NA),
       panel.grid.major  = element_line(color="grey30"),
       panel.grid.minor  = element_line(color="grey20"),
       plot.title        = element_text(color="white", size=14, face="bold"),
       axis.title        = element_text(color="white", size=12),
       axis.text         = element_text(color="white", size=10),
       legend.title      = element_text(color="white", size=12),
       legend.text       = element_text(color="white", size=10)
     )
print(p)
ggsave("medium.png", p, width=8, height=6)
EOF
    display_inline medium.png
  else
    print -P "%F{yellow}⚠️ medium CSV '$medium_csv' empty, skipping plot.%f"
  fi
fi

# ────────────────────────────────────────────────────
# 🚀 large-N benchmarks
# ────────────────────────────────────────────────────
if [[ $run_large == true ]]; then
  echo
  print -P "%F{cyan}🚀 Running large-N benchmarks…%f"
  echo "tool,N,duration" >| "$large_csv"
  large_Ns=( $(seq 500000 100000 1000000) )
  total=$(( ${#large_Ns} * ${#large[@]} ))
  count=0

  for N in "${large_Ns[@]}"; do
    python3 simulator/simulate.py -n "$N" -o "$test_data"
    for name in "${large[@]}"; do
      count=$((count+1))
      if [[ $verbose == true ]]; then
        printf "  %-30s (N=%6d): " "$name" "$N"
      else
        draw_progress "large" $count $total
      fi

      t0=$(python3 - <<<'import time;print(time.time())')
      eval "${tools[$name]} --silent < \"$test_data\"" &>/dev/null
      t1=$(python3 - <<<'import time;print(time.time())')
      dt=$(printf "%.2f" "$(echo "$t1 - $t0" | bc)")

      [[ $verbose == true ]] && printf "%6.2f s\n" "$dt"
      echo "$name,$N,$dt" >>| "$large_csv"
    done
  done
  (( verbose == false )) && echo
fi

# 📈 large plot
if [[ $plot_large == true ]]; then
  if [[ -s "$large_csv" ]]; then
    Rscript - <<EOF
library(ggplot2)
d <- read.csv("$large_csv", stringsAsFactors=FALSE)
p <- ggplot(d, aes(x=N, y=duration, color=tool)) +
     geom_line() + geom_point() +
     labs(title="Large-N Performance", x="N", y="Duration (s)") +
     theme_minimal() +
     theme(
       plot.background   = element_rect(fill="black",   color=NA),
       panel.background  = element_rect(fill="black",   color=NA),
       panel.grid.major  = element_line(color="grey30"),
       panel.grid.minor  = element_line(color="grey20"),
       plot.title        = element_text(color="white", size=14, face="bold"),
       axis.title        = element_text(color="white", size=12),
       axis.text         = element_text(color="white", size=10),
       legend.title      = element_text(color="white", size=12),
       legend.text       = element_text(color="white", size=10)
     )
print(p)
ggsave("large.png", p, width=8, height=6)
EOF
    display_inline large.png
  else
    print -P "%F{yellow}⚠️ large CSV '$large_csv' empty, skipping plot.%f"
  fi
fi

# DONE
print -P "%F{green}✅ All benchmarks completed successfully!%f"
