#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Entity.h"

#include <vector>

#define PLATFORM_COUNT 15
#define ENEMY_COUNT 1

struct GameState {
	Entity *player;
	Entity *platforms;
	Entity *enemies;
};

GameState state;

SDL_Window* displayWindow;
bool gameIsRunning = true;

GLuint fontTextureID;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint LoadTexture(const char* filePath) {
	int w, h, n;
	unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return textureID;
}



void DrawText(ShaderProgram *program, GLuint fontTextureID, std::string text,
	float size, float spacing, glm::vec3 position)
{
	float width = 1.0f / 16.0f;
	float height = 1.0f / 16.0f;

	std::vector<float> vertices;
	std::vector<float> texCoords;

	for (unsigned int i = 0; i < text.size(); i++) {
		int index = (int)text[i];
		float offset = (size + spacing) * i;
		float u = (float)(index % 16) / 16.0f;
		float v = (float)(index / 16) / 16.0f;
		vertices.insert(vertices.end(), {
			 offset + (-0.5f * size), 0.5f * size,
			 offset + (-0.5f * size), -0.5f * size,
			 offset + (0.5f * size), 0.5f * size,
			 offset + (0.5f * size), -0.5f * size,
			 offset + (0.5f * size), 0.5f * size,
			 offset + (-0.5f * size), -0.5f * size,
			});

		texCoords.insert(texCoords.end(), {
			u, v,
			u, v + height,
			u + width, v,
			u + width, v + height,
			u + width, v,
			u, v + height,
			});
	} // end of for loop
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, position);
	program->SetModelMatrix(modelMatrix);

	glUseProgram(program->programID);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glBindTexture(GL_TEXTURE_2D, fontTextureID);
	glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}




void Initialize() {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	displayWindow = SDL_CreateWindow("Rise of the AI!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	//audio

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, 640, 480);

	program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");

	viewMatrix = glm::mat4(1.0f);
	modelMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);

	glUseProgram(program.programID);

	glClearColor(0.5f, 0.8f, 0.9f, 1.0f);
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	// Initialize Game Objects

	// Initialize Player
	state.player = new Entity();
	state.player->position = glm::vec3(-4, 2.0f, 0);
	state.player->movement = glm::vec3(0);
	state.player->acceleration = glm::vec3(0, -9.81f, 0);
	state.player->speed = 1.5f;
	state.player->textureID = LoadTexture("mdr.png");

	fontTextureID = LoadTexture("font1.png");

	state.player->animRight = new int[4]{ 8, 9, 10, 11 };
	state.player->animLeft = new int[4]{ 4, 5, 6, 7 };
	state.player->animUp = new int[4]{ 12, 13, 14, 15 };
	state.player->animDown = new int[4]{ 0, 1, 2, 3 };

	state.player->animIndices = state.player->animRight;
	state.player->animFrames = 4;
	state.player->animIndex = 0;
	state.player->animTime = 0;
	state.player->animCols = 4;
	state.player->animRows = 4;

	state.player->height = 0.8f;
	state.player->width = 0.5f;
	state.player->entityType = PLAYER;


	state.player->jumpPower = 6.5f;
	state.player->entityType = PLAYER;


	state.platforms = new Entity[PLATFORM_COUNT];
	
	GLuint platformTextureID = LoadTexture("ground.png");

	for (int i = 0; i < 11; i++) {
		state.platforms[i].textureID = platformTextureID;
		state.platforms[i].position = glm::vec3(-5+i, -3.25f, 0);
	}

	state.platforms[11].textureID = platformTextureID;
	state.platforms[11].position = glm::vec3(-2, -2.25, 0);
	state.platforms[12].textureID = platformTextureID;
	state.platforms[12].position = glm::vec3(-1, -2.25, 0);
	state.platforms[13].textureID = platformTextureID;
	state.platforms[13].position = glm::vec3(0, -2.25, 0);
	state.platforms[14].textureID = platformTextureID;
	state.platforms[14].position = glm::vec3(1, -2.25, 0);



	for (int i = 0; i < PLATFORM_COUNT; i++) {
		state.platforms[i].Update(0, NULL, 0);
		state.platforms[i].entityType = PLATFORM;
	}

	

	state.enemies = new Entity[ENEMY_COUNT];
	GLuint enemyTextureID = LoadTexture("ctg2.png");

	state.enemies[0].entityType = ENEMY;
	state.enemies[0].textureID = enemyTextureID;
	state.enemies[0].position = glm::vec3(3, -2.25f, 0);
	state.enemies[0].speed = 1;
	state.enemies[0].isActive = true;

	state.enemies[0].aiType = WALKER;

}

void ProcessInput() {

	state.player->movement = glm::vec3(0);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
		case SDL_WINDOWEVENT_CLOSE:
			gameIsRunning = false;
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_LEFT:
				// Move the player left
				break;

			case SDLK_RIGHT:
				// Move the player right
				break;

			case SDLK_SPACE:
				if (state.player->isJump == true)break;
				state.player->jump = true;
				break;
			}
			break; // SDL_KEYDOWN
		}
	}

	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	if (keys[SDL_SCANCODE_LEFT]) {
		state.player->movement.x = -1.0f;
		state.player->animIndices = state.player->animLeft;
	}
	else if (keys[SDL_SCANCODE_RIGHT]) {
		state.player->movement.x = 1.0f;
		state.player->animIndices = state.player->animRight;
	}
	/*else if (keys[SDL_SCANCODE_UP]) {
		state.player->movement.y = 1.0f;
		state.player->animIndices = state.player->animUp;
	}
	else if (keys[SDL_SCANCODE_DOWN]) {
		state.player->movement.y = -1.0f;
		state.player->animIndices = state.player->animDown;
	}*/



	if (glm::length(state.player->movement) > 1.0f) {
		state.player->movement = glm::normalize(state.player->movement);
	}

}

#define FIXED_TIMESTEP 0.0166666f
float lastTicks = 0;
float accumulator = 0.0f;
void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float deltaTime = ticks - lastTicks;
	lastTicks = ticks;

	deltaTime += accumulator;
	if (deltaTime < FIXED_TIMESTEP) {
		accumulator = deltaTime;
		return;
	}

	while (deltaTime >= FIXED_TIMESTEP) {
		// Update. Notice it's FIXED_TIMESTEP. Not deltaTime
		state.player->Update(FIXED_TIMESTEP, state.platforms, PLATFORM_COUNT);

		for (int i = 0; i < ENEMY_COUNT; i++) {
			state.enemies[i].Update(FIXED_TIMESTEP, state.platforms, PLATFORM_COUNT);
		}

		deltaTime -= FIXED_TIMESTEP;
	}

	accumulator = deltaTime;

	
}


void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < PLATFORM_COUNT; i++) {
		state.platforms[i].Render(&program);
	}

	for (int i = 0; i < ENEMY_COUNT; i++) {
		state.enemies[i].Render(&program);
	}


	state.player->Render(&program);

	SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
	SDL_Quit();
}

int main(int argc, char* argv[]) {
	Initialize();

	while (gameIsRunning) {
		ProcessInput();
		Update();
		Render();
	}

	Shutdown();
	return 0;
}
