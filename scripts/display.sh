#!/bin/zsh

# display_inline <path/to/image.png>
display_inline() {
  local img=$1
  # Read the file and base64-encode it on one line
  local b64
  b64=$(base64 -w0 < "$img")
  # Emit the iTerm2 inline image escape:
  #   ESC ] 1337;File=[params]:base64… BEL
  printf '\033]1337;File=name=%s;inline=1:%s\a\n' \
         "${img##*/}" "$b64"
}
