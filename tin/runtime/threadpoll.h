// Copyright (c) 2016 Tin Project. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once
#include <cstdlib>
#include <vector>
#include <deque>

#include "base/basictypes.h"
#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/condition_variable.h"
#include "tin/runtime/env.h"

namespace tin {
namespace runtime {

class M;
// consider replace with shared_ptr<Work>
class  Work {
 public:
  Work() { }
  virtual ~Work() { }
  virtual void Run() = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(Work);
};

class GletWork : public Work {
 public:
  GletWork();
  virtual ~GletWork() {}
  virtual void Run() = 0;
  int LastError() const {return last_error_;}
  void SaveLastError(int err) {last_error_ = err;}

 protected:
  void Resume();
  void Finalize();

 private:
  int last_error_;
  G* gp_;
};

void SubmitGletWork(GletWork* work);
void SubmitGetAddrInfoGletWork(GletWork* work);

class ThreadPoll {
 public:
  ThreadPoll();

  static ThreadPoll* GetInstance() {
    return Singleton<ThreadPoll>::get();
  }

  void Start();
  void JoinAll();
  void AddWork(Work* work);
  void Run();

 private:
  int num_threads_;
  std::vector<M*> threads_;
  std::deque<Work*> tasks_;
  base::Lock lock_;
  base::WaitableEvent dry_;
  DISALLOW_COPY_AND_ASSIGN(ThreadPoll);
};

}  // namespace runtime
}  // namespace tin



