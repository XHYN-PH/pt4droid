from pathlib import Path
import os,shlex,subprocess,tempfile
repo=Path(__file__).resolve().parents[3]
code=r'''
#include <cassert>
#include <cstdio>
#include "engine/platform/frame_limit.h"
int main(){
 for(int fps:{20,30,40,45,60,90,120,144,240}){
  pt::FrameDeadline timer;const int64_t period=1000000000LL/fps;int64_t start=1000000000LL;
  const int64_t origin=start;
  for(int i=0;i<10000;i++){
   const auto work=period/3;auto wait=timer.Wait(start,start+work,fps);
   assert(wait>=0&&wait<=period);start+=work+wait+200000;
  }
  assert(start==origin+10000*period+200000); // Oversleep never accumulates each frame.
  auto wait=timer.Wait(start,start+5*period,fps);assert(wait==0);
  start+=5*period;wait=timer.Wait(start,start+period/3,fps);assert(wait==period-period/3);
  start+=1000000000LL;wait=timer.Wait(start,start+period/3,fps);assert(wait==period-period/3);
  assert(timer.Wait(start,start,0)==0);assert(timer.Wait(start,start+period/3,fps)==period-period/3);
  timer.Reset();assert(timer.Wait(start,start+period,fps)==0);
 }
 pt::FrameDeadline changing;
 assert(changing.Wait(1000000000,1001000000,60)==1000000000/60-1000000);
 assert(changing.Wait(1020000000,1021000000,90)==1000000000/90-1000000);
 puts("Frame deadlines: 90000 oversleep frames, stalls/resume, unlimited/pacing handoff and requested caps passed.");
}
'''
with tempfile.TemporaryDirectory()as directory:
 p=Path(directory);(p/'test.cpp').write_text(code)
 subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-O2','-I'+str(repo/'src'),str(p/'test.cpp'),'-o',str(p/'check')],check=True)
 subprocess.run([str(p/'check')],check=True)
