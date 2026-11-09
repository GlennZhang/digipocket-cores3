"""Copy and verify the complete resource pack; never format or erase a card."""
from pathlib import Path
import hashlib,shutil,subprocess,re,datetime,sys,os
ROOT=Path(__file__).resolve().parents[1]
def mounted_cards():
 text=subprocess.check_output(['mount'],text=True)
 return [Path(m.group(1)) for line in text.splitlines() if (m:=re.search(r' on (/Volumes/.*?) \(msdos,',line))]
def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for chunk in iter(lambda:f.read(131072),b''):h.update(chunk)
 return h.hexdigest()
def install(source,volume):
 source=Path(source);volume=Path(volume)
 files=[p for p in source.rglob('*') if p.is_file()];catalog=source/'catalog.tsv'
 if not catalog.is_file():raise RuntimeError('源素材缺少 catalog.tsv，停止复制。')
 size=sum(p.stat().st_size for p in files)
 if shutil.disk_usage(volume).free<size+2*1024*1024:raise RuntimeError('卡上空间不足。')
 stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f');stage=volume/('digipet.installing-'+stamp);target=volume/'digipet'
 print(f'复制 {len(files):,} 个文件，约 {size/1024/1024:.1f}MB 到 {volume}。',flush=True)
 shutil.copytree(source,stage,symlinks=False)
 for src in files:
  if digest(src)!=digest(stage/src.relative_to(source)):raise RuntimeError(f'校验失败：{src.name}。原素材未替换，临时目录保留供排查。')
 backup=None
 if target.exists():backup=volume/('digipet.backup-'+stamp);target.rename(backup)
 try:stage.rename(target)
 except Exception:
  if backup and not target.exists():backup.rename(target)
  raise
 print('全部文件校验通过。路径：'+str(target/'catalog.tsv'))
 if backup:print('原目录已保留备份：'+str(backup))
 return target
if __name__=='__main__':
 try:
  import argparse
  parser=argparse.ArgumentParser()
  parser.add_argument('source', type=Path, help='解压后 sdcard/digipet 目录')
  parser.add_argument('volume', nargs='?', help='已挂载的 SD 卡路径')
  args=parser.parse_args()
  cards=mounted_cards()
  if not cards:raise RuntimeError('未找到接到 Mac 的 FAT 存储卡。请先用读卡器连接 SD 卡；CoreS3 的 USB 连接不能直接复制卡文件。')
  if args.volume:
   volume=Path(args.volume)
   if volume not in cards:raise RuntimeError('指定目录不是已挂载的存储卡。')
  elif len(cards)==1:volume=cards[0]
  else:
   for i,p in enumerate(cards,1):print(f'{i}. {p}')
   number=int(input('选择 SD 卡序号：'))
   if not 1<=number<=len(cards):raise RuntimeError('无效序号。')
   volume=cards[number-1]
  install(args.source,volume)
  print('请在 Finder 安全弹出卡，再插回 CoreS3 并重启。')
 except Exception as e:print('未完成：'+str(e));sys.exit(1)
