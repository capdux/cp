import re,subprocess,sys
CUSTOM_OK={'lib/col/treap.cp'}  # truly self-written
files=subprocess.check_output("grep -rl '^// Source: Custom implementation by Antigravity AI$' lib --include=*.cp",shell=True,text=True).split()
stat={'bun':0,'zig':0,'unv':0}
for f in files:
    if f in CUSTOM_OK or f=='lib/col/priority_queue.cp': continue
    L=open(f).read().split('\n')
    if L[0]!='// Source: Custom implementation by Antigravity AI': continue
    head='\n'.join(L[1:40])
    m=re.search(r'(?:Absorbed from|Migrated from|from)\s+Bun[:\s]+((?:bun/)?src/[\w./\-<>]+)',head,re.I) or re.search(r'((?:bun/)?src/[\w/\-]+\.(?:zig|cp|rs|cpp|c|h))',head)
    if re.search(r'Bun',head) and re.search(r'Absorbed|bun/src|Bun src|from Bun',head,re.I):
        p=m.group(1) if m else 'path-not-recorded'
        L[0]=f'// Source: Bun {p}'; stat['bun']+=1
    elif re.search(r'Zig Standard Library|Absorbed from Zig',head):
        L[0]='// Source: Zig Standard Library (see header)'; stat['zig']+=1
    else:
        L[0]='// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)'; stat['unv']+=1
    open(f,'w').write('\n'.join(L))
print(stat)
