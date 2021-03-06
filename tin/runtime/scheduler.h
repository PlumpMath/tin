// Copyright (c) 2016 Tin Project. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include "tin/runtime/util.h"
#include "tin/runtime/guintptr.h"
#include "tin/runtime/unlock.h"
#include "tin/runtime/env.h"
#include "tin/runtime/raw_mutex.h"

namespace tin {
namespace runtime {
class P;
class M;

class Scheduler {
 public:
  Scheduler();

  G* FindRunnable(bool* inherit_time);

  void GlobalRunqPut(G* gp);
  void GlobalRunqPutHead(G* gp);
  void GlobalRunqBatch(G* ghead, G* gtail, int32 n);
  G*   GlobalRunqGet(P* p, int32 maximium);
  void InjectGList(G* glist);

  int32 GlobalRunqSize() {
    return runq_size_;
  }

  void PIdlePut(P* p);
  P* PIdleGet();

  void MPut(M* m);
  M* MGet();
  void MGetForP(P* curp, bool spinning, P** newp, M** newm);

  void Reschedule();
  bool OneRoundSched(G* curg);
  void G0Loop();

  int Init();
  void MakeReady(G* gp);
  void WakePIfNecessary();
  void WakeupP();
  void HandoffP(P* p);
  void ExitSyscall0(G* gp);
  bool ExitSyscallFast();
  bool ExitSyscallPIdle();
  void ResetSpinning();
  uint32 NrIdleP() {return nr_idlep_; }
  uint32 NrSpinning() {
    return nr_spinning_;
  }

  uint32 LastPollTime();
  uint32* MutableLastPollTime() {
    return &last_poll_;
  }

 private:
  void OnSwitch(G* curg);
  void DoUnlock(UnLockInfo* info);
  P** Allp() { return allp_;}
  P* ResizeProc(int nprocs);

 private:
  RawMutex lock_;
  GUintptr runq_head_;
  GUintptr runq_tail_;
  int32 runq_size_;

  P* idlep_;
  uint32 nr_idlep_;
  uint32 nr_spinning_;

  M* idlem_;      // idle m's waiting for work
  int32 nr_idlem_;         // number of idle m's waiting for work
  int32 nr_idlem_locked_;   // number of locked m's waiting for work
  int32 mcount_;        // number of m's that have been created
  int32 max_mcount_;      // maximum number of m's allowed (or die)

  uint32 last_poll_;

  P** allp_;

  friend class SchedulerLocker;
  DISALLOW_COPY_AND_ASSIGN(Scheduler);
};


class SchedulerLocker {
 public:
  SchedulerLocker() {
    sched->lock_.Lock();
  }

  ~SchedulerLocker() {
    sched->lock_.Unlock();
  }
};

P* ReleaseP();

void AcquireP(P* p);

void StartM(P* p, bool spinning);

void ParkUnlock(RawMutex* lock);

void Park(UnlockFunc unlockf = NULL, void* arg1 = NULL, void* arg2 = NULL);

void Ready(G* gp);

bool ParkUnlockF(void* arg1, void* arg2);

void DropG();

void EnterSyscallBlock();

void ExitSyscall();

void WakePIfNecessary();

void SwitchG(Greenlet* from, Greenlet* to, intptr_t args);

}  // namespace runtime
}  // namespace tin
