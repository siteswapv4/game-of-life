#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define GOL_WINDOW_NAME "Game Of Life"

int GOL_WINDOW_WIDTH = 960;
int GOL_WINDOW_HEIGHT = 544;

int GOL_FIELD_WIDTH = 960;
int GOL_FIELD_HEIGHT = 544;


typedef enum
{
	GOL_STATE_INVALID = -1,
	GOL_STATE_PAUSE,
	GOL_STATE_PLAY,
	GOL_STATE_MAX
}GOL_State;


typedef struct
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* fieldTexture;
	SDL_Gamepad* gamepad;
	bool* fieldArray;
	bool* tempFieldArray;
	Uint8* neighborArray;
	Uint8* tempNeighborArray;
	SDL_FPoint* blackPoints;
	SDL_FPoint* whitePoints;
	int whiteLen;
	int blackLen;
	const bool* keyStates;
	SDL_Point pos;
	GOL_State state;
}GOL_Environment;


void GOL_AddNeighbor(GOL_Environment* environment, int x, int y, int add)
{
	int directions[4] = {x - 1, x + 1, y - 1, y + 1};
	
	if (x == 0)
	{
		directions[0] = GOL_FIELD_WIDTH - 1;
	}
	else if (x == GOL_FIELD_WIDTH - 1)
	{
		directions[1] = 0;
	}
	
	if (y == 0)
	{
		directions[2] = GOL_FIELD_HEIGHT - 1;
	}
	else if (y == GOL_FIELD_HEIGHT - 1)
	{
		directions[3] = 0;
	}
	
	environment->neighborArray[directions[0] + y * GOL_FIELD_WIDTH] += add;
	environment->neighborArray[directions[1] + y * GOL_FIELD_WIDTH] += add;
	environment->neighborArray[x + directions[2] * GOL_FIELD_WIDTH] += add;
	environment->neighborArray[x + directions[3] * GOL_FIELD_WIDTH] += add;
	
	environment->neighborArray[directions[0] + directions[2] * GOL_FIELD_WIDTH] += add;
	environment->neighborArray[directions[1] + directions[2] * GOL_FIELD_WIDTH] += add;
	environment->neighborArray[directions[0] + directions[3] * GOL_FIELD_WIDTH] += add;
	environment->neighborArray[directions[1] + directions[3] * GOL_FIELD_WIDTH] += add;
}


void GOL_FlipPoint(GOL_Environment* environment, int x, int y, bool draw)
{
	int add = 0;
	int values[2] = {-1, 1};
	
	if ((x < 0) || (x >= GOL_FIELD_WIDTH) || (y < 0) || (y >= GOL_FIELD_HEIGHT))
	{
		return;
	}
	
	bool* fieldPoint = &environment->fieldArray[x + y * GOL_FIELD_WIDTH];

	*fieldPoint = !(*fieldPoint);
		
	add += values[*fieldPoint];

	GOL_AddNeighbor(environment, x, y, add);

	if (draw)
	{
		if (*fieldPoint)
		{
			SDL_SetRenderDrawColor(environment->renderer, 0, 0, 0, 255);
		}
		else
		{
			SDL_SetRenderDrawColor(environment->renderer, 255, 255, 255, 255);
		}
		
		SDL_RenderPoint(environment->renderer, x, y);
	}
}


void GOL_UpdateField(GOL_Environment* environment)
{
	SDL_memcpy(environment->tempFieldArray, environment->fieldArray, GOL_FIELD_WIDTH * GOL_FIELD_HEIGHT * sizeof(bool));
	SDL_memcpy(environment->tempNeighborArray, environment->neighborArray, GOL_FIELD_WIDTH * GOL_FIELD_HEIGHT * sizeof(Uint8));
	
	environment->blackLen = 0;
	environment->whiteLen = 0;
	
	for (int j = 0; j < GOL_FIELD_HEIGHT; j++)
	{
		for (int i = 0; i < GOL_FIELD_WIDTH; i++)
		{
			int index = i + j * GOL_FIELD_WIDTH;

			if (environment->tempFieldArray[index])
			{
				if ((environment->tempNeighborArray[index] < 2) || (environment->tempNeighborArray[index] > 3))
				{
					GOL_FlipPoint(environment, i, j, false);
					
					environment->whitePoints[environment->whiteLen].x = i;
					environment->whitePoints[environment->whiteLen].y = j;
					environment->whiteLen++;
				}
			}
			else
			{
				if (environment->tempNeighborArray[index] == 3)
				{
					GOL_FlipPoint(environment, i, j, false);
					
					environment->blackPoints[environment->blackLen].x = i;
					environment->blackPoints[environment->blackLen].y = j;
					environment->blackLen++;
				}
			}
		}
	}
	
	SDL_SetRenderDrawColor(environment->renderer, 0, 0, 0, 255);
	SDL_RenderPoints(environment->renderer, environment->blackPoints, environment->blackLen);
	
	SDL_SetRenderDrawColor(environment->renderer, 255, 255, 255, 255);
	SDL_RenderPoints(environment->renderer, environment->whitePoints, environment->whiteLen);
}


SDL_AppResult SDL_AppInit(void** data, int argc, char* argv[])
{
	GOL_Environment* environment = SDL_calloc(1, sizeof(GOL_Environment));
	*data = environment;

	SDL_DisplayID* displays;
	const SDL_DisplayMode* displayMode;
	
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		goto error;
	}
	
	displays = SDL_GetDisplays(NULL);
	
	if (displays[0] != 0)
	{
		displayMode = SDL_GetCurrentDisplayMode(displays[0]);
		
		GOL_WINDOW_WIDTH = GOL_FIELD_WIDTH = displayMode->w;
		GOL_WINDOW_HEIGHT = GOL_FIELD_HEIGHT = displayMode->h;
	}
	
	SDL_free(displays);
	
	if (!SDL_CreateWindowAndRenderer(GOL_WINDOW_NAME, GOL_WINDOW_WIDTH, GOL_WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN, &environment->window, &environment->renderer))
	{
		goto error;
	}
	
	if ((environment->fieldTexture = SDL_CreateTexture(environment->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, GOL_FIELD_WIDTH, GOL_FIELD_HEIGHT)) == NULL)
	{
		goto error;
	}

	SDL_SetTextureScaleMode(environment->fieldTexture, SDL_SCALEMODE_NEAREST);
	
	environment->fieldArray = SDL_calloc(GOL_FIELD_WIDTH * GOL_FIELD_HEIGHT, sizeof(bool));
	environment->tempFieldArray = SDL_calloc(GOL_FIELD_WIDTH * GOL_FIELD_HEIGHT, sizeof(bool));
	environment->neighborArray = SDL_calloc(GOL_FIELD_WIDTH * GOL_FIELD_HEIGHT, sizeof(Uint8));
	environment->tempNeighborArray = SDL_calloc(GOL_FIELD_WIDTH * GOL_FIELD_HEIGHT, sizeof(Uint8));
	environment->blackPoints = SDL_malloc(GOL_FIELD_WIDTH * GOL_FIELD_HEIGHT * sizeof(SDL_FPoint));
	environment->whitePoints = SDL_malloc(GOL_FIELD_WIDTH * GOL_FIELD_HEIGHT * sizeof(SDL_FPoint));
	
	SDL_SetRenderVSync(environment->renderer, 1);
	SDL_SetRenderLogicalPresentation(environment->renderer, GOL_FIELD_WIDTH, GOL_FIELD_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	
	SDL_SetRenderTarget(environment->renderer, environment->fieldTexture);
	SDL_SetRenderDrawColor(environment->renderer, 255, 255, 255, 255);
	SDL_RenderFillRect(environment->renderer, NULL);
	
	environment->keyStates = SDL_GetKeyboardState(NULL);
	
	environment->state = GOL_STATE_PAUSE;
	
	SDL_srand(0);

	for (int j = 0; j < GOL_FIELD_HEIGHT; j++)
	{
		for (int i = 0; i < GOL_FIELD_WIDTH; i++)
		{

			if (SDL_rand(2))
			{
				GOL_FlipPoint(environment, i, j, false);
				environment->blackPoints[environment->blackLen].x = i;
				environment->blackPoints[environment->blackLen].y = j;
				environment->blackLen++;
			}
		}
	}

	SDL_SetRenderDrawColor(environment->renderer, 0, 0, 0, 255);
	SDL_RenderPoints(environment->renderer, environment->blackPoints, environment->blackLen);
	environment->blackLen = 0;
	
	return SDL_APP_CONTINUE;
	
error:
	SDL_Log("%s", SDL_GetError());
	
	return SDL_APP_FAILURE;
}


SDL_AppResult SDL_AppEvent(void* data, SDL_Event* event)
{
	GOL_Environment* environment = data;
	
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}
	else if ((event->type == SDL_EVENT_KEY_DOWN) && (!event->key.repeat))
	{
		if (event->key.scancode == SDL_SCANCODE_SPACE)
		{
			if (environment->state == GOL_STATE_PAUSE)
			{
				environment->state = GOL_STATE_PLAY;
			}
			else
			{
				environment->state = GOL_STATE_PAUSE;
			}
		}
	}
	else if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
	{
		if (event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK)
		{
			if (environment->state == GOL_STATE_PAUSE)
			{
				environment->state = GOL_STATE_PLAY;
			}
			else
			{
				environment->state = GOL_STATE_PAUSE;
			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		environment->state = !environment->state;
	}
	else if (event->type == SDL_EVENT_GAMEPAD_ADDED)
	{
		if (!environment->gamepad)
		{
			environment->gamepad = SDL_OpenGamepad(event->gdevice.which);
		}
	}
	else if (event->type == SDL_EVENT_GAMEPAD_REMOVED)
	{
		if ((environment->gamepad) && (SDL_GetGamepadID(environment->gamepad) == event->gdevice.which))
		{
			SDL_CloseGamepad(environment->gamepad);
			environment->gamepad = NULL;
		}
	}
	
	return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void* data)
{
	GOL_Environment* environment = data;
	
	if (environment->state == GOL_STATE_PAUSE)
	{
		if ((environment->keyStates[SDL_SCANCODE_LEFT]) || (SDL_GetGamepadButton(environment->gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)))
		{
			environment->pos.x -= 1;
		}
		if ((environment->keyStates[SDL_SCANCODE_RIGHT]) || (SDL_GetGamepadButton(environment->gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)))
		{
			environment->pos.x += 1;
		}
		if ((environment->keyStates[SDL_SCANCODE_UP]) || (SDL_GetGamepadButton(environment->gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)))
		{
			environment->pos.y -= 1;
		}
		if ((environment->keyStates[SDL_SCANCODE_DOWN]) || (SDL_GetGamepadButton(environment->gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)))
		{
			environment->pos.y += 1;
		}
		if ((environment->keyStates[SDL_SCANCODE_RETURN]) || (SDL_GetGamepadButton(environment->gamepad, SDL_GAMEPAD_BUTTON_START)))
		{
			GOL_FlipPoint(environment, environment->pos.x, environment->pos.y, true);
		}
	}
	else if (environment->state == GOL_STATE_PLAY)
	{
		GOL_UpdateField(environment);
	}
	
	SDL_SetRenderTarget(environment->renderer, NULL);
	
	SDL_RenderClear(environment->renderer);
	
	SDL_RenderTexture(environment->renderer, environment->fieldTexture, NULL, NULL);
	
	SDL_RenderPresent(environment->renderer);
	
	SDL_SetRenderTarget(environment->renderer, environment->fieldTexture);
	
	return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void* data, SDL_AppResult result)
{
	GOL_Environment* environment = data;
	
	SDL_free(environment->tempFieldArray);
	SDL_free(environment->neighborArray);
	SDL_free(environment->tempNeighborArray);
	SDL_free(environment->fieldArray);
	SDL_free(environment->blackPoints);
	SDL_free(environment->whitePoints);
	SDL_free(environment);
}
