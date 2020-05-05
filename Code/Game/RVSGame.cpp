#include "Game/RVSGame.hpp"
#include "Engine/Core/RNG.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/RenderContext.hpp"
#include "Engine/Develop/DebugRenderer.hpp"
//#include "Engine/Core/WindowContext.hpp"
#include "Game/Game.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Event/EventSystem.hpp"

QuadTree::~QuadTree()
{
	delete m_sub[0];
	delete m_sub[1];
	delete m_sub[2];
	delete m_sub[3];
}

void QuadTree::build_tree(size_t depth)
{
	if (m_zones.size() <= QUAD_ZONE_LIMIT || depth >= MAX_DEPTH)
		return;
	const Vec2 center = m_box.GetCenter();
	// <<  <>  ><  >>
	// 0   1    2   3
	// III II  IV   I
	m_sub[0] = new QuadTree(AABB2{m_box.Min, center});
	m_sub[1] = new QuadTree(AABB2{m_box.Min.x, center.y, center.x, m_box.Max.y});
	m_sub[2] = new QuadTree(AABB2{center.x, m_box.Min.y, m_box.Max.x, center.y});
	m_sub[3] = new QuadTree(AABB2{center, m_box.Max});

	for(auto& each : m_zones) {
		for (size_t i = 0; i < 4; ++i) {
			if (each->m_poly.is_overlapping_box(m_sub[i]->m_box)) {
				m_sub[i]->m_zones.emplace_back(each);
			}
		}
	}
	for (size_t i = 0; i < 4; ++i) {
		m_sub[i]->build_tree(depth + 1);
	}
}

void QuadTree::display() const
{
	std::vector<Vertex_PCU> vert;
	Vec2 tl = m_box.GetTopLeft();
	Vec2 bl = m_box.GetBottomLeft();
	Vec2 br = m_box.GetBottomRight();
	Vec2 tr = m_box.GetTopRight();
	const Rgba& color = m_checked ? Rgba(0,.5f,0,0.3f) : Rgba::TRANSPARENT_BLACK;
	AddVerticesOfLine2D(vert, tl, tr, 0.005f, Rgba::GRAY);
	AddVerticesOfLine2D(vert, tr, br, 0.005f, Rgba::GRAY);
	AddVerticesOfLine2D(vert, br, bl, 0.005f, Rgba::GRAY);
	AddVerticesOfLine2D(vert, bl, tl, 0.005f, Rgba::GRAY);
	AddVerticesOfAABB2D(vert, m_box, color);
	g_theRenderer->DrawVertexArray(vert.size(), vert);
	
	if (m_sub[0]) {
		for (size_t i = 0; i < 4; ++i) {
			m_sub[i]->display();
		}
	}
	
}

ConvexImpactResult QuadTree::raycast_by(const Ray2& ray, bool set_flag)
{
	ConvexImpactResult result;
	const float hit = ray.RaycastToAABB2(m_box);
	if (hit >= 0) {
		if (m_sub[0]){
			for (size_t i = 0; i < 4; ++i) {
				ConvexImpactResult subr = m_sub[i]->raycast_by(ray, set_flag);
				if (subr.hit && subr.k < result.k) {
					result = subr;
				}
			}
		} else {
			for (auto& each : m_zones) {
				ConvexImpactResult zoner = each->m_hull.raycast_by(ray);
				if (zoner.hit && zoner.k < result.k) {
					result = zoner;
				}
			}
			//for debug
			if (set_flag) {
				m_checked = true;
			}
		}
	}
	return result;
}

void QuadTree::reset_tree_flag()
{
	m_checked = false;
	if (m_sub[0]) {
		for (size_t i = 0; i < 4; ++i) {
				m_sub[i]->reset_tree_flag();
		}
	}
}

RVSGame::~RVSGame()
{
	delete m_qt;
}

void RVSGame::Startup(size_t numPolys)
{
	constexpr float radius_min = 0.05f;
	constexpr float radius_max = 0.1f;
	for (size_t i = 0; i< numPolys;++i) {
		m_zones.emplace_back(Zone());
		auto& zone = m_zones[i];
		zone.m_poly = ConvexPoly::GetRandomPoly(g_rng.GetFloatInRange(radius_min, radius_max));
		zone.m_position = Vec2 {g_rng.GetFloatInRange(-1,1), g_rng.GetFloatInRange(-1,1)};
		zone.m_poly.move_by(zone.m_position);
		zone.m_hull = ConvexHull2(zone.m_poly);
	}

	g_Event->SubscribeEventCallback("ghcs-load", this, &RVSGame::load_ghcs);
	g_Event->SubscribeEventCallback("ghcs-save", this, &RVSGame::save_ghcs);

	_update_quad_tree();
}

void RVSGame::_update_quad_tree()
{

	delete m_qt;
	m_qt = new QuadTree(AABB2(-1,-1,1,1));
	for(auto& each:m_zones) {
		m_qt->m_zones.emplace_back(&each);
	}
	m_qt->build_tree();
}

Zone* RVSGame::get_first_zone_include(const Vec2& position)
{
	for (auto& each: m_zones) {
		if (each.m_hull.is_inside(position)) {
			return &each;
		}
	}
	return nullptr;
}

void RVSGame::BeginFrame()
{
	m_qt->reset_tree_flag();
}

void RVSGame::Update(float deltaSeconds)
{
	//invisible raycast for 1ms
	double max_time = GetCurrentTimeSeconds();
	Vec2 start, end;
	max_time += 0.001;
	size_t count = 0;
	while(GetCurrentTimeSeconds() < max_time) {
		start.x = g_rng.GetFloatInRange(-1,1);
		start.y = g_rng.GetFloatInRange(-1,1);
		end.x = g_rng.GetFloatInRange(-1,1);
		end.y = g_rng.GetFloatInRange(-1,1);
		raycast_to_all(Ray2::FromPoint(start, end));
		++count;
	}
	DebugRenderer::Log(Stringf("%6u ray in 1ms", count), 0, Rgba::RED);
	m_impact = ConvexImpactResult();
	if(m_raycast_on) {
		Ray2 ray = Ray2::FromPoint(m_mouse_start, m_mouse_end);
		ConvexImpactResult impact;
		if (m_use_quad) {
				m_impact = m_qt->raycast_by(ray, true);
		} else {
			for (auto& each : m_zones) {
				impact = each.m_hull.raycast_by(ray);
				if (impact.hit && impact.k < m_impact.k) {
					m_impact = impact;
					//m_impact.pos += each.m_position;
				}
			}
		}
	}
}

void RVSGame::Render() const
{
	std::vector<Vertex_PCU> verts;
	for (auto each:m_zones) {
		auto& poly = each.m_poly;
		auto& position = each.m_position;
		for (size_t i = 1; i < poly.m_points.size(); ++ i) {
			AddVerticesOfLine2D(verts, poly.m_points[i - 1], poly.m_points[i], 0.003f, Rgba::TEAL);
		}
		AddVerticesOfLine2D(verts, poly.m_points[poly.m_points.size() - 1], poly.m_points[0], 0.003f, Rgba::TEAL);
	}
	g_theRenderer->DrawVertexArray(verts.size(), verts);


	/*verts.clear();
	ConvexHull2 hull = ConvexHull2(m_zones[0].m_poly);
	for(size_t i = 0; i < hull.m_edges.size(); ++i) {
		if(i + 1 >= hull.m_edges.size()) {
			Vec2 center = m_zones[0].m_poly.m_points[i] + m_zones[0].m_poly.m_points[0];
			center *= 0.5f;
			center += m_zones[0].m_position;
			Vec2 end = center + hull.m_edges[i].Normal * 0.05f;
			AddVerticesOfLine2D(verts, center, end, 0.002f, Rgba::RED);
		} else {
			Vec2 center = m_zones[0].m_poly.m_points[i] + m_zones[0].m_poly.m_points[i + 1];
			center *= 0.5f;
			center += m_zones[0].m_position;
			Vec2 end = center + hull.m_edges[i].Normal * 0.05f;
			AddVerticesOfLine2D(verts, center, end, 0.002f, Rgba::RED);
		}
	}

	g_theRenderer->DrawVertexArray(verts.size(), verts);
	*/

	if (m_use_quad) {
		m_qt->display();
	}
	
	if (m_raycast_on) {
		static Vec2 SCREEN_SIZE = Vec2(g_theWindow->GetClientResolution());
		DebugRenderer::Log(Stringf("%g, %g == %g, %g", m_mouse_start.x, m_mouse_start.y, m_mouse_end.x, m_mouse_end.y), 0, 
			Rgba::BLACK);
		verts.clear();

		// const Plane2& plane = m_zones[0].m_hull.m_edges[0];
		// Ray2 ray = Ray2::FromPoint(m_mouse_start, m_mouse_end);
		// float k = ray.RaycastToPlane2(plane);
		// Vec2 point_hit = ray.GetPointAt(k);
		// DebugRenderer::DrawPoint2D((point_hit +
		// 	Vec2::ONE)*0.5f * SCREEN_SIZE, 10.f, 0, Rgba::TEAL);
		AddVerticesOfLine2D(verts, m_mouse_start, m_mouse_end, 0.0035f, m_impact.hit?
			Rgba::GREEN:
			Rgba::RED);
		g_theRenderer->DrawVertexArray(verts.size(), verts);

		
		if (m_impact.hit) {
			DebugRenderer::DrawPoint2D((m_impact.pos+Vec2::ONE) * 0.5f * SCREEN_SIZE, 10.f, 0, Rgba::LIME);
			DebugRenderer::DrawLine2D((m_impact.pos + Vec2::ONE) * 0.5f * SCREEN_SIZE, (m_impact.pos + m_impact.normal * 0.05f + Vec2::ONE) * 0.5f * SCREEN_SIZE, 2.f, 0, Rgba::CYAN);
		}
		
	}
}

void RVSGame::EndFrame()
{
}

void RVSGame::Shutdown()
{
	g_Event->UnsubscribeEventCallback("ghcs-load", this, &RVSGame::load_ghcs);
	g_Event->UnsubscribeEventCallback("ghcs-save", this, &RVSGame::save_ghcs);
}

void RVSGame::mouse_down(const Vec2& mouse_pos)
{
	m_mouse_start = mouse_pos;
}

void RVSGame::mouse_up(const Vec2& mouse_pos)
{
	m_mouse_end = mouse_pos;
	m_raycast_on = true;
}

void RVSGame::wheel_up(const Vec2& mouse_pos)
{
	if (!m_set_rotation && !m_set_scale) {
		return;
	}
	Zone* overlapped_zone = get_first_zone_include(mouse_pos);
	if (overlapped_zone) {
		if (m_set_scale) {
			overlapped_zone->scale(0.1f, mouse_pos);
		}
		if (m_set_rotation) {
			overlapped_zone->rotate(10.f, mouse_pos);
		}
		_update_quad_tree();
	}
}

void RVSGame::wheel_down(const Vec2& mouse_pos)
{
	if (!m_set_rotation && !m_set_scale) {
		return;
	}
	Zone* overlapped_zone = get_first_zone_include(mouse_pos);
	if (overlapped_zone) {
		if (m_set_scale) {
			overlapped_zone->scale(-0.1f, mouse_pos);
		}
		if (m_set_rotation) {
			overlapped_zone->rotate(-10.f, mouse_pos);
		}
		_update_quad_tree();
	}
}

#include "Game/ghcs.hpp"
#include "Game/Game.hpp"
extern Game* g_game;
bool RVSGame::load_ghcs(NamedStrings& param)
{
	constexpr int buffer_size = 10485760;
	std::string path = param.GetString("path", "Data/Test.ghcs");
	byte* buffer = new byte[buffer_size];
	LoadFileToBuffer(buffer, buffer_size, path.c_str());
	buffer_reader reader(buffer, buffer_size);
	ghcs_header header = parse_ghcs_header(reader);
	if (header.is_big_endian) {
		reader.m_reverse = true;
	}

	char fcc[4];
	while (true) {
		if (!reader.next_n_byte((byte*)fcc, 4)){
			break;
		}
		if (fcc[1] == 'T') {
			//toc
			break;
		}
		if (fcc[1] == 'C') {
			byte type = reader.next_basic<byte>();
			byte endi = reader.next_basic<byte>();
			uint32 size = reader.next_basic<uint32>();
			if (type == ghcs_ConvexPolysChunk) {
				std::vector<Zone> new_zones;
				m_zones = parse_convex_poly_chunk(reader);
				//m_zones = new_zones;
				_update_quad_tree();
				g_game->m_num_zone = m_zones.size();
			} else {
				reader.m_ptr += size;
			}
		}
	}


	return true;
}

bool RVSGame::save_ghcs(NamedStrings& param)
{
	std::string path = param.GetString("path", "Data/Test.ghcs");
	buffer_writer writer;
	ghcs_header h;
	h.is_big_endian = false;
	h.major_version  = 1;
	h.minor_version = 0;
	h.toc_offset = 0;
	write_ghcs_header(writer, &h);
	write_convex_poly_chunk(writer, m_zones);
	FILE* fp;
	fopen_s(&fp, path.c_str(), "wb");
	fwrite(writer.m_bytes.data(), 1, writer.m_bytes.size(), fp);
	fclose(fp);
	return true;
}

void RVSGame::raycast_to_all(const Ray2& ray)
{
	if (!m_use_quad) {
		for (auto& each : m_zones) {
			(void)(each.m_hull.raycast_by(ray));
		}
		return;
	}
	(void)(m_qt->raycast_by(ray));
}
