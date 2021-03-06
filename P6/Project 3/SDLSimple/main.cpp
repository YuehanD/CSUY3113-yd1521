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


#include "Entity.h"
#include "Map.h"
#include "Util.h"


#include "Scene.h"
#include "Start.h"
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"
#include "End.h"

#include <vector>




SDL_Window* displayWindow;
bool gameIsRunning = true;

GLuint fontTextureID;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;



Scene *currentScene;
Scene *sceneList[5];

bool start = false;
bool win = false;
bool fail = false;

Mix_Music *music;
Mix_Chunk *lose;
Mix_Chunk *new_game;

void SwitchToScene(Scene *scene) {
	currentScene = scene;
	currentScene->Initialize();
}


void Initialize() {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	displayWindow = SDL_CreateWindow("Maze Traveller!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	music = Mix_LoadMUS("bgm.mp3");
	Mix_PlayMusic(music, -1);
	//Mix_VolumnMusic(MIX_MAX_VOLUMN/4);
	//audio

	lose = Mix_LoadWAV("lose.wav");
	new_game = Mix_LoadWAV("start.wav");
	//Mix_PlayChannel(-1,lose,0);

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

	glClearColor(0.2f, 0.2f, 0.2f, 0.13f);
	glEnable(GL_BLEND);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	sceneList[0] = new Start();
	sceneList[1] = new Level1();
	sceneList[2] = new Level2();
	sceneList[3] = new Level3();
	sceneList[4] = new End();
	SwitchToScene(sceneList[0]);

	fontTextureID = Util::LoadTexture("font1.png");
}


void ProcessInput() {

	const Uint8 *keys = SDL_GetKeyboardState(NULL);


	currentScene->state.player->movement = glm::vec3(0);

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
		case SDL_WINDOWEVENT_CLOSE:
			gameIsRunning = false;
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_RETURN:
				if (currentScene==sceneList[0]) {
					Mix_PlayChannel(-1, new_game, 0);
					SwitchToScene(sceneList[1]);
					start = true;
					fail = false;
				}
			case SDLK_LEFT:
				// Move the player left
				break;

			case SDLK_RIGHT:
				// Move the player right
				break;

			/*case SDLK_SPACE:
				if (currentScene->state.player->isJump == true)break;
				currentScene->state.player->jump = true;
				break;*/
			}
			break; // SDL_KEYDOWN
		}
	}

	

	if (keys[SDL_SCANCODE_LEFT]) {
		currentScene->state.player->movement.x = -1.0f;
		currentScene->state.player->animIndices = currentScene->state.player->animLeft;
	}
	else if (keys[SDL_SCANCODE_RIGHT]) {
		currentScene->state.player->movement.x = 1.0f;
		currentScene->state.player->animIndices = currentScene->state.player->animRight;
	}
	else if (keys[SDL_SCANCODE_UP]) {
		currentScene->state.player->movement.y = 1.0f;
		currentScene->state.player->animIndices = currentScene->state.player->animUp;
	}
	else if (keys[SDL_SCANCODE_DOWN]) {
		currentScene->state.player->movement.y = -1.0f;
		currentScene->state.player->animIndices = currentScene->state.player->animDown;
	}



	if (glm::length(currentScene->state.player->movement) > 1.0f) {
		currentScene->state.player->movement = glm::normalize(currentScene->state.player->movement);
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
		currentScene->Update(FIXED_TIMESTEP);
		/*
		for (int i = 0; i < ENEMY_COUNT; i++) {
			state.enemies[i].Update(FIXED_TIMESTEP,state.player,0, state.map);
		}*/

		deltaTime -= FIXED_TIMESTEP;
	}

	accumulator = deltaTime;

	viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::scale(viewMatrix, glm::vec3(1.0f, 1.0f, 1));

	viewMatrix = glm::translate(viewMatrix,
		glm::vec3(-currentScene->state.player->position.x, -currentScene->state.player->position.y, 0));

	/*
	if (currentScene->state.player->position.x > 5) {
		viewMatrix = glm::translate(viewMatrix,
			glm::vec3(-currentScene->state.player->position.x, -currentScene->state.player->position.y, 0));
	}
	else {
		viewMatrix = glm::translate(viewMatrix, glm::vec3(-5, 3.75, 0));
	}*/

	if (currentScene->state.player->fail == true) {
		SwitchToScene(sceneList[0]);
		Mix_PlayChannel(-1, lose, 0);
		fail = true;
	}
}


void Render() {

	glClear(GL_COLOR_BUFFER_BIT);


	currentScene->Render(&program);
	program.SetViewMatrix(viewMatrix);


	if (!start) {
		Util::DrawText(&program, fontTextureID, "MAZE TRAVELLER", 1.0f, -0.49, glm::vec3(-3.4, 0.5, 0));
		Util::DrawText(&program, fontTextureID, "PRESS ENTER TO START!", 0.8f, -0.5f, glm::vec3(-2.75, -0.8, 0));
	}
	else if (fail) {
		Util::DrawText(&program, fontTextureID, "GAME OVER!", 1.0f, -0.49, glm::vec3(-2.3, 0.5, 0));
		Util::DrawText(&program, fontTextureID, "PRESS ENTER TO START!", 0.8f, -0.5f, glm::vec3(-2.75, -0.8, 0));
	}
	else if (win) {
		Util::DrawText(&program, fontTextureID, "YOU WIN!", 1.0f, -0.49, glm::vec3(-2.0, 0.0, 0));
	}

	

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

		if (currentScene->state.nextScene >= 0) {
			Mix_PlayChannel(-1, new_game, 0);
			if (currentScene->state.nextScene == 4) {
				win = true;
			}
			SwitchToScene(sceneList[currentScene->state.nextScene]);
		}

		Render();
	}

	Shutdown();
	return 0;
}
