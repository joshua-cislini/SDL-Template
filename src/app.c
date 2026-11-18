#include "app.h"

#include <stddef.h>

/* Injected by the Makefile from config.mk; fallbacks keep other tools happy. */
#ifndef PROJECT_NAME
#define PROJECT_NAME "SDL3-Template"
#endif
#ifndef PROJECT_VERSION
#define PROJECT_VERSION "0.1.0"
#endif

enum {
	APP_DEFAULT_WIDTH = 1280,
	APP_DEFAULT_HEIGHT = 720,
	APP_AUDIO_SAMPLE_RATE = 48000,
	APP_AUDIO_CHANNELS = 2,
	APP_AUDIO_CHUNK_FRAMES = 512,
};

static const float APP_AUDIO_TONE_HZ = 440.0F;
static const float APP_AUDIO_TONE_AMPLITUDE = 0.12F;
static const float APP_HUD_SCALE = 2.0F;

AppConfig app_default_config(void) {
	const AppConfig cfg = {
		.title = PROJECT_NAME,
		.width = APP_DEFAULT_WIDTH,
		.height = APP_DEFAULT_HEIGHT,
		.window_flags = SDL_WINDOW_RESIZABLE,
		.enable_video = true,
		.enable_audio = false,
		.enable_camera = false,
		.play_test_tone = false,
	};
	return cfg;
}

static bool app_init_video(AppState *state) {
	if(!SDL_CreateWindowAndRenderer(
		   state->config.title, state->config.width, state->config.height,
		   state->config.window_flags, &state->window, &state->renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_VIDEO,
					 "Couldn't create window and renderer: %s", SDL_GetError());
		return false;
	}
	(void)SDL_SetRenderVSync(state->renderer, 1);
	return true;
}

static void app_init_audio(AppState *state) {
	const SDL_AudioSpec spec = {
		.format = SDL_AUDIO_F32,
		.channels = APP_AUDIO_CHANNELS,
		.freq = APP_AUDIO_SAMPLE_RATE,
	};

	state->audio_stream = SDL_OpenAudioDeviceStream(
		SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
	if(state->audio_stream == nullptr) {
		SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
					"Couldn't open default playback stream: %s",
					SDL_GetError());
		return;
	}
	(void)SDL_ResumeAudioStreamDevice(state->audio_stream);
}

static void app_init_camera(AppState *state) {
	int camera_count = 0;
	SDL_CameraID *cameras = SDL_GetCameras(&camera_count);

	if(cameras == nullptr) {
		SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Couldn't enumerate cameras: %s",
					SDL_GetError());
		return;
	}

	if(camera_count > 0) {
		state->camera = SDL_OpenCamera(cameras[0], nullptr);
		if(state->camera == nullptr) {
			SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Couldn't open camera: %s",
						SDL_GetError());
		}
	} else {
		SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO,
					"Camera subsystem enabled, no camera devices detected.");
	}

	SDL_free(cameras);
}

static void app_pump_audio(AppState *state) {
	if(state->audio_stream == nullptr || !state->play_test_tone) {
		return;
	}

	const int target_bytes =
		(APP_AUDIO_SAMPLE_RATE / 10) * APP_AUDIO_CHANNELS * (int)sizeof(float);
	const int queued = SDL_GetAudioStreamQueued(state->audio_stream);

	if(queued < 0 || queued >= target_bytes) {
		return;
	}

	float samples[APP_AUDIO_CHUNK_FRAMES * APP_AUDIO_CHANNELS];
	const float two_pi = 2.0F * SDL_PI_F;
	const float phase_step =
		(two_pi * APP_AUDIO_TONE_HZ) / (float)APP_AUDIO_SAMPLE_RATE;

	for(size_t i = 0; i < (size_t)APP_AUDIO_CHUNK_FRAMES; ++i) {
		const float sample =
			SDL_sinf(state->audio_phase) * APP_AUDIO_TONE_AMPLITUDE;
		samples[i * 2] = sample;
		samples[(i * 2) + 1] = sample;
		state->audio_phase += phase_step;
		if(state->audio_phase >= two_pi) {
			state->audio_phase -= two_pi;
		}
	}

	(void)SDL_PutAudioStreamData(state->audio_stream, samples,
								 (int)sizeof(samples));
}

static void app_update_camera(AppState *state) {
	if(state->camera == nullptr || state->renderer == nullptr) {
		return;
	}

	Uint64 timestamp = 0;
	SDL_Surface *frame = SDL_AcquireCameraFrame(state->camera, &timestamp);
	if(frame == nullptr) {
		return;
	}

	if(state->camera_texture == nullptr ||
	   state->camera_texture->w != frame->w ||
	   state->camera_texture->h != frame->h ||
	   state->camera_texture->format != frame->format) {
		if(state->camera_texture != nullptr) {
			SDL_DestroyTexture(state->camera_texture);
			state->camera_texture = nullptr;
		}
		state->camera_texture =
			SDL_CreateTexture(state->renderer, frame->format,
							  SDL_TEXTUREACCESS_STREAMING, frame->w, frame->h);
	}

	if(state->camera_texture != nullptr) {
		(void)SDL_UpdateTexture(state->camera_texture, nullptr, frame->pixels,
								frame->pitch);
	}

	SDL_ReleaseCameraFrame(state->camera, frame);
}

static const char *app_camera_status(const AppState *state) {
	if(!state->config.enable_camera) {
		return "disabled";
	}
	if(state->camera == nullptr) {
		return "enabled (no device)";
	}
	switch(SDL_GetCameraPermissionState(state->camera)) {
	case SDL_CAMERA_PERMISSION_STATE_APPROVED:
		return "active";
	case SDL_CAMERA_PERMISSION_STATE_DENIED:
		return "denied";
	case SDL_CAMERA_PERMISSION_STATE_PENDING:
	default:
		return "awaiting permission";
	}
}

static void app_render_camera(AppState *state) {
	if(state->camera_texture == nullptr) {
		return;
	}

	int out_w = 0;
	int out_h = 0;
	float tex_w = 0.0F;
	float tex_h = 0.0F;

	if(!SDL_GetRenderOutputSize(state->renderer, &out_w, &out_h) ||
	   !SDL_GetTextureSize(state->camera_texture, &tex_w, &tex_h) ||
	   out_w <= 0 || out_h <= 0 || tex_w <= 0.0F || tex_h <= 0.0F) {
		(void)SDL_RenderTexture(state->renderer, state->camera_texture, nullptr,
								nullptr);
		return;
	}

	const float scale_x = (float)out_w / tex_w;
	const float scale_y = (float)out_h / tex_h;
	const float fit_scale = (scale_x < scale_y) ? scale_x : scale_y;
	const float dst_w = tex_w * fit_scale;
	const float dst_h = tex_h * fit_scale;
	const SDL_FRect dst = {
		.x = ((float)out_w - dst_w) * 0.5F,
		.y = ((float)out_h - dst_h) * 0.5F,
		.w = dst_w,
		.h = dst_h,
	};

	(void)SDL_RenderTexture(state->renderer, state->camera_texture, nullptr,
							&dst);
}

static void app_render(AppState *state) {
	if(state->renderer == nullptr) {
		return;
	}

	(void)SDL_SetRenderDrawColor(state->renderer, 20, 24, 34, SDL_ALPHA_OPAQUE);
	(void)SDL_RenderClear(state->renderer);

	app_render_camera(state);

	const int sdl_ver = SDL_GetVersion();
	const char *video_driver = SDL_GetCurrentVideoDriver();
	const char *renderer_name = SDL_GetRendererName(state->renderer);
	const char *audio_driver = SDL_GetCurrentAudioDriver();
	const char *audio_status = "disabled";
	if(state->config.enable_audio) {
		if(state->audio_stream == nullptr) {
			audio_status = "enabled (no device)";
		} else if(state->play_test_tone) {
			audio_status = "active (440 Hz tone ON)";
		} else {
			audio_status = "active (tone muted)";
		}
	}

	(void)SDL_SetRenderScale(state->renderer, APP_HUD_SCALE, APP_HUD_SCALE);
	(void)SDL_SetRenderDrawColor(state->renderer, 235, 242, 255,
								 SDL_ALPHA_OPAQUE);

	const float line_x = 12.0F;
	const float line_step = 14.0F;
	float line_y = 12.0F;

	(void)SDL_RenderDebugTextFormat(
		state->renderer, line_x, line_y, "%s %s (SDL %d.%d.%d)",
		state->config.title, PROJECT_VERSION, SDL_VERSIONNUM_MAJOR(sdl_ver),
		SDL_VERSIONNUM_MINOR(sdl_ver), SDL_VERSIONNUM_MICRO(sdl_ver));
	line_y += line_step;

	(void)SDL_RenderDebugTextFormat(
		state->renderer, line_x, line_y, "Video: %s (%s)",
		video_driver != nullptr ? video_driver : "none",
		renderer_name != nullptr ? renderer_name : "unknown");
	line_y += line_step;

	(void)SDL_RenderDebugTextFormat(
		state->renderer, line_x, line_y, "Audio: %s [%s]", audio_status,
		audio_driver != nullptr ? audio_driver : "none");
	line_y += line_step;

	(void)SDL_RenderDebugTextFormat(state->renderer, line_x, line_y,
									"Camera: %s", app_camera_status(state));
	line_y += line_step;

	if(state->audio_stream != nullptr) {
		(void)SDL_RenderDebugText(
			state->renderer, line_x, line_y,
			"Controls: ESC Quit | SPACE Toggle Test Tone");
	} else {
		(void)SDL_RenderDebugText(state->renderer, line_x, line_y,
								  "Controls: ESC Quit");
	}

	(void)SDL_SetRenderScale(state->renderer, 1.0F, 1.0F);
	(void)SDL_RenderPresent(state->renderer);
}

SDL_AppResult app_init(AppState **out_state, const AppConfig *config) {
	if(out_state == nullptr) {
		return SDL_APP_FAILURE;
	}
	*out_state = nullptr;

	AppConfig cfg = (config != nullptr) ? *config : app_default_config();
	if(cfg.width <= 0) {
		cfg.width = APP_DEFAULT_WIDTH;
	}
	if(cfg.height <= 0) {
		cfg.height = APP_DEFAULT_HEIGHT;
	}
	if(cfg.title == nullptr || cfg.title[0] == '\0') {
		cfg.title = PROJECT_NAME;
	}

	(void)SDL_SetAppMetadata(cfg.title, PROJECT_VERSION,
							 "com.example.sdl3template");

	SDL_InitFlags flags = SDL_INIT_EVENTS;
	if(cfg.enable_video) {
		flags |= SDL_INIT_VIDEO;
	}
	if(cfg.enable_audio) {
		flags |= SDL_INIT_AUDIO;
	}
	if(cfg.enable_camera) {
		flags |= SDL_INIT_CAMERA;
	}

	if(!SDL_InitSubSystem(flags)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
					 "SDL_InitSubSystem failed: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	AppState *state = SDL_calloc(1, sizeof(*state));
	if(state == nullptr) {
		SDL_QuitSubSystem(flags);
		return SDL_APP_FAILURE;
	}

	state->config = cfg;
	state->init_flags = flags;
	state->play_test_tone = cfg.play_test_tone;
	state->start_ticks = SDL_GetTicks();

	if(cfg.enable_video && !app_init_video(state)) {
		SDL_QuitSubSystem(flags);
		SDL_free(state);
		return SDL_APP_FAILURE;
	}
	if(cfg.enable_audio) {
		app_init_audio(state);
	}
	if(cfg.enable_camera) {
		app_init_camera(state);
	}

	*out_state = state;
	return SDL_APP_CONTINUE;
}

SDL_AppResult app_event(AppState *state, const SDL_Event *event) {
	if(event == nullptr) {
		return SDL_APP_CONTINUE;
	}

	switch(event->type) {
	case SDL_EVENT_QUIT:
		return SDL_APP_SUCCESS;
	case SDL_EVENT_KEY_DOWN:
		if(event->key.key == SDLK_ESCAPE) {
			return SDL_APP_SUCCESS;
		}
		if(event->key.key == SDLK_SPACE && state != nullptr &&
		   state->audio_stream != nullptr) {
			if(state->play_test_tone) {
				state->play_test_tone = false;
				(void)SDL_ClearAudioStream(state->audio_stream);
			} else {
				state->play_test_tone = true;
			}
		}
		break;
	case SDL_EVENT_CAMERA_DEVICE_APPROVED:
		SDL_LogInfo(SDL_LOG_CATEGORY_VIDEO, "Camera access approved.");
		break;
	case SDL_EVENT_CAMERA_DEVICE_DENIED:
		SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Camera access denied.");
		break;
	default:
		break;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult app_iterate(AppState *state) {
	if(state == nullptr) {
		return SDL_APP_FAILURE;
	}

	++state->frame_count;
	app_pump_audio(state);
	app_update_camera(state);
	app_render(state);

	return SDL_APP_CONTINUE;
}

void app_quit(AppState *state, SDL_AppResult result) {
	(void)result;

	if(state == nullptr) {
		return;
	}

	if(state->camera_texture != nullptr) {
		SDL_DestroyTexture(state->camera_texture);
		state->camera_texture = nullptr;
	}
	if(state->camera != nullptr) {
		SDL_CloseCamera(state->camera);
		state->camera = nullptr;
	}
	if(state->audio_stream != nullptr) {
		SDL_DestroyAudioStream(state->audio_stream);
		state->audio_stream = nullptr;
	}
	if(state->renderer != nullptr) {
		SDL_DestroyRenderer(state->renderer);
		state->renderer = nullptr;
	}
	if(state->window != nullptr) {
		SDL_DestroyWindow(state->window);
		state->window = nullptr;
	}
	if(state->init_flags != 0) {
		SDL_QuitSubSystem(state->init_flags);
		state->init_flags = 0;
	}

	SDL_free(state);
	SDL_Quit();
}
