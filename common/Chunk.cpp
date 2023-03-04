#include <GL/glew.h>
#include <glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "./Chunk.h"
#include <common/NoiseGenerator.h>

using namespace glm;
using namespace std;

Chunk::Chunk(int chunkX, int chunkZ) {
	this->chunkX = chunkX;
	this->chunkZ = chunkZ;

	NoiseGenerator ng = NoiseGenerator(1000);
	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			double h = ng.getHeight(x, z, chunkX, chunkZ);
			for (int y = 0; y < 256;y++) {
				if (y < h) {
					array[x][z][y] = 1;						// 1 --> ground
				}
				else {
					array[x][z][y] = 0;						// 0 --> air
				}
			}
		}
	}
};

void Chunk::removeInnerCubes() {
	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			for (int y = 0; y < 256; y++) {
				if (array[x][z][y + 1] == 1) {//if internal cube
					array[x][z][y] = 0;//put air (delete it)
				}
			}
		}
	}
}

void Chunk::generateSea() {
	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			for (int y = 30; y < 44; y++) {
				if (array[x][z][y] == 0) {
					array[x][z][y] = 1;
				}
			}
		}
	}
}

void Chunk::draw(GLuint modelMatrixLocation, GLuint useTextureLocation, Drawable* model, GLuint grassTexture, GLuint snowTexture, GLuint dirtTexture, GLuint stoneTexture, GLuint waterTexture) {
	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			for (int y = 0; y < 256; y++) {
				if (this->array[x][z][y] == 1) {
					vec3 trans = vec3(x + 16 * this->chunkX, y, z + 16 * this->chunkZ);
					mat4 modelMatrix2 = translate(mat4(), trans);
					glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &modelMatrix2[0][0]);

					glActiveTexture(GL_TEXTURE1);
					if (y > 90) {
						glBindTexture(GL_TEXTURE_2D, snowTexture);
					}
					else if (y > 80 && y <= 90) {
						glBindTexture(GL_TEXTURE_2D, stoneTexture);
					}
					else if (y > 45 && y <= 80) {
						glBindTexture(GL_TEXTURE_2D, grassTexture);
					}
					else if (y >= 30 && y <= 43) {
						glBindTexture(GL_TEXTURE_2D, waterTexture);
					}
					else {
						glBindTexture(GL_TEXTURE_2D, dirtTexture);
					}
					glUniform1i(useTextureLocation, 1);


					model->draw();
				}
			}
		}
	}
}
