#include <SDL3/SDL.h>
#include <stddef.h>
#include <string.h>

#include "app.h"
#include "test_framework.h"

static void enable_headless_drivers(void) {
	(void)SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
	(void)SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
	(void)SDL_SetHint(SDL_HINT_CAMERA_DRIVER, "dummy");
}

static void test_sdl_version_and_drivers(void) {
	const int ver = SDL_GetVersion();

	CHECK(ver >= SDL_VERSIONNUM(3, 4, 0));
	CHECK(SDL_GetNumVideoDrivers() > 0);
	CHECK(SDL_GetNumAudioDrivers() > 0);
	CHECK(SDL_GetNumCameraDrivers() > 0);
}

static void test_default_config(void) {
	const AppConfig cfg = app_default_config();

	CHECK(cfg.width > 0);
	CHECK(cfg.height > 0);
	CHECK(cfg.enable_video);
	CHECK(!cfg.enable_audio);
	CHECK(!cfg.enable_camera);
	CHECK(!cfg.play_test_tone);
}

static void test_screen_lifecycle(void) {
	enable_headless_drivers();

	AppConfig cfg = app_default_config();
	AppState *state = nullptr;

	CHECK_EQ_INT(SDL_APP_CONTINUE, app_init(&state, &cfg));
	CHECK(state != nullptr);
	if(state != nullptr) {
		CHECK(state->window != nullptr);
		CHECK(state->renderer != nullptr);
		CHECK((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0);
		CHECK_EQ_INT(SDL_APP_CONTINUE, app_iterate(state));
		CHECK_EQ_INT(1, (long long)state->frame_count);
		app_quit(state, SDL_APP_SUCCESS);
	}
}

static void test_audio_lifecycle(void) {
	enable_headless_drivers();

	AppConfig cfg = app_default_config();
	cfg.enable_video = false;
	cfg.enable_audio = true;
	cfg.play_test_tone = true;

	AppState *state = nullptr;
	CHECK_EQ_INT(SDL_APP_CONTINUE, app_init(&state, &cfg));
	CHECK(state != nullptr);
	if(state != nullptr) {
		CHECK(state->window == nullptr);
		CHECK(state->renderer == nullptr);
		CHECK(state->audio_stream != nullptr);
		CHECK((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) != 0);
		CHECK_EQ_INT(SDL_APP_CONTINUE, app_iterate(state));
		if(state->audio_stream != nullptr) {
			CHECK(SDL_GetAudioStreamQueued(state->audio_stream) > 0);
		}
		app_quit(state, SDL_APP_SUCCESS);
	}
}

static void test_camera_lifecycle(void) {
	enable_headless_drivers();

	AppConfig cfg = app_default_config();
	cfg.enable_video = true;
	cfg.enable_camera = true;

	AppState *state = nullptr;
	CHECK_EQ_INT(SDL_APP_CONTINUE, app_init(&state, &cfg));
	CHECK(state != nullptr);
	if(state != nullptr) {
		int count = 0;
		SDL_CameraID *cameras = SDL_GetCameras(&count);

		CHECK((SDL_WasInit(SDL_INIT_CAMERA) & SDL_INIT_CAMERA) != 0);
		CHECK_EQ_STR("dummy", SDL_GetCurrentCameraDriver());
		CHECK(cameras != nullptr);
		if(count > 0) {
			CHECK(state->camera != nullptr);
		} else {
			CHECK(state->camera == nullptr);
		}
		SDL_free(cameras);
		CHECK_EQ_INT(SDL_APP_CONTINUE, app_iterate(state));
		app_quit(state, SDL_APP_SUCCESS);
	}
}

static void test_events_quit_and_escape(void) {
	AppState state;
	SDL_zero(state);

	SDL_Event ev;
	SDL_zero(ev);
	ev.type = SDL_EVENT_QUIT;
	CHECK_EQ_INT(SDL_APP_SUCCESS, app_event(&state, &ev));

	SDL_zero(ev);
	ev.type = SDL_EVENT_KEY_DOWN;
	ev.key.key = SDLK_ESCAPE;
	CHECK_EQ_INT(SDL_APP_SUCCESS, app_event(&state, &ev));

	SDL_zero(ev);
	ev.type = SDL_EVENT_KEY_DOWN;
	ev.key.key = SDLK_A;
	CHECK_EQ_INT(SDL_APP_CONTINUE, app_event(&state, &ev));
}

int main(void) {
	RUN_TEST(test_sdl_version_and_drivers);
	RUN_TEST(test_default_config);
	RUN_TEST(test_screen_lifecycle);
	RUN_TEST(test_audio_lifecycle);
	RUN_TEST(test_camera_lifecycle);
	RUN_TEST(test_events_quit_and_escape);
	return TEST_REPORT();
}
