// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webview_backend_factory.h"

#include <system_info.h>

#include <cstdio>
#include <cstdlib>

#include "ewk_internal_api_binding.h"
#include "ewk_webview_backend.h"
#include "log.h"
#include "wv_internal_api_binding.h"
#include "wv_webview_backend.h"

namespace {

enum class BackendKind { kEwk, kWvStandalone, kWvWrapper };

// EWK through Tizen 10.0, WV wrapper mode on 10.1, WV standalone from 11.0.
BackendKind DefaultBackendForPlatform() {
  char* value = nullptr;
  int major = 0, minor = 0;
  if (system_info_get_platform_string(
          "http://tizen.org/feature/platform.version", &value) ==
          SYSTEM_INFO_ERROR_NONE &&
      value) {
    std::sscanf(value, "%d.%d", &major, &minor);
    free(value);
  }
  if (major >= 11) {
    return BackendKind::kWvStandalone;
  }
  if (major == 10 && minor >= 1) {
    return BackendKind::kWvWrapper;
  }
  return BackendKind::kEwk;
}

// Cached after the first read so InitializeEngine(), Create(), and
// ShutdownEngine() always agree on one answer for the process lifetime.
BackendKind SelectedBackend() {
  static const BackendKind kind = []() {
    BackendKind selected = DefaultBackendForPlatform();
    if (selected == BackendKind::kWvStandalone) {
      LOG_INFO("WebView backend: WV (standalone mode).");
    } else if (selected == BackendKind::kWvWrapper) {
      LOG_INFO("WebView backend: WV (EWK wrapper mode).");
    }
    return selected;
  }();
  return kind;
}

bool g_wv_engine_initialized = false;

}  // namespace

std::unique_ptr<WebViewBackend> WebViewBackendFactory::Create(
    WebViewBackend::Delegate* delegate) {
  BackendKind kind = SelectedBackend();
  if (kind != BackendKind::kEwk) {
    // Failing here must not fall back to EWK silently.
    if (!WvInternalApiBinding::GetInstance().Initialize()) {
      LOG_ERROR("Failed to initialize WV APIs.");
      return nullptr;
    }
    // Wrapper mode enables offscreen rendering through EWK (see
    // WvWebViewBackend::Create), so its EWK binding has to resolve too.
    if (kind == BackendKind::kWvWrapper &&
        !EwkInternalApiBinding::GetInstance().Initialize()) {
      LOG_ERROR("Failed to initialize EWK internal APIs for WV wrapper mode.");
      return nullptr;
    }
    return std::make_unique<WvWebViewBackend>(
        delegate, kind == BackendKind::kWvStandalone);
  }
  if (!EwkInternalApiBinding::GetInstance().Initialize()) {
    LOG_ERROR("Failed to initialize EWK internal APIs.");
    return nullptr;
  }
  return std::make_unique<EwkWebViewBackend>(delegate);
}

void WebViewBackendFactory::InitializeEngine() {
  BackendKind kind = SelectedBackend();
  if (kind != BackendKind::kEwk) {
    if (!WvInternalApiBinding::GetInstance().Initialize()) {
      LOG_ERROR("Failed to initialize WV APIs; engine not started.");
      return;
    }
    WvWebViewBackend::GlobalInitialize(kind == BackendKind::kWvStandalone);
    g_wv_engine_initialized = true;
    return;
  }
  EwkWebViewBackend::GlobalInitialize();
}

void WebViewBackendFactory::ShutdownEngine() {
  if (SelectedBackend() != BackendKind::kEwk) {
    if (g_wv_engine_initialized) {
      WvWebViewBackend::GlobalShutdown();
      g_wv_engine_initialized = false;
    }
    return;
  }
  EwkWebViewBackend::GlobalShutdown();
}
