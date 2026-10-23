import pefile,sys,json,struct
pe=pefile.PE(sys.argv[1])
out={}
for t in pe.DIRECTORY_ENTRY_RESOURCE.entries:
    if t.id!=6: continue
    for blk in t.directory.entries:
        for lang in blk.directory.entries:
            dr=lang.data.struct; d=pe.get_data(dr.OffsetToData,dr.Size)
            p=0
            for i in range(16):
                n=struct.unpack('<H',d[p:p+2])[0]; p+=2
                s=d[p:p+2*n].decode('utf-16le'); p+=2*n
                if s: out[(blk.id-1)*16+i]=s
json.dump(out,open('local/games/ee/strings_de.json','w',encoding='utf8'),ensure_ascii=False,indent=0)
print(len(out))
