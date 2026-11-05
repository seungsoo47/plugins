#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "flutter_frame_capturer.h"

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <vector>

#include "svpng.hpp"

namespace flutter_webrtc_plugin {

FlutterFrameCapturer::FlutterFrameCapturer(RTCVideoTrack* track,
                                           std::string path) {
  track_ = track;
  path_ = path;
}

void FlutterFrameCapturer::OnFrame(scoped_refptr<RTCVideoFrame> frame) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (frame_ != nullptr) {
      return;
    }
    frame_ = frame.get()->Copy();
  }
  cv_.notify_one();
}

void FlutterFrameCapturer::CaptureFrame(
    std::unique_ptr<MethodResultProxy> result) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    frame_ = nullptr;
  }

  track_->AddRenderer(this);
  bool got_frame = WaitForFrame();
  track_->RemoveRenderer(this);

  std::shared_ptr<MethodResultProxy> result_ptr(result.release());
  if (!got_frame) {
    result_ptr->Error("2", "Frame capture timed out");
  } else if (SaveFrame()) {
    result_ptr->Success();
  } else {
    result_ptr->Error("1", "Cannot save the frame as .png file");
  }
}

bool FlutterFrameCapturer::WaitForFrame() {
  std::unique_lock<std::mutex> lock(mutex_);
  return cv_.wait_for(lock, std::chrono::seconds(5),
                      [this] { return frame_ != nullptr; });
}

bool FlutterFrameCapturer::SaveFrame() {
  if (frame_ == nullptr) {
    return false;
  }

  int width = frame_.get()->width();
  int height = frame_.get()->height();
  int bytes_per_pixel = 4;
  std::vector<uint8_t> pixels(static_cast<size_t>(width) * height *
                              bytes_per_pixel);

  frame_.get()->ConvertToARGB(RTCVideoFrame::Type::kABGR, pixels.data(),
                              /* unused */ -1, width, height);

  FILE* file = fopen(path_.c_str(), "wb");
  if (!file) {
    return false;
  }

  svpng(file, width, height, pixels.data(), 1);
  fclose(file);
  return true;
}

}  // namespace flutter_webrtc_plugin