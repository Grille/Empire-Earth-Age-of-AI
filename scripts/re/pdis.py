# Print the disassembly at one or more addresses: python pdis.py <hex>...
import ee,sys
for a in sys.argv[1:]:
    print('----',a); ee.pdis(int(a,16),int(25))
