#include <furi.h>
#include <dolphin/dolphin.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/menu.h>

#define APPLICATION_NAME "DSDetail"

/* These values aren't exported anywhere, so we redefine them here.
 * This needs to be changed if the level system in the firmware changes. */
#define LEVEL2_THRESHOLD 300
#define LEVEL3_THRESHOLD 1800
#define BUTTHURT_MAX 14
#define LEVEL_MAX 3

typedef struct {
	SceneManager *scene_manager;
	ViewDispatcher *view_dispatcher;
	Menu *menu;
	char icounter_str[32];
	char butthurt_str[32];
	char level_str[20];
} DSDetailState;

typedef enum {
	DSDetailMenuItems_icounter,
	DSDetailMenuItems_butthurt,
	DSDetailMenuItems_level
} DSDetailMenuItems;

static void ds_detail_menu_callback_main(void* context, uint32_t index)
{
	FURI_LOG_T(APPLICATION_NAME, "ds_detail_scene_on_enter_main");

	furi_assert(context);
	DSDetailState* state = context;

	switch(index) {
	case DSDetailMenuItems_icounter:
		FURI_LOG_I(APPLICATION_NAME, "Menu item icounter!");
		scene_manager_handle_custom_event(state->scene_manager, 0);
		break;
	case DSDetailMenuItems_butthurt:
		FURI_LOG_I(APPLICATION_NAME, "Menu item butthurt!");
		scene_manager_handle_custom_event(state->scene_manager, 0);
		break;
	case DSDetailMenuItems_level:
		FURI_LOG_I(APPLICATION_NAME, "Menu item level!");
		scene_manager_handle_custom_event(state->scene_manager, 0);
		break;
	default:
		FURI_LOG_W(APPLICATION_NAME, "Unknown menu item!");
		break;
	}
}

static void ds_detail_scene_on_enter_main(void* context)
{
	FURI_LOG_T(APPLICATION_NAME, "ds_detail_scene_on_enter_main");

	furi_assert(context);
	DSDetailState* state = context;

	menu_reset(state->menu);

	menu_add_item(
		state->menu,
		state->icounter_str,
		NULL,
		DSDetailMenuItems_icounter,
		ds_detail_menu_callback_main,
		state
	);
	menu_add_item(
		state->menu,
		state->butthurt_str,
		NULL,
		DSDetailMenuItems_butthurt,
		ds_detail_menu_callback_main,
		state
	);
	menu_add_item(
		state->menu,
		state->level_str,
		NULL,
		DSDetailMenuItems_level,
		ds_detail_menu_callback_main,
		state
	);

	view_dispatcher_switch_to_view(state->view_dispatcher, 0);
}

static bool ds_detail_scene_on_event_main(void* context, SceneManagerEvent event)
{
	UNUSED(event);

	FURI_LOG_T(APPLICATION_NAME, "ds_detail_scene_on_event_main");
	furi_assert(context);
	return false;
}

static void ds_detail_scene_on_exit_main(void* context)
{
	FURI_LOG_T(APPLICATION_NAME, "ds_detail_scene_on_exit_main");

	furi_assert(context);
	DSDetailState* state = context;

	menu_reset(state->menu);
}

static void (*const ds_detail_scene_on_enter_handlers[])(void*) = {
	ds_detail_scene_on_enter_main
};

static bool (*const ds_detail_scene_on_event_handlers[])(void*, SceneManagerEvent) = {
	ds_detail_scene_on_event_main
};

static void (*const ds_detail_scene_on_exit_handlers[])(void*) = {
	ds_detail_scene_on_exit_main
};

static const SceneManagerHandlers ds_detail_scene_event_handlers = {
	.on_enter_handlers = ds_detail_scene_on_enter_handlers,
	.on_event_handlers = ds_detail_scene_on_event_handlers,
	.on_exit_handlers = ds_detail_scene_on_exit_handlers,
	.scene_num = 1};

static bool ds_detail_custom_event_callback(void* context, uint32_t event)
{
	FURI_LOG_T(APPLICATION_NAME, "ds_detail_custom_event_callback");
	furi_assert(context);
	DSDetailState* state = context;
	return scene_manager_handle_custom_event(state->scene_manager, event);
}

static bool ds_detail_navigation_event_callback(void* context)
{
	FURI_LOG_T(APPLICATION_NAME, "ds_detail_navigation_event_callback");
	furi_assert(context);
	DSDetailState* state = context;
	return scene_manager_handle_back_event(state->scene_manager);
}

static void copy_dolphin_state(DSDetailState *state)
{
	/* no error handling here, don't know how */
	Dolphin* dolphin = furi_record_open(RECORD_DOLPHIN);
	DolphinStats stats = dolphin_stats(dolphin);
	furi_record_close(RECORD_DOLPHIN);

	uint32_t threshold = 0;
	switch (stats.level) {
	case 1:
		threshold = LEVEL2_THRESHOLD;
		break;
	case 2:
	default:
		threshold = LEVEL3_THRESHOLD;
		break;
	}

	snprintf(state->icounter_str, sizeof(state->icounter_str),
			"Icounter: %lu/%lu",
			stats.icounter,
			threshold);
	state->icounter_str[sizeof(state->icounter_str) - 1] = '\0';
	snprintf(state->butthurt_str, sizeof(state->butthurt_str),
			"Butthurt: %lu/%lu",
			stats.butthurt,
			(uint32_t) BUTTHURT_MAX);
	state->butthurt_str[sizeof(state->butthurt_str) - 1] = '\0';
	snprintf(state->level_str, sizeof(state->level_str),
			"Level: %hu/%hu",
			stats.level,
			(uint8_t) LEVEL_MAX);
	state->level_str[sizeof(state->level_str) - 1] = '\0';
}

int32_t ds_detail(void)
{
	int32_t err = -1;

	FURI_LOG_I(APPLICATION_NAME, "Starting...");

	DSDetailState *state = malloc(sizeof(DSDetailState));
	if (state == NULL) {
		goto err_alloc;
	}

	state->scene_manager = scene_manager_alloc(&ds_detail_scene_event_handlers, state);
	if (state->scene_manager == NULL) {
		goto err_alloc_sm;
	}

	state->view_dispatcher = view_dispatcher_alloc();
	if (state->view_dispatcher == NULL) {
		goto err_alloc_vd;
	}

	state->menu = menu_alloc();
	if (state->menu == NULL) {
		goto err_alloc_m;
	}

	copy_dolphin_state(state);

	view_dispatcher_enable_queue(state->view_dispatcher);

	view_dispatcher_set_event_callback_context(state->view_dispatcher, state);
	view_dispatcher_set_custom_event_callback(
			state->view_dispatcher,
			ds_detail_custom_event_callback);
	view_dispatcher_set_navigation_event_callback(
			state->view_dispatcher,
			ds_detail_navigation_event_callback);

	view_dispatcher_add_view(state->view_dispatcher, 0, menu_get_view(state->menu));

	/* no error handling here, don't know how */
	Gui *gui = furi_record_open(RECORD_GUI);
	view_dispatcher_attach_to_gui(state->view_dispatcher, gui, ViewDispatcherTypeFullscreen);

	scene_manager_next_scene(state->scene_manager, 0);
	view_dispatcher_run(state->view_dispatcher);

	err = 0;

    	furi_record_close(RECORD_GUI);
	view_dispatcher_remove_view(state->view_dispatcher, 0);

	menu_free(state->menu);

err_alloc_m:
	view_dispatcher_free(state->view_dispatcher);

err_alloc_vd:
	scene_manager_free(state->scene_manager);

err_alloc_sm:
	free(state);

err_alloc:
	if (err != 0) {
		FURI_LOG_E(APPLICATION_NAME, "Failed to launch (alloc error)!");
	} else {
		FURI_LOG_I(APPLICATION_NAME, "Clean exit.");
	}

	return err;
}
