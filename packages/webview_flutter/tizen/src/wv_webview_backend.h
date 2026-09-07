// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_WV_WEBVIEW_BACKEND_H_
#define FLUTTER_PLUGIN_WV_WEBVIEW_BACKEND_H_

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "webview_backend.h"
#include "wv_internal_api_binding.h"

// WebViewBackend implementation on top of the WV public API. Experimental
// and strictly opt-in; EWK remains the default backend (see
// WebViewBackendFactory).
//
// The WV engine has two implementations behind the same wv_* surface, and
// wv_init() picks one from the command line: with --enable-wv-standalone it
// runs its own implementation, without it every wv_* call is forwarded to
// its ewk_* counterpart ("wrapper mode", g_use_ewk_api == true). Which one
// runs is decided once, process-wide, by GlobalInitialize()'s `standalone`.
class WvWebViewBackend : public WebViewBackend {
 public:
  explicit WvWebViewBackend(Delegate* delegate);
  ~WvWebViewBackend() override = default;

  bool Create(double width, double height, void* window,
              bool engine_policy) override;
  std::function<void()> PrepareTeardown(
      std::shared_ptr<BufferPool> pool) override;

  void Offset(double left, double top) override;
  void Resize(double width, double height) override;
  void Touch(int type, int button, double x, double y, double dx,
             double dy) override;
  bool SendKey(const char* key, const char* string, const char* compose,
               uint32_t modifiers, uint32_t scan_code, bool is_down) override;
  void Resume() override;
  void Stop() override;

  void SetJavaScriptEnabled(bool enabled) override;
  void SetHasNavigationDelegate(bool has_navigation_delegate) override;

  void LoadUrl(const std::string& url) override;
  bool LoadUrlRequest(const std::string& url, int32_t method,
                      const std::map<std::string, std::string>& headers,
                      const std::vector<uint8_t>& body) override;
  void LoadHtmlString(const std::string& html,
                      const std::string& base_url) override;
  bool CanGoBack() override;
  bool CanGoForward() override;
  void GoBack() override;
  void GoForward() override;
  void Reload() override;
  std::string GetCurrentUrl() override;
  void EvaluateJavaScript(const std::string& javascript,
                          std::function<void(const char*)> callback) override;
  void RegisterJavaScriptChannel(const std::string& name) override;
  void ClearCache() override;
  void ClearLocalStorage() override;
  std::string GetTitle() override;
  void ScrollTo(int32_t x, int32_t y) override;
  void ScrollBy(int32_t x, int32_t y) override;
  void GetScrollPosition(int32_t* x, int32_t* y) override;
  void SetBackgroundColor(int r, int g, int b, int a) override;
  void SetUserAgent(const std::string& user_agent) override;
  std::string GetUserAgent() override;
  void EnableZoom(bool enabled) override;
  void JavaScriptAlertReply() override;
  void JavaScriptConfirmReply(bool result) override;
  void JavaScriptPromptReply(const std::string& result) override;
  void SetScrollbarVisible(bool visible) override;
  bool ClearCookies() override;

  // Must be called exactly once, before any WvWebViewBackend is
  // constructed, and only after WvInternalApiBinding::Initialize() has
  // succeeded. Registers the composite argv via wv_set_arguments() and then
  // calls wv_init() — unlike the EWK path, the arguments must be registered
  // before engine init. `standalone` adds --enable-wv-standalone to that
  // argv. Returns whether wv_init() itself succeeded.
  static bool GlobalInitialize(bool standalone);

  // Must be called exactly once, after every WvWebViewBackend has been
  // destroyed.
  static void GlobalShutdown();

 private:
  static void FlushPendingTeardowns();

  static void OnFrameRendered(wv_view_h obj, void* event_info, void* user_data);
  static void OnLoadStarted(wv_view_h obj, void* event_info, void* user_data);
  static void OnLoadFinished(wv_view_h obj, void* event_info, void* user_data);
  static void OnProgress(wv_view_h obj, void* event_info, void* user_data);
  static void OnLoadError(wv_view_h obj, void* event_info, void* user_data);
  static void OnConsoleMessage(wv_view_h obj, void* event_info,
                               void* user_data);
  static void OnNavigationPolicy(wv_view_h obj, void* event_info,
                                 void* user_data);
  static void OnResponsePolicy(wv_view_h obj, void* event_info,
                               void* user_data);
  static void OnUrlChange(wv_view_h obj, void* event_info, void* user_data);
  static void OnEvaluateJavaScript(wv_view_h obj, const char* result_value,
                                   void* user_data);
  static void OnJavaScriptMessage(wv_view_h obj, wv_script_message_s message);
  static bool OnJavaScriptAlertDialog(wv_view_h view, const char* message,
                                      void* data);
  static bool OnJavaScriptConfirmDialog(wv_view_h view, const char* message,
                                        void* data);
  static bool OnJavaScriptPromptDialog(wv_view_h view, const char* message,
                                       const char* default_text, void* data);

  void SendTouchEvent(int type, double x, double y);
  void SendMouseEvent(int type, int button, double x, double y, double dx,
                      double dy);

  Delegate* delegate_;
  bool scrollbar_enabled_ = true;
  wv_view_h view_ = nullptr;
  void* window_ = nullptr;
  bool has_navigation_delegate_ = false;
  wv_mouse_button_type_e mouse_button_type_ =
      static_cast<wv_mouse_button_type_e>(0);
};

#endif  // FLUTTER_PLUGIN_WV_WEBVIEW_BACKEND_H_
