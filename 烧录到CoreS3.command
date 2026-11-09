#!/bin/zsh
set -e
cd -- "$(dirname -- "$0")"
if ! command -v pio >/dev/null; then
  print '请先安装 PlatformIO Core，并将 pio 加入 PATH。'
  exit 1
fi
pio run -d firmware -e cores3 -t upload "$@"
