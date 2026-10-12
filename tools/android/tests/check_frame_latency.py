#!/usr/bin/env python3
"""Exercise production frame budgets and direct look at varied render rates."""
from pathlib import Path
import os, re, shlex, subprocess, tempfile

repo = Path(__file__).resolve().parents[3]
source = (repo / 'src/main.cpp').read_text()
start = source.index('if (paced && !theater && !freecam && !pt::game::EndingOutroInputBlocked')
guard = source[start:source.index('{', start)]
guard = guard[guard.index('(') + 1:guard.rfind(')')]
player = (repo / 'src/game/player.h').read_text()
pitch = '\n'.join(line.strip() for line in player.splitlines() if
                 'static constexpr float kPitch' in line or 'static float ClampPitch(' in line)
code = r'''
#include <cassert>
#include <cmath>
#include <cstdio>
#include <random>
#include "engine/platform/frame_limit.h"
#include "game/render_mouse.h"
namespace pt { enum class PromptDevice {Keyboard, PlayStation, Xbox}; }
namespace pt::game { bool EndingOutroInputBlocked(int step) {return step==99;} }
struct MockGame {
 bool paused=false, demo=false; int step=0, mask=0;
 struct Locks {int bits=0;int Mask(char)const{return bits;}};
 struct Player {Locks locks;}; struct Demo {bool active;bool ControlsPlayer()const{return active;}};
 struct Control {int step;int Step()const{return step;}};
 bool Paused()const{return paused;} Demo Demos()const{return {demo};}
 Control Controller()const{return {step};} Player GetPlayer()const{return {{mask}};}
};
bool Allowed(MockGame& game,pt::PromptDevice device,bool paced=true,bool freecam=false,
             bool scripted_camera=false,void* theater=nullptr) {
 struct {struct {pt::PromptDevice device;}prompts;} polled{{device}};
 return (GUARD);
}
int main() {
 int checks=0;
 for(int cap:{0,20,30,45,60,90,120,144,240}) {
  for(int64_t elapsed:{0LL,1LL,6'000'000LL,12'000'000LL,16'666'666LL,19'000'000LL,33'000'000LL,100'000'000LL}) {
   const auto wait=pt::FrameLimitWaitNs(elapsed,cap);
   assert(wait>=0);
   if(cap<=0)assert(wait==0);
   else {const int64_t budget=1'000'000'000LL/cap;
         assert(elapsed>=budget ? wait==0 : elapsed+wait==budget);}
   ++checks;
  }
 }
 assert(pt::FrameLimitWaitNs(19'000'000,60)==0);
 assert(pt::FrameLimitWaitNs(33'000'000,60)==0);
 // Direct look remains independent of the prompt family, including mixed pad movement.
 MockGame game;
 for(auto family:{pt::PromptDevice::Keyboard,pt::PromptDevice::PlayStation,pt::PromptDevice::Xbox}) {
  assert(Allowed(game,family));game.paused=true;assert(!Allowed(game,family));game.paused=false;
  game.demo=true;assert(!Allowed(game,family));game.demo=false;
  game.mask=2;assert(!Allowed(game,family));game.mask=0;
  game.step=99;assert(!Allowed(game,family));game.step=0;
  assert(!Allowed(game,family,false));assert(!Allowed(game,family,true,true));
  assert(!Allowed(game,family,true,false,true));assert(!Allowed(game,family,true,false,false,&game));
  checks+=9;
 }
 std::mt19937 random(125);
 for(int hz:{30,45,50,60,72,90,120,144,240})for(bool mixed:{false,true}) {
  float accum=0,pending=0,consumed=0,total=0,current=0,previous=0,pad=0,pad_previous=0;
  const float tick=1.f/60;
  for(int f=0;f<hz*8;f++) {
   // Include stalled frames, no-input frames and touch reversals.
   float dt=(1.f/hz)*(1.f+(int(random()%9)-4)*.04f);
   float move=f%7==0 ? 0.f : (f%29<15 ? .6f : -.4f)*dt;
   pending+=move;total+=move;accum+=dt;
   while(accum>=tick) {
    previous=current;pad_previous=pad;consumed=pending;
    pad+=mixed?.3f*tick:0.f;current+=mixed?.3f*tick:0.f;
    current-=consumed;pending=0;accum-=tick;
   }
   const float blend=accum/tick;
   pt::Camera view;view.yaw=previous+(current-previous)*blend;
   view.position={1,2,3};view.roll=.2f;view.fov_y=.4f;
   auto shown=pt::game::RenderDeltaLook(view,{consumed,0},{pending,0},blend,false,false);
   const float expected=pad_previous+(pad-pad_previous)*blend-total;
   assert(std::abs(shown.yaw-expected)<5e-5f);
   assert(shown.position==view.position && shown.roll==view.roll && shown.fov_y==view.fov_y);
   ++checks;
  }
 }
 pt::Camera view;view.yaw=.7f;view.pitch=.2f;
 auto inverted=pt::game::RenderDeltaLook(view,{.2f,.1f},{.1f,.05f},.5f,true,true);
 assert(std::abs(inverted.yaw-.9f)<1e-6f && std::abs(inverted.pitch-.3f)<1e-6f);
 auto clamped=pt::game::RenderDeltaLook(view,{0,0},{0,-100},1,false,false);
 assert(clamped.pitch==pt::game::Player::kPitchUp);
 clamped=pt::game::RenderDeltaLook(view,{0,0},{0,100},1,false,false);
 assert(clamped.pitch==-pt::game::Player::kPitchDown);
 view.pitch=2.f;auto untouched=pt::game::RenderDeltaLook(view,{0,0},{0,0},.5f,false,false);
 assert(untouched.pitch==view.pitch && untouched.yaw==view.yaw);
 std::printf("Frame/look checks passed: %d cases; over-budget frames never sleep; mixed controls, pauses, locks, stalls, pitch limits and 30–240 Hz\n",checks);
}
'''.replace('GUARD', guard)
with tempfile.TemporaryDirectory() as folder:
    root=Path(folder);(root/'game').mkdir()
    (root/'game/player.h').write_text('#pragma once\nnamespace pt::game {struct Player {\n'+pitch+'\n};}\n')
    test=root/'latency.cpp';test.write_text(code)
    flags=shlex.split(os.environ.get('PT_TEST_CXX_FLAGS',''))
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-D__ANDROID__','-I'+str(root),
                    '-I'+str(repo/'src')]+flags+[str(test),'-o',str(root/'latency')],check=True,timeout=40)
    subprocess.run([str(root/'latency')],check=True,timeout=15)
