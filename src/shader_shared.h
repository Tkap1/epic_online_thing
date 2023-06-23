

#ifdef m_app
	#define shader_v2 _Alignas(8) s_v2
	#define shader_v4 _Alignas(16) s_v4
	#define shader_float _Alignas(4) float
	#define shader_int _Alignas(4) int
	#define shader_bool _Alignas(4) b32
#else
	#define shader_v2 vec2
	#define shader_v4 vec4
	#define shader_float float
	#define shader_int int
	#define shader_bool bool
#endif

#ifdef m_app
typedef
#endif
struct s_transform
{
	shader_bool do_clip;
	shader_bool do_circle;
	shader_bool do_light;
	shader_bool do_background;
	shader_bool use_texture;
	shader_bool flip_x;
	shader_int layer;
	shader_int sublayer;
	shader_float mix_weight;
	shader_v2 pos;
	shader_v2 origin_offset;
	shader_v2 draw_size;
	shader_v2 texture_size;
	shader_v2 clip_pos;
	shader_v2 clip_size;
	shader_v2 uv_min;
	shader_v2 uv_max;
	shader_v4 color;
	shader_v4 mix_color;
}
#ifdef m_app
s_transform
#endif
;

