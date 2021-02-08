#define MIN_LEVEL_SIZE 2
#define MAX_LEVEL_SIZE 10
// this is false it's actually (6 + (MAX_LEVEL_SIZE*MAX_LEVEL_SIZE)/3) because of octal
#define MAX_LEVEL_STRING_LENGTH (6 + MAX_LEVEL_SIZE*MAX_LEVEL_SIZE)
#define NUM_LEVELS 16

// level encoding
// l[0] : size.x
// l[1] : size.y
// l[2] : start_position.x
// l[3] : start_position.y
// l[4] : target_position.x
// l[5] : target_position.y
// l[6] onwards is size.x*size.y bits in octal
static char g_level_strings[NUM_LEVELS][MAX_LEVEL_STRING_LENGTH+1] =
{
	{"1000106"},
	{"220111272"},
	{"11001174"},
	{"04000466"},
	{"12100100"},
	{"221111252"},
	{"2301223257"},
	{"24001027700"},
	{"330213762564"},
	{"4100401720"},
	{"331122767770"},
	{"814140623673"},
	{"3411237324332"},
	{"2910196617464716"},
	{"550023217636363641"},
	{"7500140030303042510737"},
};

static char g_level_buffer[MAX_LEVEL_STRING_LENGTH+1];
static char *g_loaded_level = g_level_strings[0];
