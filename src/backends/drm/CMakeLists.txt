set(DRM_SOURCES
    drm_backend.cpp
    drm_object.cpp
    drm_property.cpp
    drm_object_connector.cpp
    drm_object_crtc.cpp
    drm_object_plane.cpp
    drm_output.cpp
    drm_buffer.cpp
    edid.cpp
    logging.cpp
    scene_qpainter_drm_backend.cpp
    drm_gpu.cpp
    dumb_swapchain.cpp
    shadowbuffer.cpp
    drm_layer.cpp
    drm_pipeline.cpp
    drm_pipeline_legacy.cpp
    drm_abstract_output.cpp
    drm_virtual_output.cpp
    drm_lease_output.cpp
    drm_qpainter_layer.cpp
    egl_gbm_backend.cpp
    egl_gbm_layer.cpp
    drm_buffer_gbm.cpp
    gbm_surface.cpp
    placeholderinputeventfilter.cpp
    virtual_egl_gbm_layer.cpp
    drm_lease_egl_gbm_layer.cpp
    egl_gbm_layer_surface.cpp
    dmabuf_feedback.cpp
    drm_dumb_buffer.cpp
    egl_gbm_cursor_layer.cpp
)

add_library(KWinWaylandDrmBackend MODULE ${DRM_SOURCES})
set_target_properties(KWinWaylandDrmBackend PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/org.kde.kwin.waylandbackends/")
target_link_libraries(KWinWaylandDrmBackend kwin Libdrm::Libdrm gbm::gbm PkgConfig::Libxcvt)

install(
    TARGETS
        KWinWaylandDrmBackend
    DESTINATION
        ${KDE_INSTALL_PLUGINDIR}/org.kde.kwin.waylandbackends/
)
