#!/bin/zsh
cd -- "$(dirname -- "$0")" || exit 1
python3 tools/install-sd.py "$@"
printf '\n按回车关闭窗口。'
read
