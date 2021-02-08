#define ARRAY_SIZE(array) (sizeof(array)/sizeof((array)[0]))

// nokia 3310 colours
#define LIGHT3310 CLITERAL(Color){ 199, 240, 216, 255 }
#define GRID3310 CLITERAL(Color){ 214, 255, 234, 255 }
#define DARK3310 CLITERAL(Color){ 67, 82, 61, 255 }

#define CELL_WIDTH 8
#define CELL_HEIGHT 4
#define SCREEN_WIDTH 84
#define SCREEN_HEIGHT 48
#define SCALE 5
#define FPS 30

// to accomodate for people without numpads
#define NUM_KEY_PRESSED(n) (IsKeyPressed(KEY_ZERO + (n)) || IsKeyPressed(KEY_KP_0 + (n)))
#define ANY_RIGHT_PRESSED() (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT) || NUM_KEY_PRESSED(6))
#define ANY_LEFT_PRESSED() (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT) || NUM_KEY_PRESSED(4))
#define ANY_DOWN_PRESSED() (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN) || NUM_KEY_PRESSED(2))
#define ANY_UP_PRESSED() (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP) || NUM_KEY_PRESSED(8))
#define ANY_DIVIDE_PRESSED() (IsKeyPressed(KEY_KP_DIVIDE) || IsKeyPressed(KEY_SLASH))
#define ANY_DECIMAL_PRESSED() (IsKeyPressed(KEY_KP_DECIMAL) || IsKeyPressed(KEY_PERIOD))

// this is formatted weirdly because the printing function doesn't do any formatting
#define HELP_TEXT \
	"Move the car" \
	"around the  " \
	"grid to turn" \
	"off all the " \
	"lights and  " \
	"reach the   " \
	"target cell." \
	"Moving to a " \
	"cell flips  " \
	"that cell   " \
	"and the four" \
	"neighbouring" \
	"cells."

typedef struct v2i {
	int x;
	int y;
} v2i;

typedef enum mode
{
	mode_splash_screen,
	mode_main_menu,
	mode_select_level,
	mode_loader,
	mode_play_level,
	mode_editor,
	mode_help,
	mode_settings_menu,
	mode_credits,
	mode_load_error,
} mode;

typedef enum editor_mode
{
	editor_mode_init,
	editor_mode_start,
	editor_mode_draw,
	editor_mode_end,
	editor_mode_display,
} editor_mode;

typedef struct settings
{
	bool mute;
} settings;

typedef struct sounds
{
	Sound intro;
	Sound move;
	Sound win;
} sounds;

typedef struct level_data
{
	v2i size;
	v2i position;
	v2i target_position;
	int remaining_on;
	bool *cells;
} level_data;

typedef struct main_menu_data
{
	int selected;
} main_menu_data;

typedef struct select_level_data
{
	int level;
} select_level_data;

typedef struct help_data
{
	int scroll_position;
} help_data;

typedef struct editor_data
{
	level_data level;
	editor_mode mode;
	int scroll_position;
} editor_data;

typedef union all_data
{
	main_menu_data main_menu;
	select_level_data select_level;
	help_data help;
	level_data play_level;
	editor_data editor;
} all_data;
