

struct s_small_str
{
	static constexpr int max_chars = 31;
	int len;
	char data[max_chars + 1];
};

struct s_medium_str
{
	static constexpr int max_chars = 63;
	int len;
	char data[max_chars + 1];
};

