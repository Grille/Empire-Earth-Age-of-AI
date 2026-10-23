import ee,re,struct,sys
T0=ee.text_va-ee.IB; seg=ee.img[T0:T0+ee.text_sz]
def branch_refs(t):
    out=[]
    for op in (b'\xe8',b'\xe9'):
        for m in re.finditer(re.escape(op),seg):
            va=ee.text_va+m.start(); rel=struct.unpack('<i',seg[m.start()+1:m.start()+5])[0]
            if va+5+rel==t: out.append((op.hex(),va))
    for x in ee.xrefs(t): out.append(('ptr',x))
    return out
if __name__=='__main__':
    for a in sys.argv[1:]:
        print(a,[(k,hex(v)) for k,v in branch_refs(int(a,16))])
