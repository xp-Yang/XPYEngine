#ifndef GcodeTrace_hpp
#define GcodeTrace_hpp

#include "GcodeImporter/GcodeResultData.hpp"
#include "SimpleMesh.hpp"

enum LineType : unsigned int {
	// EXTRUDE TYPE
	NONE = 0,
	INNER_WALL					= 1 << 0,
	OUTER_WALL                  = 1 << 1,
	BRIDGE_WALL					= 1 << 2,
	SPARSE_INFILL				= 1 << 3,
	INTERNAL_SOLID_INFILL		= 1 << 4,
	TOP_SURFACE			        = 1 << 5,
	BOTTOM_SURFACE				= 1 << 6,
	IRONING						= 1 << 7,
	BRIDGE						= 1 << 8,
	SKIRT						= 1 << 9,
	BRIM						= 1 << 10,
	SUPPORT						= 1 << 11,
	SUPPORT_INTERFACE			= 1 << 12,
	PRIME_TOWER					= 1 << 13,
	CUSTOM						= 1 << 14,
	MIXED						= 1 << 15,

	// MOVE TYPE
	// EXTRUDE
	TRAVEL						= 1 << 16,
	RETRACT						= 1 << 17,
	UNRETRACT					= 1 << 18,
	WIPE						= 1 << 19,
	SEAM						= 1 << 20,
	COUNT						= 1 << 21,
};

inline const std::array<std::string, ExtrusionRole::erCount> role_labels = {
    ("Undefined"),
    ("Inner wall"),
    ("Outer wall"),
    ("Overhang wall"),
    ("Sparse infill"),
    ("Internal solid infill"),
    ("Top surface"),
    ("Bottom surface"),
    ("Ironing"),
    ("Bridge"),
    ("Gap infill"),
    ("Skirt"),
    ("Brim"),
    ("Support"),
    ("Support interface"),
    ("Support transition"),
    ("Prime tower"),
    ("Custom"),
    ("Multiple"),
};

inline const std::vector<Color4> Extrusion_Role_Colors{
	{ 0.90f, 0.70f, 0.70f, 1.0f },   // erNone
	{ 1.00f, 0.90f, 0.30f, 1.0f },   // erPerimeter
	{ 1.00f, 0.49f, 0.22f, 1.0f },   // erExternalPerimeter
	{ 0.12f, 0.12f, 1.00f, 1.0f },   // erOverhangPerimeter
	{ 0.69f, 0.19f, 0.16f, 1.0f },   // erInternalInfill
	{ 0.59f, 0.33f, 0.80f, 1.0f },   // erSolidInfill
	{ 0.94f, 0.25f, 0.25f, 1.0f },   // erTopSolidInfill
	{ 0.40f, 0.36f, 0.78f, 1.0f },   // erBottomSurface
	{ 1.00f, 0.55f, 0.41f, 1.0f },   // erIroning
	{ 0.30f, 0.50f, 0.73f, 1.0f },   // erBridgeInfill
	{ 1.00f, 1.00f, 1.00f, 1.0f },   // erGapFill
	{ 0.00f, 0.53f, 0.43f, 1.0f },   // erSkirt
	{ 0.00f, 0.23f, 0.43f, 1.0f },   // erBrim
	{ 0.00f, 1.00f, 0.00f, 1.0f },   // erSupportMaterial
	{ 0.00f, 0.50f, 0.00f, 1.0f },   // erSupportMaterialInterface
	{ 0.00f, 0.25f, 0.00f, 1.0f },   // erSupportTransition
	{ 0.70f, 0.89f, 0.67f, 1.0f },   // erWipeTower
	{ 0.37f, 0.82f, 0.58f, 1.0f },   // erCustom
	{ 0.37f, 0.82f, 0.58f, 1.0f },   // erMixed
};

inline const Color4 Silent_Color = { 0.12f, 0.12f, 0.12f, 1.0f };

enum class ViewType {
	LINE_TYPE,
	FILAMENT,
	SPEED,
	LAYER_HEIGHT,
	LAYER_WIDTH,
	TEMPERATURE,
	COUNT
};

struct Layer {
	Layer(float height_, int begin_move_id_, int end_move_id_)
		: height(height_)
		, begin_move_id(begin_move_id_)
		, end_move_id(end_move_id_)
	{}
	float height;
	int begin_move_id;
	int end_move_id;
};

// A Segment corresponds to one move
// For non-arc, a segment corresponds to a cuboid + corner face filling
// For an arc, a segment corresponds to an entire arc
struct Segment {
	// begin_move_id == end_move_id - 1
	//int begin_move_id;
	int end_move_id;
	// persist index_offset because the mesh will be released after send its data to gpu.
	int index_offset = 0;

	std::shared_ptr<SimpleMesh> mesh;
};

// Polyline is a set of continuous Segment
struct Polyline {
	int begin_move_id = INT_MAX;
	int end_move_id = -INT_MAX;
	int index_offset = 0;

	std::vector<Segment> segments;

	void append_segment(const Segment& segment);
};

// LinesBatch is a set of Polyline that can be rendered in one batch, i.e. a set of Polyline with the same color.
struct LinesBatch {
	std::vector<Polyline> polylines;
	std::pair<int, int> colorless_indices_interval;
	std::pair<int, int> colored_indices_interval;

	std::shared_ptr<SimpleMesh> merged_mesh;

	bool empty() const { return polylines.empty(); }
	void append_polyline(const Polyline& polyline);
	int calculate_index_offset_of(int move_id, bool begin) const;
};

class GcodeTrace {
public:
	GcodeTrace();
	//GcodeTrace(const GcodeTrace&) = delete;
	//GcodeTrace(GcodeTrace&&) = delete;
	//GcodeTrace& operator=(const GcodeTrace&) = delete;
	//GcodeTrace& operator=(GcodeTrace&&) = delete;

	void load(const GCodeProcessorResult& result);

	const std::array<LinesBatch, ExtrusionRole::erCount>& linesBatches() const { return m_lines_batches; }
	std::pair<int, int> colorless_indices_interval(ExtrusionRole role_type) const { return m_lines_batches[role_type].colorless_indices_interval; }
	std::pair<int, int> colored_indices_interval(ExtrusionRole role_type) const { return m_lines_batches[role_type].colored_indices_interval; }

	void set_layer_scope(std::array<int, 2> layer_scope);
	void set_move_scope(std::array<int, 2> move_scope);

	const std::array<int, 2>& get_layer_range() const { return m_layer_range; }
	const std::array<int, 2>& get_layer_scope() const { return m_layer_scope; }
	const std::array<int, 2>& get_move_range() const { return m_move_range; }
	const std::array<int, 2>& get_move_scope() const { return m_move_scope; }

	const std::vector<Layer>& get_layers() const { return m_layers; }

	void set_visible(ExtrusionRole role_type, bool visible);
	bool is_visible(ExtrusionRole role_type) const { return m_role_visible[role_type]; }

	bool dirty() const { return m_dirty; }
	void setDirty(bool dirty) { m_dirty = dirty; }

	bool valid() const { return m_valid; }

signals:
	Signal<std::array<LinesBatch, ExtrusionRole::erCount>> loaded;

protected:
	void reset();
	void parse_moves(std::vector<MoveVertex> moves);
	std::shared_ptr<SimpleMesh> generate_cuboid_from_move(const MoveVertex& prev, const MoveVertex& curr);
	std::shared_ptr<SimpleMesh> generate_arc_from_move(const MoveVertex& prev, const MoveVertex& curr);
	void refresh();

private:
	std::vector<Layer> m_layers;

	std::array<int, 2> m_layer_range;
	std::array<int, 2> m_layer_scope;
	std::array<int, 2> m_move_range;
	std::array<int, 2> m_move_scope;

	std::array<bool, EMoveType::Count> m_move_type_visible = {};
	std::array<bool, ExtrusionRole::erCount> m_role_visible = {};
	ViewType m_view_type;

	std::array<LinesBatch, ExtrusionRole::erCount> m_lines_batches = {};

	bool m_dirty = false;

	bool m_valid = false;
};

#endif // !#define GcodeTrace_hpp
