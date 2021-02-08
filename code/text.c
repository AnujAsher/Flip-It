static void draw_glyph(int x, int y, char c)
{
	for(int dy = 0; dy < FONT_HEIGHT; ++dy)
	{
		for(int dx = 0; dx < FONT_WIDTH; ++dx)
		{
			DrawPixel(x + dx, y + dy, g_font_glyph[c][dy][dx] ? DARK3310 : LIGHT3310);
		}
	}
}

static void draw_text(int x, int y, char *text)
{
	while(*text)
	{
		draw_glyph(x, y, *text++);
		x += FONT_WIDTH + FONT_H_SPACING;
		if(x >= SCREEN_WIDTH)
		{
			// no wrapping
			break;
		}
	}
}

static int string_length(char *str)
{
	int result = 0;

	while(*str++) ++result;

	return result;
}

// this is just dumb scrolling with absolutely no formatting/reflowing
static int draw_scrollable_text(int x, int y, int width, int height, char *text, int position)
{
	int length = string_length(text);
	int columns = width / FONT_WIDTH;
	int rows = height / (FONT_HEIGHT + FONT_V_SPACING);
	int total_rows = (length + columns - 1) / columns;
	int max_position = total_rows - rows;
	int row = 0;
	int column = 0;

	if(total_rows > rows)
	{
		if(position > 0 && ANY_UP_PRESSED()) --position;
		else if(position >= 0 && position < max_position && ANY_DOWN_PRESSED()) ++position;
		// support position = -1 for the loader
		if(position > max_position || position < 0) position = max_position;

		text += position * columns;
	}

	while(*text)
	{
		draw_glyph(x + column * FONT_WIDTH, y + row * (FONT_HEIGHT + FONT_V_SPACING), *text++);
		++column;
		if(column == columns)
		{
			column = 0;
			++row;
		}
		if(row == rows) break;
	}

	// draw the scrollbar only if it is needed
	if(total_rows > rows)
	{
		DrawRectangle(x + width + 3,              y, 1, height, DARK3310);
		DrawRectangle(x + width + 2,          1 + y, 3,      1, DARK3310);
		DrawRectangle(x + width + 2, height - 2 + y, 3,      1, DARK3310);
		int scroller_length = ((float)rows/(float)total_rows)*((float)(height-4));
		int scroller_position = ((float)position/(float)max_position)*(float)(height-3-scroller_length);
		DrawRectangle(x + width + 2, y + 2 + scroller_position, 1, scroller_length, DARK3310);
		DrawRectangle(x + width + 4, y + 2 + scroller_position, 1, scroller_length, DARK3310);
	}

	return position;
}
