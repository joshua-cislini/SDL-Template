/**
 * @file app.h
 * @brief Application lifecycle, subsystem configuration, and state management.
 *
 * Provides a ready-to-use SDL3 application shell covering video/screen,
 * 2D rendering, audio playback streaming, and camera capture.
 */
#ifndef APP_H
#define APP_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Startup configuration for the SDL3 application shell.
 */
typedef struct AppConfig {
	const char *title; /**< Window title (NULL uses PROJECT_NAME). */
	int width;		   /**< Initial window width in pixels. */
	int height;		   /**< Initial window height in pixels. */
	SDL_WindowFlags
		window_flags;  /**< Flags passed to SDL_CreateWindowAndRenderer. */
	bool enable_video; /**< Initialize video, window, and 2D renderer. */
	bool
		enable_audio; /**< Initialize audio and open default playback stream. */
	bool enable_camera;	 /**< Initialize camera and open first available device.
						  */
	bool play_test_tone; /**< Play a 440 Hz test tone when audio is active. */
} AppConfig;

/**
 * @brief Runtime state owned by the SDL3 application shell.
 */
typedef struct AppState {
	AppConfig config;
	SDL_InitFlags init_flags;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_AudioStream *audio_stream;
	SDL_Camera *camera;
	SDL_Texture *camera_texture;
	float audio_phase;
	bool play_test_tone;
	Uint64 start_ticks;
	Uint64 frame_count;
} AppState;

/**
 * @brief Returns default configuration (1280x720 resizable window, video on).
 */
AppConfig app_default_config(void);

/**
 * @brief Initializes SDL subsystems, devices, and application state.
 *
 * @param out_state Receives the newly allocated AppState on SDL_APP_CONTINUE.
 * @param config    Optional configuration; NULL uses app_default_config().
 * @return SDL_APP_CONTINUE on success, or SDL_APP_FAILURE on fatal error.
 */
SDL_AppResult app_init(AppState **out_state, const AppConfig *config);

/**
 * @brief Handles a single SDL event.
 *
 * @param state Active application state (may be NULL).
 * @param event Event delivered by SDL.
 * @return SDL_APP_CONTINUE to keep running, or SDL_APP_SUCCESS/FAILURE to exit.
 */
SDL_AppResult app_event(AppState *state, const SDL_Event *event);

/**
 * @brief Advances one frame: queues audio, acquires camera frames, and renders.
 *
 * @param state Active application state.
 * @return SDL_APP_CONTINUE to keep running, or SDL_APP_FAILURE on error.
 */
SDL_AppResult app_iterate(AppState *state);

/**
 * @brief Releases all SDL resources and frees @p state.
 *
 * Safe to call even if @p state is NULL or initialization failed partway.
 *
 * @param state  Application state allocated by app_init().
 * @param result Final application result code.
 */
void app_quit(AppState *state, SDL_AppResult result);

#ifdef __cplusplus
}
#endif

#endif /* APP_H */
