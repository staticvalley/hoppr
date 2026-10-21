#include <Material.hpp>
#include <Log.hpp>

namespace {
	Texture* missingTexture() {
		// purple black checkboard style
		static const uint8_t pixels[] = {
			255, 0, 255, 255,   0, 0, 0, 255,
			  0, 0,   0, 255, 255, 0, 255, 255,
		};
		static Texture* texture = new Texture(pixels, 2, 2, GL_RGBA);
		return texture;
	}
}

void Material::apply(Shader& shader) {
	if (diffuseTexture) {
		diffuseTexture->bind(0);
	} else {
		missingTexture()->bind(0);
	}
	shader.setUniformInt1("u_texture", 0);
}