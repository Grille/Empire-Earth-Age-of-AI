import pefile,struct,re,capstone,functools,os
# the repository lives in <game folder>/_aimod; EE_EXE overrides
EXE=os.environ.get("EE_EXE") or os.path.join(os.path.dirname(os.path.abspath(__file__)),"..","..","Empire Earth.exe")
pe=pefile.PE(EXE,fast_load=True)
IB=pe.OPTIONAL_HEADER.ImageBase
img=pe.get_memory_mapped_image()
secs={s.Name.rstrip(b'\0').decode():(IB+s.VirtualAddress,s.Misc_VirtualSize) for s in pe.sections}
text_va,text_sz=secs['.text']
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32); md.detail=False
def rd(va,n): o=va-IB; return img[o:o+n]
def u32(va): return struct.unpack('<I',rd(va,4))[0]
def cstr(va):
    o=va-IB; e=img.index(b'\0',o); return img[o:e].decode('latin1')
def find_str(s):
    b=s.encode('latin1')+b'\0'; return [IB+m.start() for m in re.finditer(re.escape(b),img)]
def xrefs(va,region=None):
    b=struct.pack('<I',va); out=[]
    st,sz=region or (IB,len(img))
    o=st-IB; seg=img[o:o+sz]
    return [st+m.start() for m in re.finditer(re.escape(b),seg)]
def dis(va,n=40,stop_ret=True):
    out=[]
    for ins in md.disasm(rd(va,n*15),va):
        out.append(ins)
        if len(out)>=n or (stop_ret and ins.mnemonic=='ret'): break
    return out
def pdis(va,n=40,stop_ret=True):
    for i in dis(va,n,stop_ret):
        extra=''
        for m in re.finditer(r'0x([0-9a-f]{6,8})',i.op_str):
            v=int(m.group(1),16)
            if IB<=v<IB+len(img):
                try:
                    s=cstr(v)
                    if len(s)>=3 and s.isprintable(): extra+=f'  ; "{s[:60]}"'
                except: pass
        print(f'{i.address:08x}: {i.mnemonic:6} {i.op_str}{extra}')
# --- RTTI ---
@functools.lru_cache(None)
def rtti():
    """returns dict classname -> list of vtable VAs"""
    res={}
    rdata=secs['.rdata']
    for m in re.finditer(rb'\.\?AV([\w@]+?)@@\0',img):
        td=IB+m.start()-8; name=m.group(1).decode()
        # COL: sig(0) off cdOff tdPtr chdPtr ; find refs to td
        for ref in xrefs(td):
            col=ref-12
            if u32(col)!=0: continue
            # vtable = ref to col +4
            for vref in xrefs(col):
                res.setdefault(name,[]).append((vref+4,u32(col+4)))
    return res
def func_start(va,maxback=0x4000):
    # heuristic: walk back to previous int3/nop padding before a push ebp / sub esp / etc
    o=va-IB
    for k in range(o,o-maxback,-1):
        if img[k-1] in (0xcc,0x90,0xc3) and img[k] not in (0xcc,0x90):
            # check aligned-ish
            if (k)%16==0 or img[k-1]==0xcc: return IB+k
    return None
