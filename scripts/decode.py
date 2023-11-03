
import struct

MEASLEN = 24 
with open('/tmp/a.out','rb') as f:
  d = f.read()
  num_bytes = len(d)

  for i in range(0,num_bytes//MEASLEN):
    idx = i * MEASLEN 
    xy,yz,x,y,z,xydir,yzdir = struct.unpack('<iifffhh',d[idx:idx+MEASLEN]) 
    #print(xy,yz,x,y,z)
    print(f'{x},{y},{z}')

