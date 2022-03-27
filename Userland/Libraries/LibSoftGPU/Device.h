/*
 * Copyright (c) 2021, Stephan Unverwerth <s.unverwerth@serenityos.org>
 * Copyright (c) 2022, Jelle Raaijmakers <jelle@gmta.nl>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Array.h>
#include <AK/NonnullRefPtr.h>
#include <AK/RefPtr.h>
#include <AK/Vector.h>
#include <LibGPU/DeviceInfo.h>
#include <LibGPU/Enums.h>
#include <LibGPU/ImageFormat.h>
#include <LibGPU/Light.h>
#include <LibGPU/Material.h>
#include <LibGPU/SamplerConfig.h>
#include <LibGPU/StencilConfiguration.h>
#include <LibGfx/Bitmap.h>
#include <LibGfx/Matrix3x3.h>
#include <LibGfx/Matrix4x4.h>
#include <LibGfx/Rect.h>
#include <LibGfx/Vector4.h>
#include <LibSoftGPU/AlphaBlendFactors.h>
#include <LibSoftGPU/Buffer/FrameBuffer.h>
#include <LibSoftGPU/Buffer/Typed2DBuffer.h>
#include <LibSoftGPU/Clipper.h>
#include <LibSoftGPU/Config.h>
#include <LibSoftGPU/Image.h>
#include <LibSoftGPU/Sampler.h>
#include <LibSoftGPU/Triangle.h>
#include <LibSoftGPU/Vertex.h>

namespace SoftGPU {

struct TexCoordGenerationConfig {
    GPU::TexCoordGenerationMode mode { GPU::TexCoordGenerationMode::EyeLinear };
    FloatVector4 coefficients {};
};

struct RasterizerOptions {
    bool shade_smooth { true };
    bool enable_stencil_test { false };
    bool enable_depth_test { false };
    bool enable_depth_write { true };
    bool enable_alpha_test { false };
    GPU::AlphaTestFunction alpha_test_func { GPU::AlphaTestFunction::Always };
    float alpha_test_ref_value { 0 };
    bool enable_blending { false };
    GPU::BlendFactor blend_source_factor { GPU::BlendFactor::One };
    GPU::BlendFactor blend_destination_factor { GPU::BlendFactor::One };
    u32 color_mask { 0xffffffff };
    float depth_min { 0.f };
    float depth_max { 1.f };
    GPU::DepthTestFunction depth_func { GPU::DepthTestFunction::Less };
    GPU::PolygonMode polygon_mode { GPU::PolygonMode::Fill };
    FloatVector4 fog_color { 0.0f, 0.0f, 0.0f, 0.0f };
    float fog_density { 1.0f };
    GPU::FogMode fog_mode { GPU::FogMode::Exp };
    bool fog_enabled { false };
    float fog_start { 0.0f };
    float fog_end { 1.0f };
    bool scissor_enabled { false };
    bool normalization_enabled { false };
    Gfx::IntRect scissor_box;
    bool enable_color_write { true };
    float depth_offset_factor { 0 };
    float depth_offset_constant { 0 };
    bool depth_offset_enabled { false };
    bool enable_culling { false };
    GPU::WindingOrder front_face { GPU::WindingOrder::CounterClockwise };
    bool cull_back { true };
    bool cull_front { false };
    Array<u8, NUM_SAMPLERS> texcoord_generation_enabled_coordinates {};
    Array<Array<TexCoordGenerationConfig, 4>, NUM_SAMPLERS> texcoord_generation_config {};
    Gfx::IntRect viewport;
    bool lighting_enabled { false };
    bool color_material_enabled { false };
    GPU::ColorMaterialFace color_material_face { GPU::ColorMaterialFace::FrontAndBack };
    GPU::ColorMaterialMode color_material_mode { GPU::ColorMaterialMode::AmbientAndDiffuse };
};

struct LightModelParameters {
    FloatVector4 scene_ambient_color { 0.2f, 0.2f, 0.2f, 1.0f };
    bool viewer_at_infinity { false };
    GPU::ColorControl color_control { GPU::ColorControl::SingleColor };
    bool two_sided_lighting { false };
};

struct PixelQuad;

struct RasterPosition {
    FloatVector4 window_coordinates { 0.0f, 0.0f, 0.0f, 1.0f };
    float eye_coordinate_distance { 0.0f };
    bool valid { true };
    FloatVector4 color_rgba { 1.0f, 1.0f, 1.0f, 1.0f };
    float color_index { 1.0f };
    FloatVector4 texture_coordinates { 0.0f, 0.0f, 0.0f, 1.0f };
};

class Device final {
public:
    Device(Gfx::IntSize const& min_size);

    GPU::DeviceInfo info() const;

    void draw_primitives(GPU::PrimitiveType, FloatMatrix4x4 const& model_view_transform, FloatMatrix4x4 const& projection_transform, FloatMatrix4x4 const& texture_transform, Vector<Vertex> const& vertices, Vector<size_t> const& enabled_texture_units);
    void resize(Gfx::IntSize const& min_size);
    void clear_color(FloatVector4 const&);
    void clear_depth(GPU::DepthType);
    void clear_stencil(GPU::StencilType);
    void blit_color_buffer_to(Gfx::Bitmap& target);
    void blit_to_color_buffer_at_raster_position(Gfx::Bitmap const&);
    void blit_to_depth_buffer_at_raster_position(Vector<GPU::DepthType> const&, int, int);
    void set_options(RasterizerOptions const&);
    void set_light_model_params(LightModelParameters const&);
    RasterizerOptions options() const { return m_options; }
    LightModelParameters light_model() const { return m_lighting_model; }
    GPU::ColorType get_color_buffer_pixel(int x, int y);
    GPU::DepthType get_depthbuffer_value(int x, int y);

    NonnullRefPtr<Image> create_image(GPU::ImageFormat format, unsigned width, unsigned height, unsigned depth, unsigned levels, unsigned layers);

    void set_sampler_config(unsigned, GPU::SamplerConfig const&);
    void set_light_state(unsigned, GPU::Light const&);
    void set_material_state(GPU::Face, GPU::Material const&);
    void set_stencil_configuration(GPU::Face, GPU::StencilConfiguration const&);

    RasterPosition raster_position() const { return m_raster_position; }
    void set_raster_position(RasterPosition const& raster_position);
    void set_raster_position(FloatVector4 const& position, FloatMatrix4x4 const& model_view_transform, FloatMatrix4x4 const& projection_transform);

private:
    void draw_statistics_overlay(Gfx::Bitmap&);
    Gfx::IntRect get_rasterization_rect_of_size(Gfx::IntSize size);

    void rasterize_triangle(Triangle const& triangle);
    void setup_blend_factors();
    void shade_fragments(PixelQuad&);
    bool test_alpha(PixelQuad&);

    RefPtr<FrameBuffer<GPU::ColorType, GPU::DepthType, GPU::StencilType>> m_frame_buffer {};
    RasterizerOptions m_options;
    LightModelParameters m_lighting_model;
    Clipper m_clipper;
    Vector<Triangle> m_triangle_list;
    Vector<Triangle> m_processed_triangles;
    Vector<Vertex> m_clipped_vertices;
    Array<Sampler, NUM_SAMPLERS> m_samplers;
    Vector<size_t> m_enabled_texture_units;
    AlphaBlendFactors m_alpha_blend_factors;
    Array<GPU::Light, NUM_LIGHTS> m_lights;
    Array<GPU::Material, 2u> m_materials;
    RasterPosition m_raster_position;
    Array<GPU::StencilConfiguration, 2u> m_stencil_configuration;
};

}
