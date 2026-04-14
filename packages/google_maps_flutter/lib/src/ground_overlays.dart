// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

part of '../google_maps_flutter_tizen.dart';

/// This class manages all the [GroundOverlayController]s associated to a [GoogleMapController].
class GroundOverlaysController extends GeometryController {
  /// Initialize the cache. The [StreamController] comes from the [GoogleMapController], and is shared with other controllers.
  GroundOverlaysController({required StreamController<MapEvent<Object?>> stream})
      : _streamController = stream,
        _groundOverlayIdToController = <GroundOverlayId, GroundOverlayController>{},
        _idToGroundOverlayId = <int, GroundOverlayId>{};

  // A cache of [GroundOverlayController]s indexed by their [GroundOverlayId].
  final Map<GroundOverlayId, GroundOverlayController> _groundOverlayIdToController;
  final Map<int, GroundOverlayId> _idToGroundOverlayId;

  // The stream over which ground overlays broadcast their events
  final StreamController<MapEvent<Object?>> _streamController;

  /// Adds a set of [GroundOverlay] objects to the cache.
  ///
  /// Wraps each [GroundOverlay] into its corresponding [GroundOverlayController].
  void addGroundOverlays(Set<GroundOverlay> groundOverlaysToAdd) {
    groundOverlaysToAdd.forEach(_addGroundOverlay);
  }

  LatLngBounds _computeBounds(GroundOverlay groundOverlay) {
    if (groundOverlay.bounds != null) {
      return groundOverlay.bounds!;
    }

    final double width = groundOverlay.width!;
    final double height = groundOverlay.height ?? width;
    final Offset anchor = groundOverlay.anchor ?? const Offset(0.5, 0.5);

    const double metersPerDegree = 111320.0;
    const double latPerMeter = 1.0 / metersPerDegree;
    final double lngPerMeter =
        1.0 / (metersPerDegree * cos(groundOverlay.position!.latitude * pi / 180.0));

    final double southLat = groundOverlay.position!.latitude -
        (height * (1.0 - anchor.dy)) * latPerMeter;
    final double northLat =
        groundOverlay.position!.latitude + (height * anchor.dy) * latPerMeter;
    final double westLng = groundOverlay.position!.longitude -
        (width * anchor.dx) * lngPerMeter;
    final double eastLng = groundOverlay.position!.longitude +
        (width * (1.0 - anchor.dx)) * lngPerMeter;

    return LatLngBounds(
      southwest: LatLng(southLat, westLng),
      northeast: LatLng(northLat, eastLng),
    );
  }

  void _addGroundOverlay(GroundOverlay? groundOverlay) {
    if (groundOverlay == null) {
      return;
    }

    final String bounds = boundsLiteral(_computeBounds(groundOverlay));
    final String url = urlFromMapBitmap(groundOverlay.image);

    final util.GGroundOverlayOptions groundOverlayOptions =
        util.GGroundOverlayOptions()
          ..opacity = 1.0 - groundOverlay.transparency
          ..clickable = groundOverlay.clickable
          ..zIndex = groundOverlay.zIndex
          ..map = groundOverlay.visible ? 'map' : null;

    final util.GGroundOverlay gGroundOverlay =
        util.GGroundOverlay(url, bounds, groundOverlayOptions);
    final GroundOverlayController controller = GroundOverlayController(
      groundOverlay: gGroundOverlay,
      onTap: () {
        _onGroundOverlayTap(groundOverlay.groundOverlayId);
      },
      controller: util.webController,
    );
    _idToGroundOverlayId[gGroundOverlay.id] = groundOverlay.groundOverlayId;
    _groundOverlayIdToController[groundOverlay.groundOverlayId] = controller;
  }

  /// Updates a set of [GroundOverlay] objects with new options.
  void changeGroundOverlays(Set<GroundOverlay> groundOverlaysToChange) {
    groundOverlaysToChange.forEach(_changeGroundOverlay);
  }

  void _changeGroundOverlay(GroundOverlay groundOverlay) {
    final GroundOverlayController? controller =
        _groundOverlayIdToController[groundOverlay.groundOverlayId];

    if (controller == null || controller.groundOverlay == null) {
      return;
    }

    final String bounds = boundsLiteral(_computeBounds(groundOverlay));
    final String url = urlFromMapBitmap(groundOverlay.image);

    final util.GGroundOverlayOptions groundOverlayOptions =
        util.GGroundOverlayOptions()
          ..opacity = 1.0 - groundOverlay.transparency
          ..clickable = groundOverlay.clickable
          ..zIndex = groundOverlay.zIndex
          ..map = groundOverlay.visible ? 'map' : null;

    if (controller.groundOverlay!.url != url ||
        controller.groundOverlay!.bounds != bounds) {
      _idToGroundOverlayId.remove(controller.groundOverlay!.id);
      controller.remove();
      final util.GGroundOverlay gGroundOverlay =
          util.GGroundOverlay(url, bounds, groundOverlayOptions);
      controller.groundOverlay = gGroundOverlay;
      _idToGroundOverlayId[gGroundOverlay.id] = groundOverlay.groundOverlayId;
    } else {
      controller.update(groundOverlayOptions);
    }
  }

  /// Removes a set of [GroundOverlayId]s from the cache.
  void removeGroundOverlays(Set<GroundOverlayId> groundOverlayIdsToRemove) {
    groundOverlayIdsToRemove.forEach(_removeGroundOverlay);
  }

  // Removes a ground overlay and its controller by its [GroundOverlayId].
  void _removeGroundOverlay(GroundOverlayId groundOverlayId) {
    final GroundOverlayController? controller = _groundOverlayIdToController[groundOverlayId];
    controller?.remove();
    _groundOverlayIdToController.remove(groundOverlayId);
  }

  // Handles the global onGroundOverlayTap function to funnel events into the stream.
  bool _onGroundOverlayTap(GroundOverlayId groundOverlayId) {
    _streamController.add(GroundOverlayTapEvent(mapId, groundOverlayId));
    return _groundOverlayIdToController[groundOverlayId]?.consumeTapEvents ?? false;
  }
}
