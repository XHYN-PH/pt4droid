#!/usr/bin/env python3
from pathlib import Path
import ctypes,os,random,shlex,subprocess,tempfile,texture2ddecoder
repo=Path(__file__).resolve().parents[3]
includes=['-I'+str(repo/'src'),'-I'+str(repo/'build/android/_deps/glm-src'),'-I'+str(repo/'build/android/_deps/volk-src'),'-I'+str(repo/'build/android/_deps/vulkan_headers-src/include')]
compiler=[os.environ.get('CXX','g++'),'-std=c++20','-O2']+shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))+includes
with tempfile.TemporaryDirectory() as directory:
 root=Path(directory);subprocess.run(compiler+[str(repo/'tests/mobile_next_refinement_test.cpp'),'-o',str(root/'checks')],check=True);subprocess.run([str(root/'checks')],check=True)
 wrapper=root/'decode.cpp';wrapper.write_text('#include "engine/assets/bc_texture.h"\nextern "C" int decode(int f,unsigned w,unsigned h,const unsigned char* d,unsigned n,unsigned char* o,unsigned cap){std::vector<uint8_t> out;if(!pt::DecodeBcMip(VkFormat(f),w,h,{d,n},out)||out.size()>cap)return -1;std::copy(out.begin(),out.end(),o);return out.size();}\n')
 subprocess.run(compiler+['-fPIC','-shared',str(wrapper),str(repo/'src/engine/assets/bc_texture.cpp'),'-o',str(root/'decode.so')],check=True)
 lib=ctypes.CDLL(str(root/'decode.so'));lib.decode.argtypes=[ctypes.c_int,ctypes.c_uint,ctypes.c_uint,ctypes.c_void_p,ctypes.c_uint,ctypes.c_void_p,ctypes.c_uint]
 def decode(f,w,h,d):
  out=ctypes.create_string_buffer(w*h*4);n=lib.decode(f,w,h,d,len(d),out,len(out));return None if n<0 else out.raw[:n]
 rng=random.Random(8472);count=0
 for fmt,reference,block,channels in [(133,texture2ddecoder.decode_bc1,8,4),(137,texture2ddecoder.decode_bc3,16,4),(141,texture2ddecoder.decode_bc5,16,2),(145,texture2ddecoder.decode_bc7,16,4)]:
  for trial in range(2200):
   w,h=rng.randrange(1,16),rng.randrange(1,16);data=bytearray(rng.randbytes(((w+3)//4)*((h+3)//4)*block))
   if fmt==145:
    for i in range(0,len(data),16):mode=trial%8;data[i]=(data[i]&~((1<<(mode+1))-1))|(1<<mode)
   if fmt==137:
    # The independent decoder uses BC1's reversed-endpoint branch incorrectly
    # for BC3. Compare its valid branch and check reversed BC3 with a golden block.
    for i in range(0,len(data),16):
     a=int.from_bytes(data[i+8:i+10],'little');b=int.from_bytes(data[i+10:i+12],'little')
     if a<=b:
      a,b=max(a,b),min(a,b)
      if a==b:a,b=65535,0
      data[i+8:i+10]=a.to_bytes(2,'little');data[i+10:i+12]=b.to_bytes(2,'little')
   actual=decode(fmt,w,h,bytes(data));ref=reference(bytes(data),w,h);assert actual is not None
   compared=3 if fmt==133 else channels;tolerance=0 if fmt==145 else 1
   for i in range(w*h):
    values=(ref[i*4+2],ref[i*4+1],ref[i*4],ref[i*4+3])[:compared]
    assert max(abs(actual[i*channels+k]-values[k])for k in range(compared))<=tolerance,(fmt,trial,i)
   count+=1
 transparent=b'\0\0\xff\xff'+b'\xff'*4;assert decode(133,4,4,transparent)==bytes(64);assert decode(131,4,4,transparent)==bytes([0,0,0,255])*16
 assert decode(135,4,4,b'\xff'*8+b'\0\xf8\0\xf8'+bytes(4))==bytes([255,0,0,255])*16
 assert decode(137,4,4,bytes([255,0])+bytes(6)+transparent)==bytes([170,170,170,255])*16
 assert decode(133,4,4,bytes(7))is None;assert decode(133,4,4,bytes(9))is None;assert decode(37,4,4,bytes(16))is None
 print('BC fallback:',count,'independent comparisons plus alpha, BC2 and reversed BC3 golden cases passed')
