#ifndef IDAllocator_hpp
#define IDAllocator_hpp

#include <functional>

namespace Meta {
namespace Registration {
void allMetaRegister();
}
}

// --- Legacy design (kept for reference) ---
// template<typename T>
// class IDAllocator {
// public:
// 	IDAllocator() : id(++global_id) {}
// 	bool operator==(const IDAllocator& rhs) const {
// 		return id == rhs.id;
// 	}
// 	operator int() const { return id; }
// 	int id;
//
// protected:
// 	static inline int global_id = 0;
// };
//
// class GObjectID : public IDAllocator<GObjectID> {
// public:
//     GObjectID() = default;
// };

class GObjectID {
public:
	static constexpr int Invalid = 0;

	constexpr GObjectID() = default;
	constexpr explicit GObjectID(int id) : m_id(id) {}

	static GObjectID next() { return GObjectID(++s_global_id); }
	static GObjectID restore(int id) {
		if (id > s_global_id)
			s_global_id = id;
		return GObjectID(id);
	}
	static void resetCounter(int value = 0) { s_global_id = value; }

	int value() const { return m_id; }
	bool isValid() const { return m_id != Invalid; }
	explicit operator bool() const { return isValid(); }

	bool operator==(const GObjectID& rhs) const { return m_id == rhs.m_id; }
	bool operator!=(const GObjectID& rhs) const { return m_id != rhs.m_id; }
	bool operator<(const GObjectID& rhs) const { return m_id < rhs.m_id; }

private:
	int m_id{ Invalid };
	static inline int s_global_id = 0;

	friend void Meta::Registration::allMetaRegister();
};

namespace std {
template<>
struct hash<GObjectID> {
	size_t operator()(const GObjectID& id) const noexcept {
		return hash<int>()(id.value());
	}
};
}

#endif
