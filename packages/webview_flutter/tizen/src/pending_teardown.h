// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_PENDING_TEARDOWN_H_
#define FLUTTER_PLUGIN_PENDING_TEARDOWN_H_

#include <glib.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "buffer_pool.h"
#include "log.h"

template <typename Handle>
class PendingTeardownRegistry {
 public:
  std::function<void()> Prepare(Handle instance,
                                std::shared_ptr<BufferPool> pool,
                                void (*destroy)(Handle)) {
    auto pending = std::make_shared<Entry>();
    pending->instance = instance;
    pending->pool = std::move(pool);
    pending->destroy = destroy;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      entries_.push_back(pending);
    }
    return [this, pending]() { Complete(pending); };
  }

  void Flush() {
    constexpr gint64 kDeadlineUsec = 2 * G_USEC_PER_SEC;
    const gint64 deadline = g_get_monotonic_time() + kDeadlineUsec;
    for (;;) {
      bool deadline_passed = g_get_monotonic_time() >= deadline;
      std::vector<std::shared_ptr<Entry>> snapshot;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (entries_.empty()) {
          return;
        }
        if (deadline_passed) {
          snapshot = entries_;
        }
      }
      if (deadline_passed) {
        LOG_WARN("Forcing %zu pending teardown(s) past deadline",
                 snapshot.size());
        for (auto& pending : snapshot) {
          Complete(pending);
        }
        continue;
      }
      if (!g_main_context_iteration(g_main_context_default(), FALSE)) {
        g_usleep(1000);
      }
    }
  }

 private:
  struct Entry {
    Handle instance{};
    std::shared_ptr<BufferPool> pool;
    std::atomic<bool> completed{false};
    void (*destroy)(Handle) = nullptr;
  };

  void Complete(const std::shared_ptr<Entry>& pending) {
    bool expected = false;
    if (pending->completed.compare_exchange_strong(expected, true) &&
        pending->instance) {
      pending->destroy(pending->instance);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(entries_.begin(), entries_.end(), pending);
    if (it != entries_.end()) {
      entries_.erase(it);
    }
  }

  std::mutex mutex_;
  std::vector<std::shared_ptr<Entry>> entries_;
};

#endif  // FLUTTER_PLUGIN_PENDING_TEARDOWN_H_
