import ee,re,struct,sys
T0=ee.text_va-ee.IB; seg=ee.img[T0:T0+ee.text_sz]
def disp_users(d):
    b=struct.pack('<I',d); out=[]
    for m in re.finditer(re.escape(b),seg):
        va=ee.text_va+m.start()
        for back in range(2,8):
            l=list(ee.md.disasm(ee.rd(va-back,16),va-back,count=1))
            if l and l[0].address+l[0].size>=va+4 and hex(d) in l[0].op_str:
                out.append(l[0]); break
    return out
if __name__=='__main__':
    for a in sys.argv[1:]:
        d=int(a,16)
        us=disp_users(d); print('### disp',a,len(us))
        for i in us: print(f'  {i.address:08x}: {i.mnemonic} {i.op_str}')
