// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_WEBVIEW_BACKEND_FACTORY_H_
#define FLUTTER_PLUGIN_WEBVIEW_BACKEND_FACTORY_H_

#include <memory>

#include "webview_backend.h"

class WebViewBackendFactory {
 public:
  // Selects and constructs a backend for a single WebView. Returns nullptr
  // (after logging) if no backend's native API layer initializes
  // successfully.
  static std::unique_ptr<WebViewBackend> Create(
      WebViewBackend::Delegate* delegate);

  // Must be called exactly once, before any WebViewBackend is constructed.
  static void InitializeEngine();

  // Must be called exactly once, after every WebViewBackend has been
  // destroyed.
  static void ShutdownEngine();
};

#endif  // FLUTTER_PLUGIN_WEBVIEW_BACKEND_FACTORY_H_
