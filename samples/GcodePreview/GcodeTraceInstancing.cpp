#include "GcodeTraceInstancing.hpp"

namespace Instance {
void Polyline::append_segment(const Segment& segment)
{
	if (segment.end_move_id - 1 < begin_move_id)
		begin_move_id = segment.end_move_id - 1;

	if (segment.end_move_id > end_move_id)
		end_move_id = segment.end_move_id;

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

GcodeTraceInstancing::GcodeTraceInstancing()
{
	reset();
}

void GcodeTraceInstancing::reset()
{
	m_move_type_visible.fill(false);
	m_role_visible.fill(false);
	m_view_type = ViewType::LINE_TYPE;

	m_layers.clear();
	m_layer_range = {};
	m_layer_scope = {};
	m_move_range = {};
	m_move_scope = {};

	for (auto& batch : m_lines_batches) {
		if (batch.instance_data_buffer)
			delete[] batch.instance_data_buffer;
	}
	m_lines_batches = {};

	m_dirty = false;
	m_valid = false;
}

void GcodeTraceInstancing::load(const GCodeProcessorResult& result)
{
	reset();
	parse_moves(result.moves);
	m_valid = true;
	emit loaded(m_lines_batches);

	// already send instance data to gpu.
	// release the instance data
	for (auto& batch : m_lines_batches) {
		delete[] batch.instance_data_buffer;
		batch.instance_data_buffer = nullptr;
	}
}

void GcodeTraceInstancing::set_layer_scope(std::array<int, 2> layer_scope)
{
	assert(layer_scope[1] >= layer_scope[0]);
	m_layer_scope = layer_scope;

	m_move_range[0] = m_layers[m_layer_scope[1]].begin_move_id;
	m_move_range[1] = m_layers[m_layer_scope[1]].end_move_id;
	m_move_scope = m_move_range;

	refresh();
}

void GcodeTraceInstancing::set_move_scope(std::array<int, 2> move_scope)
{
	assert(move_scope[1] >= move_scope[0]);
	m_move_scope[0] = std::max(move_scope[0], m_move_range[0]);
	m_move_scope[1] = std::min(move_scope[1], m_move_range[1]);

	refresh();
}

void GcodeTraceInstancing::set_visible(ExtrusionRole role_type, bool visible)
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

void GcodeTraceInstancing::parse_moves(std::vector<MoveVertex> moves)
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
			// TODO calc matrix
			Mat4 transform = Math::Translate(prev.position)
				* Math::Rotate(acos(Math::Dot(Vec3(1,0,0), Math::Normalize(curr.position - prev.position))), Vec3(0, 0, 1))
				* Math::Scale(curr.position - prev.position);
			segment.instance_data = new inst_data{ transform };
			segment.index_offset = 36;// segment.mesh->indices.size();
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
		batch.instance_data_buffer_size = 0;
		for (auto& polyline : batch.polylines) {
			batch.instance_data_buffer_size += polyline.segments.size();
		}
		batch.instance_data_buffer = new inst_data[batch.instance_data_buffer_size]{};

		int i = 0;
		for (auto& polyline : batch.polylines) {
			for (auto& seg : polyline.segments) {
				assert(seg.instance_data);
				batch.instance_data_buffer[i] = *seg.instance_data;
				delete seg.instance_data;
				seg.instance_data = nullptr;
				i++;
			}
		}
	}

	for (int role_type = ExtrusionRole::erNone + 1; role_type < ExtrusionRole::erCount; role_type++) {
		if (!m_lines_batches[role_type].empty()) {
			m_role_visible[role_type] = true;
		}
	}

	refresh();
}

void GcodeTraceInstancing::refresh()
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
}