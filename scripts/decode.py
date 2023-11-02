
import struct

MEASLEN = 20
with open('/tmp/a.out','rb') as f:
  d = f.read()
  num_bytes = len(d)



  for i in range(0,100000):
    idx = i * MEASLEN 
    xy,yz,x,y,z = struct.unpack('<iifff',d[idx:idx+MEASLEN]) 
    #print(xy,yz,x,y,z)
    print(f'{x},{y},{z}')

