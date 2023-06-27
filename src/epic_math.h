
#define pi (3.141f)
global constexpr float epsilon = 0.000001f;

struct s_v2
{
	float x;
	float y;
};

struct s_v4
{
	float x;
	float y;
	float z;
	float w;
};

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

func float lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

func int roundfi(float x)
{
	return (int)roundf(x);
}

func float sinf2(float t)
{
	return sinf(t) * 0.5f + 0.5f;
}

func float deg_to_rad(float d)
{
	return d * (pi / 180.f);
}

func float rad_to_deg(float r)
{
	return r * (180.f / pi);
}

func float at_least(float a, float b)
{
	return a > b ? a : b;
}

func int at_least(int a, int b)
{
	return a > b ? a : b;
}

func b8 floats_equal(float a, float b)
{
	return (a >= b - epsilon && a <= b + epsilon);
}

func float ilerp(float start, float end, float val)
{
	float b = end - start;
	if(floats_equal(b, 0)) { return val; }
	return (val - start) / b;
}