
#define STB_IMAGE_IMPLEMENTATION

#include "../libs/stb/stb_image.h"
#include <Texture.hpp>
#include <SDL3/SDL.h>
#include <Log.hpp>

Texture::Texture(const char* filePath) {

	// flip image so y=0 is bottom of image
	stbi_set_flip_vertically_on_load(true);

	// read image
	uint8_t* textureData = (uint8_t*)stbi_load(filePath, &width, &height, &channels, 0);
	if (textureData)
		generateTexture(textureData, width, height, channels == 4 ? GL_RGBA : GL_RGB);
	else
		Log::error(Log::Channel::RENDERER, "Failed to load texture file \"{}\"", filePath);

	stbi_image_free(textureData);
}

Texture::Texture(const uint8_t* data, const int width, const int height, const GLenum format) {
	generateTexture(data, width, height, format);
}


Texture::~Texture() {
	glDeleteTextures(1, &id);
}

void Texture::bind(GLuint slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::unbind() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::generateTexture(const uint8_t* data, const int width, const int height, const GLenum format) {
	glGenTextures(1, &id); 
	glBindTexture(GL_TEXTURE_2D, id);

	// uv sampling wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// scaled sampling parameters
	const GLenum minFilter = format == GL_RGB ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST;
	const GLenum magFilter = format == GL_RGB ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		format,
		width,
		height,
		0,
		format,
		GL_UNSIGNED_BYTE,
		data
	);

	if (format == GL_RGB) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}