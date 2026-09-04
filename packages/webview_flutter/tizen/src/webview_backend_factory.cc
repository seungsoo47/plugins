// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webview_backend_factory.h"

#include "ewk_internal_api_binding.h"
#include "ewk_webview_backend.h"
#include "log.h"

std::unique_ptr<WebViewBackend> WebViewBackendFactory::Create(
    WebViewBackend::Delegate* delegate) {
  if (!EwkInternalApiBinding::GetInstance().Initialize()) {
    LOG_ERROR("Failed to initialize EWK internal APIs.");
    return nullptr;
  }
  return std::make_unique<EwkWebViewBackend>(delegate);
}

void WebViewBackendFactory::InitializeEngine() {
  EwkWebViewBackend::GlobalInitialize();
}

void WebViewBackendFactory::ShutdownEngine() {
  EwkWebViewBackend::GlobalShutdown();
}
