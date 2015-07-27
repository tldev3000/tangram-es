#pragma once

#include "style.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "gl/typedMesh.h"
#include "labels/labelMesh.h"
#include "labels/label.h"
#include "labels/spriteAtlas.h"

namespace Tangram {

class Texture;

class SpriteStyle : public Style {

protected:

    virtual void constructVertexLayout() override;
    virtual void constructShaderProgram() override;
    virtual void buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, VboMesh& _mesh, Tile& _tile) const override;

    virtual VboMesh* newMesh() const override {
        return new LabelMesh(m_vertexLayout, m_drawMode);
    };

    std::unique_ptr<SpriteAtlas> m_spriteAtlas;

public:

    bool isOpaque() const override { return false; }

    virtual void onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) override;

    SpriteStyle(std::string _name, GLenum _drawMode = GL_TRIANGLES);

    virtual ~SpriteStyle();

};

}
