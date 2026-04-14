// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of '../google_maps_flutter_tizen.dart';

/// The `GroundOverlayController` class wraps a [GGroundOverlay] and its tap event.
class GroundOverlayController {
  /// Creates a `GroundOverlayController` that wraps a [GGroundOverlay] object and its `onTap` behavior.
  GroundOverlayController({
    required util.GGroundOverlay groundOverlay,
    bool consumeTapEvents = false,
    ui.VoidCallback? onTap,
    WebViewController? controller,
  })  : _groundOverlay = groundOverlay,
        _consumeTapEvents = consumeTapEvents,
        tapEvent = onTap {
    if (controller != null) {
      _addGroundOverlayEvent(controller);
    }
  }

  util.GGroundOverlay? _groundOverlay;
  final bool _consumeTapEvents;

  /// GroundOverlay's tap event.
  ui.VoidCallback? tapEvent;

  Future<void> _addGroundOverlayEvent(WebViewController? controller) async {
    final String command = '''
        ${_groundOverlay!}.addListener("click", (event) => GroundOverlayClick.postMessage(JSON.stringify(${_groundOverlay?.id})));''';
    await controller!.runJavaScript(command);
  }

  /// Returns `true` if this Controller will use its own `onTap` handler to consume events.
  bool get consumeTapEvents => _consumeTapEvents;

  /// Returns the [GGroundOverlay] associated to this controller.
  util.GGroundOverlay? get groundOverlay => _groundOverlay;

  /// Updates the options of the wrapped [GGroundOverlay] object.
  void update(util.GGroundOverlayOptions options) {
    if (_groundOverlay != null) {
      _groundOverlay!.opacity = options.opacity;
      _groundOverlay!.clickable = options.clickable;
      _groundOverlay!.map = options.map;
    }
  }

  /// Disposes of the currently wrapped [GGroundOverlay].
  void remove() {
    if (_groundOverlay != null) {
      _groundOverlay!.map = null;
      _groundOverlay = null;
    }
  }
}
