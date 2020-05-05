#pragma once
#include "Engine/Math/Convex.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/AABB2.hpp"

class Zone
{
public:
	Vec2 m_position;
	ConvexPoly m_poly;
	ConvexHull2 m_hull;
public:
	void scale(float scale_by, const Vec2& scale_center=Vec2::ZERO)
	{
		m_poly.scale(scale_by, m_position, scale_center);
		m_hull = ConvexHull2(m_poly);
	}
	void rotate(float angle, const Vec2& center=Vec2::ZERO)
	{
		m_poly.rotate(angle, m_position, center);
		m_hull = ConvexHull2(m_poly);
		m_position = center;
	}
};

constexpr size_t QUAD_ZONE_LIMIT = 2;

class QuadTree
{
public:
	static constexpr size_t MAX_DEPTH = 5;
public:
	QuadTree() = default;
	QuadTree(const AABB2& box) : m_box(box) {}
	~QuadTree();
	void build_tree(size_t depth=0);
	void display() const;
	ConvexImpactResult raycast_by(const Ray2& ray, bool set_flag=false);
	
	void reset_tree_flag();
	
	AABB2 m_box;
	QuadTree* m_sub[4] {nullptr, nullptr, nullptr, nullptr};
	std::vector<Zone*>	m_zones;
	bool m_checked = false;
};

class RVSGame
{
public:
	~RVSGame();
	void Startup(size_t numPolys=10);
	void BeginFrame();
	void Update(float deltaSeconds);
	void Render() const;

	void EndFrame();
	void Shutdown();

	void mouse_down(const Vec2& mouse_pos);
	void mouse_up(const Vec2& mouse_pos);
	void snap_on();
	void snap_off();
	void wheel_up(const Vec2& mouse_pos);
	void wheel_down(const Vec2& mouse_pos);

	
	bool load_ghcs(NamedStrings& param);
	bool save_ghcs(NamedStrings& param);

	void raycast_to_all(const Ray2& ray);
	void _update_quad_tree();
	Zone* get_first_zone_include(const Vec2& position);


public:
	std::vector<Zone> m_zones;
	Vec2 m_mouse_start;
	Vec2 m_mouse_end;
	bool m_raycast_on = false;
	ConvexImpactResult m_impact;
	QuadTree*	m_qt = nullptr;
	bool m_use_quad = false;

	bool m_set_rotation = false;
	bool m_set_scale = false;
};

