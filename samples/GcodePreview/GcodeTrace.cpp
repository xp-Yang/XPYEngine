#include "GcodeTrace.hpp"

void Polyline::append_segment(const Segment& segment)
{
	if (segment.end_move_id - 1 < begin_move_id)
		begin_move_id = segment.end_move_id - 1;

	if (segment.end_move_id > end_move_id)
		end_move_id = segment.end_move_id;

	if (!segments.empty()) {
		auto& last_segment = segments.back();
		assert(last_segment.end_move_id == segment.end_move_id - 1);
		Vec3 last_segment_dir = last_segment.mesh->vertices[4].position - last_segment.mesh->vertices[0].position;
		bool is_turn_left = Math::Dot((segment.mesh->vertices[1].position - segment.mesh->vertices[0].position), last_segment_dir) >= 0;
		std::vector<int> indices = is_turn_left ? std::vector<int>{ 6, 10, 7, 5, 10, 6 } : std::vector<int>{ 4, 7, 8, 5, 4, 8 };
		// debug: turn off the corner faces filling
		// indices = { 0, 0, 0, 0, 0, 0 };
		last_segment.mesh->indices.reserve(last_segment.mesh->indices.size() + indices.size());
		last_segment.mesh->indices.insert(last_segment.mesh->indices.end(), indices.begin(), indices.end());
		last_segment.index_offset = last_segment.mesh->indices.size();
		index_offset += 6;
	}

	segments.push_back(segment);
	index_offset += segment.index_offset;
}

void LinesBatch::append_polyline(const Polyline& polyline)
{
	polylines.push_back(polyline);
}

int LinesBatch::calculate_index_offset_of(int move_id, bool begin) const
{
	// debug statistics
	//int total_segment_count = 0;
	//for (int i = 0; i < polylines.size(); i++) {
	//	const auto& polyline = polylines[i];
	//	for (int j = 0; j < polyline.segments.size(); j++) {
	//		total_segment_count++;
	//	}
	//}

	int index_offset = 0;
	for (int i = 0; i < polylines.size(); i++) {
		const auto& polyline = polylines[i];
		if (polyline.end_move_id <= move_id) {
			index_offset += polyline.index_offset;
			continue;
		}
		if (polyline.begin_move_id < move_id && move_id <= polyline.end_move_id) {
			for (int j = 0; j < polyline.segments.size(); j++) {
				const auto& segment = polyline.segments[j];
				if (segment.end_move_id <= move_id)
					index_offset += segment.index_offset;
				else
					return (begin || segment.end_move_id == polyline.end_move_id) ? index_offset : index_offset - 6;
			}
		}
	}
	return index_offset;
}

GcodeTrace::GcodeTrace()
{
	reset();
}

void GcodeTrace::reset()
{
	m_move_type_visible.fill(false);
	m_role_visible.fill(false);
	m_view_type = ViewType::LINE_TYPE;

	m_layers.clear();
	m_layer_range = {};
	m_layer_scope = {};
	m_move_range = {};
	m_move_scope = {};

	m_lines_batches = {};

	m_dirty = false;
	m_valid = false;
}

void GcodeTrace::load(const GCodeProcessorResult& result)
{
	reset();
	parse_moves(result.moves);
	m_valid = true;
	emit loaded(m_lines_batches);

	// already send vertices and indices data to gpu.
	// release the meshes
	for (auto& batch : m_lines_batches) {
		batch.merged_mesh.reset();
	}
}

void GcodeTrace::set_layer_scope(std::array<int, 2> layer_scope)
{
	assert(layer_scope[1] >= layer_scope[0]);
	m_layer_scope = layer_scope;

	m_move_range[0] = m_layers[m_layer_scope[1]].begin_move_id;
	m_move_range[1] = m_layers[m_layer_scope[1]].end_move_id;
	m_move_scope = m_move_range;

	refresh();
}

void GcodeTrace::set_move_scope(std::array<int, 2> move_scope)
{
	assert(move_scope[1] >= move_scope[0]);
	m_move_scope[0] = std::max(move_scope[0], m_move_range[0]);
	m_move_scope[1] = std::min(move_scope[1], m_move_range[1]);

	refresh();
}

void GcodeTrace::set_visible(ExtrusionRole role_type, bool visible)
{
	m_role_visible[role_type] = visible;
	if (!visible) {
		m_lines_batches[role_type].colored_indices_interval = {};
		m_lines_batches[role_type].colorless_indices_interval = {};
		m_dirty = true;
	}
	else {
		refresh();
	}
	//update the horizontal slider
}

std::shared_ptr<SimpleMesh> GcodeTrace::generate_cuboid_from_move(const MoveVertex& prev, const MoveVertex& curr)
{
	Vec3 up_dir = Vec3(0, 0, 1);
	Vec3 to_curr_dir = Math::Normalize(curr.position - prev.position);
	Vec3 left_dir = Math::Normalize(Math::Cross(up_dir, to_curr_dir));

	std::array<Vec3, 2> face_center_pos;
	Vec3 face_left_vec;
	Vec3 face_up_vec;
	float half_width = curr.width / 2.0f;
	float half_height = curr.height / 2.0f;
	face_center_pos[0] = prev.position;
	face_center_pos[1] = curr.position;
	face_left_vec = half_width * left_dir;
	face_up_vec = half_height * up_dir;

	return SimpleMesh::create_vertex_normal_cuboid_mesh(face_center_pos, face_left_vec, face_up_vec);
}

std::shared_ptr<SimpleMesh> GcodeTrace::generate_arc_from_move(const MoveVertex& prev, const MoveVertex& curr)
{
	size_t loop_num = curr.is_arc_move_with_interpolation_points() ? curr.interpolation_points.size() : 0;
	assert(loop_num > 0);
	Vec3 up_dir = Vec3(0, 0, 1);
	float half_width = curr.width / 2.0f;
	float half_height = curr.height / 2.0f;

	std::vector<Vec3> face_center_pos;
	std::vector<Vec3> face_left_vec;
	std::vector<Vec3> face_up_vec;
	face_center_pos.reserve(loop_num + 2);
	face_left_vec.reserve(loop_num + 2);
	face_up_vec.reserve(loop_num + 2);

	face_center_pos.push_back(prev.position);
	face_left_vec.push_back(half_width * Math::Normalize(Math::Cross(up_dir, Math::Normalize(curr.interpolation_points[0] - prev.position))));
	face_up_vec.push_back(half_height * up_dir);
	for (size_t i = 0; i < loop_num; i++) {
		const Vec3f& sub_prev_pos = (i == 0 ? prev.position : curr.interpolation_points[i - 1]);
		const Vec3f& sub_curr_pos = curr.interpolation_points[i];
		Vec3 to_curr_dir = Math::Normalize(sub_curr_pos - sub_prev_pos);
		Vec3 left_dir = Math::Normalize(Math::Cross(up_dir, to_curr_dir));
		face_center_pos.push_back(sub_curr_pos);
		// debug:
		//Vec3 to_next_dir = Vec3(0);
		//if (i == loop_num - 1)
		//	to_next_dir = Math::Normalize(curr.position - curr.interpolation_points[loop_num - 1]);
		//else
		//	to_next_dir = Math::Normalize(curr.interpolation_points[i + 1] - curr.interpolation_points[i]);
		//Vec3 corrected_left_dir = Math::Cross(to_curr_dir, to_next_dir).z >= 0 ? Math::Normalize(-to_curr_dir + to_next_dir) : -Math::Normalize(-to_curr_dir + to_next_dir);
		//float corrected_width = half_width / (Math::Dot(corrected_left_dir, left_dir) / (Math::Length(corrected_left_dir) * Math::Length(left_dir)));
		//face_left_vec.push_back(corrected_width * corrected_left_dir);
		face_left_vec.push_back(half_width * left_dir);
		face_up_vec.push_back(half_height * up_dir);
	}
	face_center_pos.push_back(curr.position);
	face_left_vec.push_back(half_width * Math::Normalize(Math::Cross(up_dir, Math::Normalize(curr.position - curr.interpolation_points[loop_num - 1]))));
	face_up_vec.push_back(half_height * up_dir);
	return SimpleMesh::create_vertex_normal_arc_mesh(face_center_pos, face_left_vec, face_up_vec);
}

void GcodeTrace::parse_moves(std::vector<MoveVertex> moves)
{
	ExtrusionRole prev_role = ExtrusionRole::erNone;
	for (size_t move_id = 1; move_id < moves.size(); move_id++) {
		const MoveVertex& prev = moves[move_id - 1];
		const MoveVertex& curr = moves[move_id];

		if (curr.type != EMoveType::Extrude) {
			prev_role = ExtrusionRole::erNone;
		}

		if (prev.position == curr.position) {
			// seam or spit or ...
			// skip segment generation for these moves
			continue;
		}

		if (curr.type == EMoveType::Extrude) {
			// parse layers
			float last_height = m_layers.empty() ? -FLT_MAX : m_layers.back().height;
			float curr_height = curr.position.z;
			if (last_height < curr_height)
				m_layers.emplace_back(curr.position.z, move_id - 1, move_id);
			else
				m_layers.back().end_move_id = move_id;

			// parse segment
			Segment segment;
			segment.end_move_id = move_id;
			segment.mesh = curr.is_arc_move_with_interpolation_points() ? generate_arc_from_move(prev, curr) : generate_cuboid_from_move(prev, curr);
			segment.index_offset = segment.mesh->indices.size();
			ExtrusionRole curr_role = curr.extrusion_role;
			if (prev_role != curr_role) {
				prev_role = curr_role;
				Polyline polyline;
				polyline.append_segment(segment);
				m_lines_batches[curr_role].append_polyline(polyline);
			}
			else {
				m_lines_batches[curr_role].polylines.back().append_segment(segment);
			}
		}
	}

	m_layer_range = { 0, (int)m_layers.size() - 1 };
	m_layer_scope = m_layer_range;
	m_move_range = { m_layers[m_layer_scope[1]].begin_move_id, m_layers[m_layer_scope[1]].end_move_id };
	m_move_scope = m_move_range;

	for (auto& batch : m_lines_batches) {
		std::vector<std::shared_ptr<SimpleMesh>> can_merge_meshes;
		int capacity = 0;
		for (auto& polyline : batch.polylines) {
			capacity += polyline.segments.size();
		}
		can_merge_meshes.reserve(capacity);
		for (auto& polyline : batch.polylines) {
			for (auto& seg : polyline.segments) {
				if (seg.mesh)
					can_merge_meshes.push_back(std::move(seg.mesh));
			}
		}
		if (!can_merge_meshes.empty()) {
			batch.merged_mesh = SimpleMesh::merge(can_merge_meshes);
		}
	}

	for (int role_type = ExtrusionRole::erNone + 1; role_type < ExtrusionRole::erCount; role_type++) {
		if (!m_lines_batches[role_type].empty()) {
			m_role_visible[role_type] = true;
		}
	}

	refresh();
}

void GcodeTrace::refresh()
{
	int begin_move_id = m_layers[m_layer_scope[0]].begin_move_id;
	int end_move_id = m_move_scope[1];
	assert(begin_move_id <= end_move_id);

	if (end_move_id == m_layers[m_layer_scope[1]].end_move_id) {
		for (int role_type = ExtrusionRole::erNone + 1; role_type < ExtrusionRole::erCount; role_type++) {
			auto& batch = m_lines_batches[role_type];
			if (batch.empty() || !m_role_visible[role_type])
				continue;

			int begin_index_offset = batch.calculate_index_offset_of(begin_move_id, true);
			int end_index_offset = batch.calculate_index_offset_of(end_move_id, false);
			batch.colored_indices_interval = std::make_pair(begin_index_offset, end_index_offset);
			batch.colorless_indices_interval = {};
		}
	}
	else {
		int last_layer_begin_move_id = m_layers[m_layer_scope[1]].begin_move_id;
		int last_layer_end_move_id = end_move_id;

		for (int role_type = ExtrusionRole::erNone + 1; role_type < ExtrusionRole::erCount; role_type++) {
			auto& batch = m_lines_batches[role_type];
			if (batch.empty() || !m_role_visible[role_type])
				continue;

			int begin_index_offset = batch.calculate_index_offset_of(last_layer_begin_move_id, true);
			int end_index_offset = batch.calculate_index_offset_of(last_layer_end_move_id, false);
			batch.colored_indices_interval = std::make_pair(begin_index_offset, end_index_offset);

			begin_index_offset = batch.calculate_index_offset_of(begin_move_id, true);
			end_index_offset = batch.calculate_index_offset_of(last_layer_begin_move_id, false);
			batch.colorless_indices_interval = std::make_pair(begin_index_offset, end_index_offset);
		}
	}

	m_dirty = true;
}
