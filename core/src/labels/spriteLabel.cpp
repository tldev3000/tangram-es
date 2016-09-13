#include "labels/spriteLabel.h"

#include "gl/dynamicQuadMesh.h"
#include "style/pointStyle.h"
#include "view/view.h"
#include "platform.h"

namespace Tangram {

using namespace LabelProperty;

const float SpriteVertex::position_scale = 4.0f;
const float SpriteVertex::alpha_scale = 65535.0f;
const float SpriteVertex::texture_scale = 65535.0f;

SpriteLabel::SpriteLabel(Label::WorldTransform _transform, glm::vec2 _size, Label::Options _options,
                         float _extrudeScale, SpriteLabels& _labels, size_t _labelsPos)
    : Label(_transform, _size, Label::Type::point, _options),
      m_labels(_labels),
      m_labelsPos(_labelsPos),
      m_extrudeScale(_extrudeScale) {

    applyAnchor(m_options.anchors[0]);
}

void SpriteLabel::applyAnchor(LabelProperty::Anchor _anchor) {

    m_anchor = LabelProperty::anchorDirection(_anchor) * m_dim * 0.5f;
}

bool SpriteLabel::updateScreenTransform(const glm::mat4& _mvp, const ViewState& _viewState, bool _drawAllLabels) {

    switch (m_type) {
        case Type::debug:
        case Type::point:
        {
            glm::vec2 p0 = glm::vec2(m_worldTransform.position);

            if (m_options.flat) {

                auto& positions = m_screenTransform.positions;
                float sourceScale = pow(2, m_worldTransform.position.z);
                float scale = float(sourceScale / (_viewState.zoomScale * _viewState.tileSize * 2.0));
                if (m_extrudeScale != 1.f) {
                    scale *= pow(2, _viewState.fractZoom) * m_extrudeScale;
                }
                glm::vec2 dim = m_dim * scale;


                positions[0] = p0 - dim;
                positions[1] = p0 + glm::vec2(dim.x, -dim.y);
                positions[2] = p0 + glm::vec2(-dim.x, dim.y);
                positions[3] = p0 + dim;

                // Rotate in clockwise order on the ground plane
                if (m_options.angle != 0.f) {
                    glm::vec2 rotation(cos(DEG_TO_RAD * m_options.angle),
                                       sin(DEG_TO_RAD * m_options.angle));

                    positions[0] = rotateBy(positions[0], rotation);
                    positions[1] = rotateBy(positions[1], rotation);
                    positions[2] = rotateBy(positions[2], rotation);
                    positions[3] = rotateBy(positions[3], rotation);
                }

                for (size_t i = 0; i < 4; i++) {
                    m_projected[i] = worldToClipSpace(_mvp, glm::vec4(positions[i], 0.f, 1.f));
                    if (m_projected[i].w <= 0.0f) { return false; }

                    positions[i] = clipToScreenSpace(m_projected[i], _viewState.viewportSize);
                }

            } else {
                m_projected[0] = worldToClipSpace(_mvp, glm::vec4(p0, 0.f, 1.f));
                if (m_projected[0].w <= 0.0f) { return false; }

                glm::vec2 position = clipToScreenSpace(m_projected[0], _viewState.viewportSize);

                m_screenTransform.position = position + m_options.offset;

                m_viewportSize = _viewState.viewportSize;

            }

            break;
        }
        default:
            break;
    }

    return true;
}

void SpriteLabel::updateBBoxes(float _zoomFract) {
    glm::vec2 dim;

    if (m_options.flat) {
        static float infinity = std::numeric_limits<float>::infinity();

        float minx = infinity, miny = infinity;
        float maxx = -infinity, maxy = -infinity;

        for (int i = 0; i < 4; ++i) {
            minx = std::min(minx, m_screenTransform.positions[i].x);
            miny = std::min(miny, m_screenTransform.positions[i].y);
            maxx = std::max(maxx, m_screenTransform.positions[i].x);
            maxy = std::max(maxy, m_screenTransform.positions[i].y);
        }

        dim = glm::vec2(maxx - minx, maxy - miny);

        if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

        // TODO: Manage extrude scale

        glm::vec2 obbCenter = glm::vec2((minx + maxx) * 0.5f, (miny + maxy) * 0.5f);

        m_obb = OBB(obbCenter, glm::vec2(1.0, 0.0), dim.x, dim.y);
    } else {
        dim = m_dim + glm::vec2(m_extrudeScale * 2.f * _zoomFract);

        if (m_occludedLastFrame) { dim += Label::activation_distance_threshold; }

        m_obb = OBB(m_screenTransform.position, m_screenTransform.rotation, dim.x, dim.y);
    }
}

void SpriteLabel::addVerticesToMesh() {

    if (!visibleState()) { return; }

    // TODO
    // if (a_extrude.x != 0.0) {
    //     float dz = u_map_position.z - abs(u_tile_origin.z);
    //     vertex_pos.xy += clamp(dz, 0.0, 1.0) * UNPACK_EXTRUDE(a_extrude.xy);
    // }

    auto& quad = m_labels.quads[m_labelsPos];

    SpriteVertex::State state {
        quad.color,
        uint16_t(m_screenTransform.alpha * SpriteVertex::alpha_scale),
        0,
    };

    auto& style = m_labels.m_style;

    auto* quadVertices = style.getMesh()->pushQuad();


    if (m_options.flat) {
        for (int i = 0; i < 4; i++) {
            SpriteVertex& vertex = quadVertices[i];

            vertex.pos = m_projected[i];
            vertex.uv = quad.quad[i].uv;
            vertex.state = state;
        }

    } else {

        glm::vec2 scale = 2.0f / m_viewportSize;
        scale.y *= -1;

        glm::vec2 pos = glm::vec2(m_projected[0]) / m_projected[0].w;

        for (int i = 0; i < 4; i++) {
            SpriteVertex& vertex = quadVertices[i];

            vertex.pos = glm::vec4(pos + (glm::vec2(quad.quad[i].pos) / SpriteVertex::position_scale) * scale , 0.f, 1.f);

            vertex.uv = quad.quad[i].uv;
            vertex.state = state;
        }
    }
}

}
