
global constexpr float pi =  3.1415926f;
global constexpr float tau = 6.283185f;
global constexpr float epsilon = 0.000001f;

struct s_v2
{
	float x;
	float y;
};

struct s_v3
{
	float x;
	float y;
	float z;
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

func s_v3 v3(float x, float y, float z)
{
	s_v3 result;
	result.x = x;
	result.y = y;
	result.z = z;
	return result;
}

func s_v3 v3_mul(s_v3 a, float b)
{
	s_v3 result;
	result.x = a.x * b;
	result.y = a.y * b;
	result.z = a.z * b;
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

func s_v4 v4(s_v3 v, float w)
{
	s_v4 result;
	result.x = v.x;
	result.y = v.y;
	result.z = v.z;
	result.w = w;
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

func float at_most(float a, float b)
{
	return b > a ? a : b;
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

func int floorfi(float x)
{
	return (int)floorf(x);
}

func float fract(float x)
{
	return x - (int)x;
}

func s_v2 operator-(s_v2 a, s_v2 b)
{
	s_v2 result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

func s_v2 operator+(s_v2 a, s_v2 b)
{
	s_v2 result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

func s_v2 operator*(s_v2 a, float b)
{
	s_v2 result;
	result.x = a.x * b;
	result.y = a.y * b;
	return result;
}

func s_v2 lerp(s_v2 a, s_v2 b, float t)
{
	s_v2 result;
	result.x = lerp(a.x, b.x, t);
	result.y = lerp(a.y, b.y, t);
	return result;
}

func float v2_length(s_v2 a)
{
	return sqrtf(a.x * a.x + a.y * a.y);
}

func float range_lerp(float input_val, float input_start, float input_end, float output_start, float output_end)
{
	return output_start + ((output_end - output_start) / (input_end - input_start)) * (input_val - input_start);
}

s_v3 hsv_to_rgb(s_v3 colour)
{
	s_v3 rgb;

	if(colour.y <= 0.0f)
	{
		rgb.x = colour.z;
		rgb.y = colour.z;
		rgb.z = colour.z;
		return rgb;
	}

	colour.x *= 360.0f;
	if(colour.x < 0.0f || colour.x >= 360.0f)
		colour.x = 0.0f;
	colour.x /= 60.0f;

	u32 i = (u32)colour.x;
	float ff = colour.x - i;
	float p = colour.z * (1.0f - colour.y );
	float q = colour.z * (1.0f - (colour.y * ff));
	float t = colour.z * (1.0f - (colour.y * (1.0f - ff)));

	switch(i)
	{
	case 0:
		rgb.x = colour.z;
		rgb.y = t;
		rgb.z = p;
		break;

	case 1:
		rgb.x = q;
		rgb.y = colour.z;
		rgb.z = p;
		break;

	case 2:
		rgb.x = p;
		rgb.y = colour.z;
		rgb.z = t;
		break;

	case 3:
		rgb.x = p;
		rgb.y = q;
		rgb.z = colour.z;
		break;

	case 4:
		rgb.x = t;
		rgb.y = p;
		rgb.z = colour.z;
		break;

	default:
		rgb.x = colour.z;
		rgb.y = p;
		rgb.z = q;
		break;
	}

	return rgb;
}
