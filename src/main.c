#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>

#include "app.h"

/* Injected by the Makefile from config.mk; fallbacks keep other tools happy. */
#ifndef PROJECT_NAME
#define PROJECT_NAME "SDL3-Template"
#endif
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "0.1.0"
#endif

static void print_usage(const char *program) {
	printf("Usage: %s [options]\n"
		   "  --version    Print project and SDL versions and exit\n"
		   "  --help       Show this help message and exit\n"
		   "  --audio      Enable audio playback stream with 440 Hz test tone\n"
		   "  --camera     Enable camera capture device\n"
		   "  --all        Enable video, audio, and camera\n"
		   "  --no-video   Run without creating a window or renderer\n",
		   program != nullptr ? program : PROJECT_NAME);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
	AppConfig config = app_default_config();

	for(int i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if(strcmp(arg, "--version") == 0) {
			const int ver = SDL_GetVersion();
			printf("%s %s (SDL %d.%d.%d)\n", PROJECT_NAME, PROJECT_VERSION,
				   SDL_VERSIONNUM_MAJOR(ver), SDL_VERSIONNUM_MINOR(ver),
				   SDL_VERSIONNUM_MICRO(ver));
			return SDL_APP_SUCCESS;
		}
		if(strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
			print_usage(argc > 0 ? argv[0] : PROJECT_NAME);
			return SDL_APP_SUCCESS;
		}
		if(strcmp(arg, "--audio") == 0) {
			config.enable_audio = true;
			config.play_test_tone = true;
		} else if(strcmp(arg, "--camera") == 0) {
			config.enable_camera = true;
		} else if(strcmp(arg, "--all") == 0) {
			config.enable_video = true;
			config.enable_audio = true;
			config.enable_camera = true;
			config.play_test_tone = true;
		} else if(strcmp(arg, "--no-video") == 0) {
			config.enable_video = false;
		}
	}

	return app_init((AppState **)appstate, &config);
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	return app_event((AppState *)appstate, event);
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	return app_iterate((AppState *)appstate);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	app_quit((AppState *)appstate, result);
}
