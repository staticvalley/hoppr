#pragma once

#include <glad/glad.h>
#include <vector>

class Texture {
public:
	
	/**
	 * @brief constructs texture from raw bytes
	 * @param data raw pixel data
	 * @param width width of image
	 * @param height height of image
	 * @param format representation of image data (`GL_RGB` or `GL_RGBA`)
	 */
	Texture(const uint8_t* data, const int width, const int height, const GLenum format = GL_RGB);

	/**
	 * @brief constructs texture from file
	 * @param filePath full path to image file
	 */
	Texture(const char* filePath);


	Texture(const uint8_t* data, const uint8_t* palette, const int width, const int height, bool isTransparent);
	
	~Texture();

	// disallow copy
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	/**
	 * @brief binds texture as active
	 * @param slot texture slot to bind to
	 */
	void bind(GLuint slot);
	void unbind();

private:
	GLuint id;
	int width, height, channels;

	/**
	 * @brief converts unapplied palette data into colored RGB(A?) image buffer
	 * @param data image data with indices to palette
	 * @param palette color palette
	 * @param dataSize size of buffer (usually width * height)
	 * @param isTransparent is texture transparent
	 * @return colored image buffer
	 */
	std::vector<uint8_t> convertWithPalette(
		const uint8_t *data,
		const uint8_t *palette,
		size_t dataSize,
		bool isTransparent = false
	);

	/**
	 * @brief base texture constructor the public constructors call after formatting data
	 * @param data raw pixel data
	 * @param width width of image
	 * @param height height of image
	 * @param format representation of image data (`GL_RGB` or `GL_RGBA')
	 */
	void generateTexture(const uint8_t* data, const int width, const int height, const GLenum format);
};