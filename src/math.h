
#define pi (3.141f)

typedef struct s_v2
{
	float x;
	float y;
} s_v2;

typedef struct s_v4
{
	float x;
	float y;
	float z;
	float w;
} s_v4;

func s_v2 v2(float x, float y)
{
	s_v2 result;
	result.x = x;
	result.y = y;
	return result;
}

func s_v2 v22i(int x, int y)
{
	s_v2 result;
	result.x = (float)x;
	result.y = (float)y;
	return result;
}

func s_v2 v2ii(int x, int y)
{
	s_v2 result;
	result.x = (float)x;
	result.y = (float)y;
	return result;
}

func s_v2 v2_mul(s_v2 a, float b)
{
	s_v2 result;
	result.x = a.x * b;
	result.y = a.y * b;
	return result;
}

func s_v4 v4(float x, float y, float z, float w)
{
	s_v4 result;
	result.x = x;
	result.y = y;
	result.z = z;
	result.w = w;
	return result;
}
func s_v4 v41f(float v)
{
	s_v4 result;
	result.x = v;
	result.y = v;
	result.z = v;
	result.w = v;
	return result;
}

func b8 rect_collides_circle(s_v2 rect_center, s_v2 rect_size, s_v2 center, float radius)
{
	b8 collision = false;

	float dx = fabsf(center.x - (float)rect_center.x);
	float dy = fabsf(center.y - (float)rect_center.y);

	if(dx > (rect_size.x/2.0f + radius)) { return false; }
	if(dy > (rect_size.y/2.0f + radius)) { return false; }

	if(dx <= (rect_size.x/2.0f)) { return true; }
	if(dy <= (rect_size.y/2.0f)) { return true; }

	float cornerDistanceSq = (dx - rect_size.x/2.0f)*(dx - rect_size.x/2.0f) +
													(dy - rect_size.y/2.0f)*(dy - rect_size.y/2.0f);

	collision = (cornerDistanceSq <= (radius*radius));

	return collision;
}

func s_v2 v2_from_angle(float angle)
{
	return v2(
		cosf(angle),
		sinf(angle)
	);
}

func s_v2 v2_sub(s_v2 a, s_v2 b)
{
	s_v2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

func float v2_angle(s_v2 v)
{
	return atan2f(v.y, v.x);
}
