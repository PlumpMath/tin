// Copyright (c) 2016 Tin Project. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdlib>

#include "base/basictypes.h"
#include "base/threading/thread_local.h"

#include "tin/runtime/env.h"

namespace tin {
namespace runtime {

class Greenlet;
class P;
class M;
typedef Greenlet G;

inline G* GetG() {
  return glet_tls->Get();
}

inline void SetG(G* gp) {
  glet_tls->Set(gp);
}

P* GetP();

M* GetM();

inline uintptr_t GpCast(G* gp) {
  return reinterpret_cast<uintptr_t>(gp);
}

inline G* GpCastBack(uintptr_t gp) {
  return reinterpret_cast<G*>(gp);
}

void YieldLogicProcessor();
void YieldLogicProcessor(int n);
int GetLastSystemErrorCode();

}  // namespace runtime
}  // namespace tin
