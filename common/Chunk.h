#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <common/model.h>
#include <common/texture.h>

using namespace std;
using namespace glm;

class Chunk {
public:
	int chunkX;
	int chunkZ;
	int array[16][16][256];
	Chunk(int chunkX, int chunkZ);
	void removeInnerCubes();
	void generateSea();
	void draw(GLuint modelMatrixLocation, GLuint useTextureLocation, Drawable* model, GLuint grassTexture, GLuint snowTexture, GLuint dirtTexture, GLuint stoneTexture, GLuint waterTexture);
};

#endif