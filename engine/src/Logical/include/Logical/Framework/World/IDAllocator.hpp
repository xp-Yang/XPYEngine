#ifndef IDAllocator_hpp
#define IDAllocator_hpp

template<typename T>
class IDAllocator {
public:
	IDAllocator() : id(++global_id) {}
	bool operator==(const IDAllocator& rhs) const {
		return id == rhs.id;
	}
	operator int() const { return id; }
	int id;

protected:
	static inline int global_id = 0;
};

class GObjectID : public IDAllocator<GObjectID> {
public:
    GObjectID() = default;
};

#endif
