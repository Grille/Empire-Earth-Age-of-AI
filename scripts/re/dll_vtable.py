# An engine DLL's exported vtable, slot by slot: the function in each and its exported name (the DLLs export their
# classes' members - DX7HRTnLDisplay.dll's DX7Rasterizer, Low-Level Engine.dll's GE* - so a slot the exe calls through
# names itself). For the spec's [slot]s of a DLL's class.
# usage: python dll_vtable.py <dll> <Class> [<Class> ...]   e.g. "../DX7HRTnLDisplay.dll" DX7Rasterizer
import sys
import pefile


def main():
    pe = pefile.PE(sys.argv[1])
    base = pe.OPTIONAL_HEADER.ImageBase
    names = {}
    vtables = {}
    for e in pe.DIRECTORY_ENTRY_EXPORT.symbols:
        if not e.name:
            continue
        n = e.name.decode()
        if n.startswith('??_7') and n.endswith('@@6B@'):
            vtables[n[4:-5]] = e.address
        else:
            names.setdefault(e.address, []).append(n)
    text = next(s for s in pe.sections if s.Characteristics & 0x20000000)
    lo, hi = text.VirtualAddress, text.VirtualAddress + text.Misc_VirtualSize
    for cls in sys.argv[2:]:
        if cls not in vtables:
            print('%s: no exported vtable' % cls)
            continue
        vt = vtables[cls]
        print('%s vtable rva %05x' % (cls, vt))
        k = 0
        while True:
            f = pe.get_dword_at_rva(vt + 4 * k) - base
            if not lo <= f < hi:
                break
            print('  [%2d] +0x%03x  rva %05x  %s' % (k, 4 * k, f, ' '.join(names.get(f, ['-']))))
            k += 1


main()
