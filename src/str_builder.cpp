

func void builder_add_(s_str_builder* builder, const char* what, b8 use_tabs, va_list args)
{
	if(use_tabs)
	{
		for(int tab_i = 0; tab_i < builder->tab_count; tab_i++)
		{
			builder->data[builder->len++] = '\t';
		}
	}
	char* where_to_write = &builder->data[builder->len];
	int written = vsnprintf(where_to_write, c_str_builder_size + 1 - builder->len, what, args);
	assert(written > 0 && written < c_str_builder_size);
	builder->len += written;
	assert(builder->len < c_str_builder_size);
	builder->data[builder->len] = 0;
}

func void builder_add(s_str_builder* builder, const char* what, ...)
{
	va_list args;
	va_start(args, what);
	builder_add_(builder, what, false, args);
	va_end(args);
}

func void builder_add_char(s_str_builder* builder, char c)
{
	assert(builder->len < c_str_builder_size);
	builder->data[builder->len++] = c;
	builder->data[builder->len] = 0;
}

func void builder_add_with_tabs(s_str_builder* builder, const char* what, ...)
{
	va_list args;
	va_start(args, what);
	builder_add_(builder, what, true, args);
	va_end(args);
}

func void builder_add_line(s_str_builder* builder, const char* what, ...)
{
	va_list args;
	va_start(args, what);
	builder_add_(builder, what, false, args);
	va_end(args);
	builder_add(builder, "\n");
}

func void builder_add_line_with_tabs(s_str_builder* builder, const char* what, ...)
{
	va_list args;
	va_start(args, what);
	builder_add_(builder, what, true, args);
	va_end(args);
	builder_add(builder, "\n");
}

func void builder_add_tabs(s_str_builder* builder)
{
	for(int tab_i = 0; tab_i < builder->tab_count; tab_i++)
	{
		builder->data[builder->len++] = '\t';
	}
}

func void builder_line(s_str_builder* builder)
{
	builder_add(builder, "\n");
}

func void builder_push_tab(s_str_builder* builder)
{
	assert(builder->tab_count <= 64);
	builder->tab_count++;
}

func void builder_pop_tab(s_str_builder* builder)
{
	assert(builder->tab_count > 0);
	builder->tab_count--;
}

