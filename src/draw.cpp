
func void draw_rect(s_v2 pos, int layer, s_v2 size, s_v4 color, s_transform t)
{
	t.pos = pos;
	t.layer = layer;
	t.draw_size = size;
	t.color = color;
	t.uv_min = v2(0, 0);
	t.uv_max = v2(1, 1);
	t.mix_color = v41f(1);
	transforms.add(t);
}

func void draw_circle(s_v2 pos, int layer, float radius, s_v4 color, s_transform t)
{
	t.do_circle = true;
	t.pos = pos;
	t.layer = layer;
	t.draw_size = v2(radius, radius);
	t.color = color;
	t.uv_min = v2(0, 0);
	t.uv_max = v2(1, 1);
	t.mix_color = v41f(1);
	transforms.add(t);
}

func void draw_light(s_v2 pos, int layer, float radius, s_v4 color, s_transform t)
{
	t.do_light = true;
	t.pos = pos;
	t.layer = layer;
	t.draw_size = v2(radius, radius);
	t.color = color;
	t.uv_min = v2(0, 0);
	t.uv_max = v2(1, 1);
	t.mix_color = v41f(1);
	transforms.add(t);
}

func void draw_texture(s_v2 pos, int layer, s_v2 size, s_v4 color, s_transform t)
{
	t.layer = layer;
	t.use_texture = true;
	t.pos = pos;
	t.draw_size = size;
	t.color = color;
	t.uv_min = v2(0, 0);
	t.uv_max = v2(1, 1);
	t.mix_color = v41f(1);
	transforms.add(t);
}

func void draw_text(const char* text, s_v2 in_pos, int layer, s_v4 color, e_font font_id, b8 centered, s_transform t)
{
	t.layer = layer;

	s_font* font = &game->font_arr[font_id];

	int len = (int)strlen(text);
	assert(len > 0);
	s_v2 pos = in_pos;
	if(centered)
	{
		s_v2 size = get_text_size(text, font_id);
		pos.x -= size.x / 2;
		pos.y -= size.y / 2;
	}
	pos.y += font->ascent * font->scale;
	for(int char_i = 0; char_i < len; char_i++)
	{
		int c = text[char_i];
		if(c <= 0 || c >= 128) { continue; }

		s_glyph glyph = font->glyph_arr[c];
		s_v2 glyph_pos = pos;
		glyph_pos.y += -glyph.y0 * font->scale;

		t.use_texture = true;
		t.pos = glyph_pos;
		t.draw_size = v2((glyph.x1 - glyph.x0) * font->scale, (glyph.y1 - glyph.y0) * font->scale);
		// t.draw_size = v2(glyph.width, glyph.height);
		t.color = color;
		// t.texture_size = texture.size;
		t.uv_min = glyph.uv_min;
		t.uv_max = glyph.uv_max;
		t.origin_offset = c_origin_bottomleft;
		text_arr[font_id].add(t);

		pos.x += glyph.advance_width * font->scale;

	}
}
