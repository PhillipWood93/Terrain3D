//Copyright © 2023 Roope Palmroos, Cory Petkovsek, and Contributors. All rights reserved. See LICENSE.
#ifndef TERRAINSTORAGE_CLASS_H
#define TERRAINSTORAGE_CLASS_H

#ifdef WIN32
#include <windows.h>
#endif

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/shader_material.hpp>

#include "terrain_surface.h"

class Terrain3D;
using namespace godot;

#define COLOR_ZERO Color(0.0f, 0.0f, 0.0f, 0.0f)
#define COLOR_BLACK Color(0.0f, 0.0f, 0.0f, 1.0f)
#define COLOR_WHITE Color(1.0f, 1.0f, 1.0f, 1.0f)
#define COLOR_ROUGHNESS Color(1.0f, 1.0f, 1.0f, 0.5f)
#define COLOR_RB Color(1.0f, 0.0f, 1.0f, 1.0f)
#define COLOR_NORMAL Color(0.5f, 0.5f, 1.0f, 1.0f)

class Terrain3DStorage : public Resource {
	GDCLASS(Terrain3DStorage, Resource);

	enum MapType {
		TYPE_HEIGHT,
		TYPE_CONTROL,
		TYPE_COLOR,
		TYPE_MAX,
	};

	static inline const Image::Format FORMAT[] = {
		Image::FORMAT_RF, // TYPE_HEIGHT
		Image::FORMAT_RGBA8, // TYPE_CONTROL
		Image::FORMAT_RGBA8, // TYPE_COLOR
		Image::Format(TYPE_MAX), // Proper size of array instead of FORMAT_MAX
	};

	static inline const char *TYPESTR[] = {
		"TYPE_HEIGHT",
		"TYPE_CONTROL",
		"TYPE_COLOR",
		"TYPE_MAX",
	};

	static inline const Color COLOR[] = {
		COLOR_BLACK, // TYPE_HEIGHT
		COLOR_BLACK, // TYPE_CONTROL
		COLOR_ROUGHNESS, // TYPE_COLOR
		COLOR_ZERO, // TYPE_MAX, unused just in case someone indexes the array
	};

	enum RegionSize {
		SIZE_64 = 64,
		SIZE_128 = 128,
		SIZE_256 = 256,
		SIZE_512 = 512,
		SIZE_1024 = 1024,
		SIZE_2048 = 2048,
	};

	static const int REGION_MAP_SIZE = 16;
	static inline const Vector2i REGION_MAP_VSIZE = Vector2i(REGION_MAP_SIZE, REGION_MAP_SIZE);

	struct Generated {
		RID rid = RID();
		Ref<Image> image;
		bool dirty = false;

	public:
		void clear();
		bool is_dirty() { return dirty; }
		void create(const TypedArray<Image> &p_layers);
		void create(const Ref<Image> &p_image);
		Ref<Image> get_image() const { return image; }
		RID get_rid() { return rid; }
	};

	RegionSize region_size = SIZE_1024;
	Vector2i region_vsize = Vector2i(region_size, region_size);

	Terrain3D *terrain = nullptr;

	float _version = 0.8;
	bool _save_16_bit = false;

	RID material;
	RID shader;
	bool shader_override_enabled = false;
	Ref<ShaderMaterial> shader_override;

	bool noise_enabled = false;
	float noise_scale = 2.0;
	float noise_height = 300.0;
	float noise_blend_near = 0.5;
	float noise_blend_far = 1.0;

	bool surfaces_enabled = false;

	/**
	 * These arrays house all of the storage data.
	 * The Image arrays are region_sized slices of all heightmap data. Their world
	 * location is tracked by region_offsets. The region data are combined into one large
	 * texture in generated_*_maps.
	 */
	TypedArray<Vector2i> region_offsets;
	TypedArray<Terrain3DSurface> surfaces;
	TypedArray<Image> height_maps;
	TypedArray<Image> control_maps;
	TypedArray<Image> color_maps;

	Vector2 _height_range = Vector2(0, 0);

	// These contain an Image described below and a texture RID from the RenderingServer
	Generated generated_region_map; // REGION_MAP_SIZE^2 sized texture w/ active regions
	Generated generated_region_blend_map; // 512x512 blurred version of above for blending
	// These contain the TextureLayered RID from the RenderingServer, no Image
	Generated generated_height_maps;
	Generated generated_control_maps;
	Generated generated_color_maps;
	Generated generated_albedo_textures;
	Generated generated_normal_textures;

	bool _initialized = false;
	bool _modified = false;

private:
	void _update_surfaces();
	void _update_surface_data(bool p_update_textures, bool p_update_values);
	void _update_regions();
	void _update_material();
	String _generate_shader_code();

	void _clear();
	Vector2i _get_offset_from(Vector3 p_global_position);

protected:
	static void _bind_methods();

public:
	Terrain3DStorage();
	~Terrain3DStorage();

	inline void set_terrain(Terrain3D *p_terrain) { terrain = p_terrain; }
	inline Terrain3D *get_terrain() const { return terrain; }

	inline void set_version(float p_version) { _version = p_version; }
	inline float get_version() const { return _version; }

	inline void set_save_16_bit(bool p_enabled) { _save_16_bit = p_enabled; }
	inline bool get_save_16_bit() const { return _save_16_bit; }

	void _clear_modified() { _modified = false; }
	void save();

	void print_audit_data();

	void set_region_size(RegionSize p_size);
	RegionSize get_region_size() const { return region_size; }

	inline void set_height_range(Vector2 p_range) { _height_range = p_range; }
	inline Vector2 get_height_range() const { return _height_range; }
	void update_heights(float p_height);
	void update_heights(Vector2 p_heights);
	void update_height_range();

	void set_surface(const Ref<Terrain3DSurface> &p_material, int p_index);
	Ref<Terrain3DSurface> get_surface(int p_index) const { return surfaces[p_index]; }
	void set_surfaces(const TypedArray<Terrain3DSurface> &p_surfaces);
	TypedArray<Terrain3DSurface> get_surfaces() const { return surfaces; }
	int get_surface_count() const { return surfaces.size(); }

	// Workaround until callable_mp is implemented
	void update_surface_textures();
	void update_surface_values();

	Error add_region(Vector3 p_global_position, const TypedArray<Image> &p_images = TypedArray<Image>(), bool p_update = true);
	void remove_region(Vector3 p_global_position, bool p_update = true);
	bool has_region(Vector3 p_global_position);
	int get_region_index(Vector3 p_global_position);
	void set_region_offsets(const TypedArray<Vector2i> &p_array);
	TypedArray<Vector2i> get_region_offsets() const { return region_offsets; }
	int get_region_count() const { return region_offsets.size(); }

	void set_map_region(MapType p_map_type, int p_region_index, const Ref<Image> p_image);
	Ref<Image> get_map_region(MapType p_map_type, int p_region_index) const;
	void set_maps(MapType p_map_type, const TypedArray<Image> &p_maps);
	TypedArray<Image> get_maps(MapType p_map_type) const;
	TypedArray<Image> get_maps_copy(MapType p_map_type) const;
	void set_height_maps(const TypedArray<Image> &p_maps);
	TypedArray<Image> get_height_maps() const { return height_maps; }
	void set_control_maps(const TypedArray<Image> &p_maps);
	TypedArray<Image> get_control_maps() const { return control_maps; }
	void set_color_maps(const TypedArray<Image> &p_maps);
	TypedArray<Image> get_color_maps() const { return color_maps; }
	Color get_pixel(MapType p_map_type, Vector3 p_global_position);
	inline float get_height(Vector3 p_global_position) { return get_pixel(TYPE_HEIGHT, p_global_position).r; }
	inline Color get_color(Vector3 p_global_position);
	inline Color get_control(Vector3 p_global_position) { return get_pixel(TYPE_CONTROL, p_global_position); }
	inline float get_roughness(Vector3 p_global_position) { return get_pixel(TYPE_COLOR, p_global_position).a; }
	TypedArray<Image> sanitize_maps(MapType p_map_type, const TypedArray<Image> &p_maps);
	void force_update_maps(MapType p_map = TYPE_MAX);

	static Ref<Image> load_image(String p_file_name, int p_cache_mode = ResourceLoader::CACHE_MODE_IGNORE,
			Vector2 p_r16_height_range = Vector2(0, 255), Vector2i p_r16_size = Vector2i(0, 0));
	void import_images(const TypedArray<Image> &p_images, Vector3 p_global_position = Vector3(0, 0, 0),
			float p_offset = 0.0, float p_scale = 1.0);
	Error export_image(String p_file_name, MapType p_map_type = TYPE_HEIGHT);
	Ref<Image> layered_to_image(MapType p_map_type);
	static Vector2 get_min_max(const Ref<Image> p_image);
	static Ref<Image> get_thumbnail(const Ref<Image> p_image, Vector2i p_size = Vector2i(256, 256));
	static Ref<Image> get_filled_image(Vector2i p_size, Color p_color = COLOR_BLACK, bool create_mipmaps = true, Image::Format format = FORMAT[TYPE_HEIGHT]);

	RID get_material() const { return material; }
	void set_shader_override(const Ref<ShaderMaterial> &p_shader);
	Ref<ShaderMaterial> get_shader_override() const { return shader_override; }
	void enable_shader_override(bool p_enabled);
	bool is_shader_override_enabled() const { return shader_override_enabled; }

	RID get_region_blend_map() { return generated_region_blend_map.get_rid(); }
	void set_noise_enabled(bool p_enabled);
	void set_noise_scale(float p_scale);
	void set_noise_height(float p_height);
	void set_noise_blend_near(float p_near);
	void set_noise_blend_far(float p_far);
	bool get_noise_enabled() const { return noise_enabled; }
	float get_noise_scale() const { return noise_scale; };
	float get_noise_height() const { return noise_height; };
	float get_noise_blend_near() const { return noise_blend_near; };
	float get_noise_blend_far() const { return noise_blend_far; };
};

VARIANT_ENUM_CAST(Terrain3DStorage::MapType);
VARIANT_ENUM_CAST(Terrain3DStorage::RegionSize);

#endif // TERRAINSTORAGE_CLASS_H
