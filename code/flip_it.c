#include "../raylib/include/raylib.h"
#include "flip_it.h"
#include "font.h"
#include "levels.h"
#include "text.c"

static all_data g_empty_data;
static settings g_settings;
static sounds g_sounds;

// wrapper to play the sound only if it is not muted in settings
static void play_sound(Sound s)
{
	if(!g_settings.mute) PlaySound(s);
}

// check if a cell is in range and flip it
// returns the number of cells turned on by the flip
static int flip_it_single(bool *b, int x, int y, int x_limit, int y_limit)
{
	int result = 0;
	if(x >= 0 && x < x_limit && y >= 0 && y < y_limit)
	{
		b[y*MAX_LEVEL_SIZE + x] = !b[y*MAX_LEVEL_SIZE + x];
		result = b[y*MAX_LEVEL_SIZE + x] ? 1 : -1;
	}
	return result;
}

// flip at the player position and the four neighbouring cells, updating the number of remaining cells on
static void flip_it(level_data *data)
{
	data->remaining_on += flip_it_single(data->cells, data->position.x, data->position.y, data->size.x, data->size.y);
	data->remaining_on += flip_it_single(data->cells, data->position.x - 1, data->position.y, data->size.x, data->size.y);
	data->remaining_on += flip_it_single(data->cells, data->position.x + 1, data->position.y, data->size.x, data->size.y);
	data->remaining_on += flip_it_single(data->cells, data->position.x, data->position.y - 1, data->size.x, data->size.y);
	data->remaining_on += flip_it_single(data->cells, data->position.x, data->position.y + 1, data->size.x, data->size.y);
}

static bool positions_equal(v2i a, v2i b)
{
	bool result = a.x == b.x && a.y == b.y;
	return result;
}

static bool get_digit(char c, int *digit)
{
	bool result = false;

	if(c >= '0' && c <= '9')
	{
		*digit = c - '0';
		result = true;
	}

	return result;
}

// deserialise the level from g_loaded_level which should be set before calling this function
// return whether the level was successfully loaded or not
static bool load_level(level_data *data)
{
	bool result = false;
	int digit;
	int bit_index = 0;
	char *level = g_loaded_level;
#define GET_DIGIT(destination) \
	do { \
		if(!get_digit(*level++, &digit)) goto end; \
		destination = digit; \
	} while(0)

	GET_DIGIT(data->size.x);
	++data->size.x;
	GET_DIGIT(data->size.y);
	++data->size.y;
	GET_DIGIT(data->position.x);
	GET_DIGIT(data->position.y);
	GET_DIGIT(data->target_position.x);
	GET_DIGIT(data->target_position.y);

	for(int y = 0; y < data->size.y; ++y)
	{
		for(int x = 0; x < data->size.x; ++x)
		{
			if(!bit_index)
			{
				bit_index = 3;
				GET_DIGIT(digit);
				if(digit > 7) goto end;
			}
			data->cells[y*MAX_LEVEL_SIZE + x] = (digit >> (bit_index - 1)) & 1;
			if(data->cells[y*MAX_LEVEL_SIZE + x] == 1) ++data->remaining_on;
			--bit_index;
		}
	}

	// if start position is same as target position and there are no cells on the level is won instantly
	// so to avoid that it is marked as an invalid level
	if(positions_equal(data->position, data->target_position) && data->remaining_on == 0) goto end;

	// check that the level string has ended
	result = *level == 0;

#undef GET_DIGIT
end:
	if(!result) TraceLog(LOG_WARNING, "Error loading data %s", g_loaded_level);
	return result;
}

// serialise the level out to g_level_buffer
static void save_level(level_data *data)
{
	char *level = g_level_buffer;
	int digit = 0;
	int bit_index = 0;
#define SET_DIGIT(source) \
	do { \
		*(level++) = '0' + (source); \
	} while(0)

	SET_DIGIT(data->size.x - 1);
	SET_DIGIT(data->size.y - 1);
	SET_DIGIT(data->position.x);
	SET_DIGIT(data->position.y);
	SET_DIGIT(data->target_position.x);
	SET_DIGIT(data->target_position.y);

	for(int y = 0; y < data->size.y; ++y)
	{
		for(int x = 0; x < data->size.x; ++x)
		{
			digit = (digit << 1) | data->cells[y*MAX_LEVEL_SIZE + x];
			++bit_index;
			if(bit_index == 3)
			{
				SET_DIGIT(digit);
				digit = bit_index = 0;
			}
		}
	}

	// padding bits if needed
	if(bit_index)
	{
		digit = digit << (3 - bit_index);
		SET_DIGIT(digit);
	}
#undef SET_DIGIT

	*level = 0;
}

static void draw_level(level_data *data)
{
	for(int y = 0; y < data->size.y; ++y)
	{
		for(int x = 0; x < data->size.x; ++x)
		{
			v2i cell_base = {(SCREEN_WIDTH - data->size.x*CELL_WIDTH)/2 + x*CELL_WIDTH, (SCREEN_HEIGHT - data->size.y*CELL_HEIGHT)/2 + y*CELL_HEIGHT};

			// draw a dark background for the grid and light/dark inside it based on cell state
			DrawRectangle(cell_base.x, cell_base.y, CELL_WIDTH, CELL_HEIGHT, DARK3310);
			DrawRectangle(cell_base.x + 1, cell_base.y + 1, CELL_WIDTH - 2, CELL_HEIGHT - 2, data->cells[y*MAX_LEVEL_SIZE + x] ? LIGHT3310 : DARK3310);

			// draw the player in the inverse colour of the cell
			if(data->position.x == x && data->position.y == y)
			{
				DrawRectangle(cell_base.x + CELL_WIDTH/4 + 1, cell_base.y + CELL_HEIGHT/4, 2, 1, data->cells[y*MAX_LEVEL_SIZE + x] ? DARK3310 : LIGHT3310);
				DrawRectangle(cell_base.x + CELL_WIDTH/4, cell_base.y + CELL_HEIGHT/4 + 1, 4, 1, data->cells[y*MAX_LEVEL_SIZE + x] ? DARK3310 : LIGHT3310);
			}

			// light cell corners to identify the target cell
			if(data->target_position.x == x && data->target_position.y == y)
			{
				DrawPixel(                 cell_base.x,                   cell_base.y, LIGHT3310);
				DrawPixel(CELL_WIDTH - 1 + cell_base.x,                   cell_base.y, LIGHT3310);
				DrawPixel(                 cell_base.x, CELL_HEIGHT - 1 + cell_base.y, LIGHT3310);
				DrawPixel(CELL_WIDTH - 1 + cell_base.x, CELL_HEIGHT - 1 + cell_base.y, LIGHT3310);
			}
		}
	}
}

static mode splash_screen(int *counter)
{
	mode result = mode_splash_screen;
	char title[] = "Flip It";
	int title_length = *counter / (FPS / 4);

	// animate writing out the title
	if(title_length < sizeof(title))
	{
		title[title_length] = 0;
	}
	draw_text(21, 10, title);

	if(title_length >= sizeof(title) && (*counter & 31) < 16)
	{
		draw_text(15, 30, "Press <\r>");
	}

	if(ANY_ENTER_PRESSED()) result = mode_main_menu;

	return result;
}

static mode main_menu(main_menu_data *data)
{
	int result = mode_main_menu;

typedef struct mode_list
{
	char *mode_name;
	mode mode_value;
} mode_list;
	// this is for looping over the modes to avoid manually writing out the text and setting the result
	mode_list modes[] =
	{
		{"Play", mode_select_level},
		{"Create", mode_editor},
		{"Help", mode_help},
		{"Settings", mode_settings_menu},
		{"Credits", mode_credits},
	};

	for(int mode_index = 0; mode_index < ARRAY_SIZE(modes); ++mode_index)
	{
		draw_text(16, 3 + mode_index * 9, modes[mode_index].mode_name);
	}
	draw_text(8, 3 + data->selected * 9, ">");

	if(data->selected > 0 && ANY_UP_PRESSED())
	{
		--data->selected;
	}
	else if(data->selected < 4 && ANY_DOWN_PRESSED())
	{
		++data->selected;
	}
	else if(ANY_ENTER_PRESSED())
	{
		result = modes[data->selected].mode_value;
	}
	else if(ANY_DIVIDE_PRESSED())
	{
		result = mode_splash_screen;
	}

	return result;
}

static mode select_level(select_level_data *data)
{
	int result = mode_select_level;
	char level_buffer[3] = "";
	level_buffer[0] = '0' + data->level / 10;
	level_buffer[1] = '0' + data->level % 10;

	draw_text(6, 2, "Select Level");
	draw_text(36, 20, level_buffer);
	draw_text(18, 40, "<0>-load");
	// up and down arrows around the level number
	DrawRectangle(42, 14, 1, 1, DARK3310);
	DrawRectangle(41, 15, 3, 1, DARK3310);
	DrawRectangle(40, 16, 5, 1, DARK3310);
	DrawRectangle(40, 30, 5, 1, DARK3310);
	DrawRectangle(41, 31, 3, 1, DARK3310);
	DrawRectangle(42, 32, 1, 1, DARK3310);

	if(data->level > 0 && ANY_DOWN_PRESSED())
	{
		--data->level;
	}
	else if(data->level < NUM_LEVELS - 1 && ANY_UP_PRESSED())
	{
		++data->level;
	}
	else if(ANY_ENTER_PRESSED())
	{
		g_loaded_level = g_level_strings[data->level];
		result = mode_play_level;
	}
	else if(ANY_DIVIDE_PRESSED())
	{
		result = mode_main_menu;
	}
	else if(NUM_KEY_PRESSED(0))
	{
		result = mode_loader;
	}

	return result;
}

static mode loader(int *counter)
{
	mode result = mode_loader;
	int length = string_length(g_level_buffer);

	// blinking text cursor if more text can be entered
	if(length < MAX_LEVEL_STRING_LENGTH)
	{
		g_level_buffer[length] = ((*counter >> 4) & 1) ? ' ' : '_';
		g_level_buffer[length+1] = 0;
	}

	draw_text(3, 2, "Enter a level");
	draw_text(0, 40, "</>-del <\r>-ok");
	// this text isn't scrollable by the player, it only scrolls when it overflows
	draw_scrollable_text(2, 10, 76, 28, g_level_buffer, -1);

	if(length < MAX_LEVEL_STRING_LENGTH)
	{
		g_level_buffer[length] = 0;
		for(int digit = 0; digit <= 9; ++digit)
		{
			if(NUM_KEY_PRESSED(digit))
			{
				g_level_buffer[length] = '0' + digit;
				break;
			}
		}
	}

	if(length > 0 && ANY_DIVIDE_PRESSED())
	{
		g_level_buffer[length-1] = 0;
	}
	else if(ANY_ENTER_PRESSED())
	{
		g_loaded_level = g_level_buffer;
		result = mode_play_level;
	}

	return result;
}

static mode play_level(level_data *data, int *counter)
{
	mode result = mode_play_level;
	bool moved = false;
	bool won = positions_equal(data->position, data->target_position) && data->remaining_on == 0;

	if(!won)
	{
		// handle input, move the player, and check for the winning condition
		if(ANY_RIGHT_PRESSED() && data->position.x < data->size.x - 1)
		{
			data->position.x += 1;
			moved = true;
		}
		else if(ANY_LEFT_PRESSED() && data->position.x > 0)
		{
			data->position.x -= 1;
			moved = true;
		}
		else if(ANY_DOWN_PRESSED() && data->position.y < data->size.y - 1)
		{
			data->position.y += 1;
			moved = true;
		}
		else if(ANY_UP_PRESSED() && data->position.y > 0)
		{
			data->position.y -= 1;
			moved = true;
		}

		if(moved)
		{
			flip_it(data);
			play_sound(g_sounds.move);
		}

		draw_level(data);

		if(positions_equal(data->position, data->target_position) && data->remaining_on == 0)
		{
			play_sound(g_sounds.win);
			*counter = 0;
		}
	}
	else
	{
		// win animation
		if(*counter < 30)
		{
			for (int y = 0; y < SCREEN_HEIGHT; ++y)
			{
				for (int x = 0; x < SCREEN_WIDTH; ++x)
				{
					if(((y + x) & 1) == ((*counter >> 2) & 1)) DrawPixel(x, y, DARK3310);
				}
			}
		}
		// load the next level if there is one
		else if(g_loaded_level >= g_level_strings[0] && g_loaded_level < g_level_strings[NUM_LEVELS - 1])
		{
			g_loaded_level += sizeof(*g_level_strings);
			load_level(data);
		}
		// win screen if there is no next level
		else
		{
			draw_text(18, 10, "A WINNER");
			draw_text(21, 18, "IS YOU!");
			draw_text(18, 40, "</>-exit");
		}
	}

	if(ANY_DIVIDE_PRESSED()) result = won ? mode_splash_screen : mode_select_level;

	return result;
}

// the editor works in modes
// init    - initialise data and set mode to start
// start   - size the grid and select the starting position
// draw    - move around flipping the cells
// end     - make the last move without a flip
// display - show the level string
static mode editor(editor_data *data)
{
	mode result = mode_editor;
	bool moved = false;

	if(data->mode == editor_mode_init)
	{
		data->level.size.x = 2;
		data->level.size.y = 2;
		data->level.target_position.x = -1;
		data->level.target_position.y = -1;
		data->mode = editor_mode_start;
	}

	if(data->mode != editor_mode_display)
	{
		if(ANY_RIGHT_PRESSED() && data->level.position.x < data->level.size.x - 1)
		{
			data->level.position.x += 1;
			moved = true;
		}
		else if(ANY_LEFT_PRESSED() && data->level.position.x > 0)
		{
			data->level.position.x -= 1;
			moved = true;
		}
		else if(ANY_DOWN_PRESSED() && data->level.position.y < data->level.size.y - 1)
		{
			data->level.position.y += 1;
			moved = true;
		}
		else if(ANY_UP_PRESSED() && data->level.position.y > 0)
		{
			data->level.position.y -= 1;
			moved = true;
		}
	}

	switch(data->mode)
	{
	case editor_mode_start:
	{
		if(NUM_KEY_PRESSED(1) && data->level.size.x > 1)
		{
			--data->level.size.x;
			if(data->level.position.x == data->level.size.x) --data->level.position.x;
		}
		else if(NUM_KEY_PRESSED(7) && data->level.size.x < MAX_LEVEL_SIZE)
		{
			++data->level.size.x;
		}
		else if(NUM_KEY_PRESSED(3) && data->level.size.y > 1)
		{
			--data->level.size.y;
			if(data->level.position.y == data->level.size.y) --data->level.position.y;
		}
		else if(NUM_KEY_PRESSED(9) && data->level.size.y < MAX_LEVEL_SIZE)
		{
			++data->level.size.y;
		}
		else if(ANY_ENTER_PRESSED())
		{
			// target cell needs to be flipped
			flip_it(&data->level);
			data->level.target_position = data->level.position;
			data->mode = editor_mode_draw;
		}
	} break;
	case editor_mode_draw:
	{
		if(moved)
		{
			flip_it(&data->level);
		}

		if(ANY_ENTER_PRESSED()) data->mode = editor_mode_end;
	} break;
	case editor_mode_end:
	{
		// border around the screen to identify end mode
		DrawRectangle(               0,                 0, SCREEN_WIDTH,             1, DARK3310);
		DrawRectangle(               0,                 0,            1, SCREEN_HEIGHT, DARK3310);
		DrawRectangle(               0, SCREEN_HEIGHT - 1, SCREEN_WIDTH,             1, DARK3310);
		DrawRectangle(SCREEN_WIDTH - 1,                 0,            1, SCREEN_HEIGHT, DARK3310);

		if(moved)
		{
			data->mode = editor_mode_display;
			save_level(&data->level);
			TraceLog(LOG_INFO, "Level Saved: %s", g_level_buffer);
		}
	} break;
	}

	if(data->mode != editor_mode_display)
	{
		draw_level(&data->level);
	}
	else
	{
		draw_text(12, 2, "Your level");
		data->scroll_position = draw_scrollable_text(2, 10, 76, 36, g_level_buffer, data->scroll_position);
		if(ANY_ENTER_PRESSED()) result = mode_main_menu;
	}

	if(ANY_DIVIDE_PRESSED()) result = mode_main_menu;

	return result;
}

static mode help(help_data *data)
{
	mode result = mode_help;

	data->scroll_position = draw_scrollable_text(2, 2, 76, 44, HELP_TEXT, data->scroll_position);
	if(ANY_DIVIDE_PRESSED()) result = mode_main_menu;

	return result;
}

static mode settings_menu()
{
	mode result = mode_settings_menu;

	if(ANY_ENTER_PRESSED()) g_settings.mute = !g_settings.mute;

	// should it be called settings if there is only one setting? (ﾟ ｰﾟ )ゝ
	draw_text(18, 2, "Settings");
	draw_text(10, 20, "Sound:");
	draw_text(52, 20, g_settings.mute ? "Off" : "On");
	draw_text(18, 40, "</>-back");

	if(ANY_DIVIDE_PRESSED()) result = mode_main_menu;

	return result;
}

static mode credits()
{
	mode result = mode_credits;

	draw_text(21, 2, "Credits");
	draw_text(2, 12, "Made by:");
	draw_text(2, 20, "Anuj Asher"); // that's me! ٩( ^ᴗ^ )۶
	draw_text(2, 30, "Made with:");
	draw_text(2, 38, "raylib");
	if(ANY_DIVIDE_PRESSED() || ANY_ENTER_PRESSED()) result = mode_main_menu;

	return result;
}

// (ノಠ益ಠ)ノ彡┻━┻
static mode load_error()
{
	mode result = mode_load_error;

	draw_text(21, 10, "Invalid");
	draw_text(24, 18, "level!");
	draw_text(18, 40, "</>-back");

	if(ANY_DIVIDE_PRESSED() || ANY_ENTER_PRESSED()) result = mode_select_level;

	return result;
}

int main()
{
	// initialise the window
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_UNDECORATED);
	InitWindow(SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, "Flip It");
	SetTargetFPS(FPS);

	// initialise audio, load sounds, and play the intro sound
	InitAudioDevice();
	g_sounds.intro = LoadSound("audio/intro.wav");
	g_sounds.move = LoadSound("audio/move.wav");
	g_sounds.win = LoadSound("audio/win.wav");
	play_sound(g_sounds.intro);

	// render to a 84x48 texture which is scaled up to the window
	RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
	SetTextureFilter(target.texture, FILTER_POINT);

	// initialise variables
	int counter = 0; // for animations
	mode running_mode = mode_splash_screen; // the current mode
	bool cells[MAX_LEVEL_SIZE][MAX_LEVEL_SIZE] = {false}; // the game grid, also used for the editor
	all_data data = {0}; // data passed to mode functions

	// the main loop
	while (!WindowShouldClose())
	{
		mode old_mode = running_mode; // save the old mode to detect when mode changes
		BeginDrawing();

			BeginTextureMode(target); // draw everything to the 84x48 texture

				ClearBackground(LIGHT3310);

				switch(running_mode)
				{
				case mode_splash_screen:
				{
					running_mode = splash_screen(&counter);
				} break;
				case mode_main_menu:
				{
					running_mode = main_menu(&data.main_menu);
				} break;
				case mode_select_level:
				{
					running_mode = select_level(&data.select_level);
				} break;
				case mode_loader:
				{
					running_mode = loader(&counter);
				} break;
				case mode_play_level:
				{
					running_mode = play_level(&data.play_level, &counter);
				} break;
				case mode_editor:
				{
					running_mode = editor(&data.editor);
				} break;
				case mode_help:
				{
					running_mode = help(&data.help);
				} break;
				case mode_settings_menu:
				{
					running_mode = settings_menu();
				} break;
				case mode_credits:
				{
					running_mode = credits();
				} break;
				case mode_load_error:
				{
					running_mode = load_error();
				} break;
				}

			EndTextureMode();

			// draw the texture to the screen
			DrawTexturePro(target.texture, (Rectangle){0.0f, 0.0f, target.texture.width, -target.texture.height}, (Rectangle){0.0f, 0.0f, SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE}, (Vector2){0, 0}, 0.0f, WHITE);

			// draw a grid over the texture for a more pixel-y look
			for(int y = 1; y < SCREEN_HEIGHT; ++y)
			{
				DrawRectangle(0, y*SCALE, SCREEN_WIDTH*SCALE, 1, GRID3310);
			}
			for(int x = 1; x < SCREEN_WIDTH; ++x)
			{
				DrawRectangle(x*SCALE, 0, 1, SCREEN_HEIGHT*SCALE, GRID3310);
			}

		EndDrawing();

		++counter;

		if(old_mode != running_mode)
		{
			// reset data and animation counter when mode changes
			data = g_empty_data;
			counter = 0;

			if(running_mode == mode_select_level)
			{
				// if the played level was not a custom level, mark it as the selected level to save some scrolling
				if(g_loaded_level >= g_level_strings[0] && g_loaded_level < g_level_strings[NUM_LEVELS])
				{
					data.select_level.level = (int)((g_loaded_level - g_level_strings[0])/sizeof(*g_level_strings));
				}
			}
			else if(running_mode == mode_play_level)
			{
				// load the level from g_loaded_level which should be set by whoever changed the mode to play_level
				data.play_level.cells = cells[0];
				if(!load_level(&data.play_level)) running_mode = mode_load_error;
			}
			else if(running_mode == mode_editor)
			{
				// clear the grid before starting the editor
				data.editor.level.cells = cells[0];
				for(int y = 0; y < MAX_LEVEL_SIZE; ++y) for(int x = 0; x < MAX_LEVEL_SIZE; ++x) cells[y][x] = 0;
			}
			else if(running_mode == mode_loader)
			{
				// reset the level buffer to an empty string
				g_level_buffer[0] = 0;
			}
			else if(running_mode == mode_splash_screen)
			{
				// play the intro sound again
				play_sound(g_sounds.intro);
			}
		}
	}

	// unloading ¯\_(ツ)_/¯
	UnloadSound(g_sounds.intro);
	UnloadSound(g_sounds.move);
	UnloadSound(g_sounds.win);
	CloseAudioDevice();
	CloseWindow();

	return 0;
}
