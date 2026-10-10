# Show every code reference to fields of AI settings records (table 0x8CFA80): python cpbehavior_refs.py <record>...
import ee,sys
T=(ee.text_va,ee.text_sz)
BASE=0x8cfa80
F=['name','idx','min1','max1','val1','min2','max2','val2','f8','minf','maxf','scale','fval','flag']
def refs_for(idx):
    out=[]
    for k,f in enumerate(F):
        a=BASE+idx*0x38+k*4
        for x in ee.xrefs(a,T): out.append((f,hex(a),x))
    return out
for idx in map(int,sys.argv[1:]):
    r=refs_for(idx); print('#### record',idx,len(r))
    for f,a,x in r:
        print('==',f,a,'at',hex(x)); ee.pdis(x-36,28,False)
