#ifndef Serializer_hpp
#define Serializer_hpp

#include "Meta.hpp"
#include <json11.hpp>
#include <iostream>
#include <sstream>
#include <fstream>

namespace Meta {
namespace Serialization {

class Serializer {
public:
	static void dump_pretty_json(std::ostream& os, const json11::Json& j, int indent = 0)
	{
		const std::string pad(indent, ' ');
		if (j.is_object()) {
			os << "{\n";
			const auto& obj = j.object_items();
			size_t i = 0;
			for (const auto& kv : obj) {
				os << std::string(indent + 2, ' ')
					<< json11::Json(kv.first).dump()
					<< ": ";
				dump_pretty_json(os, kv.second, indent + 2);
				if (++i < obj.size()) os << ",";
				os << "\n";
			}
			os << pad << "}";
			return;
		}
		if (j.is_array()) {
			const auto& arr = j.array_items();
			if (arr.empty()) {
				os << "[]";
				return;
			}
			os << "[\n";
			for (size_t i = 0; i < arr.size(); i++) {
				os << std::string(indent + 2, ' ');
				dump_pretty_json(os, arr[i], indent + 2);
				if (i + 1 < arr.size()) os << ",";
				os << "\n";
			}
			os << pad << "]";
			return;
		}
		os << j.dump();
	}

	template<class T>
	static T& read(const Json& json, T& obj)
	{
		Instance refl_obj = obj;
		read_internal(json, refl_obj);
		return refl_obj.getValue<T&>();
	}

	template<class T>
	static Json write(const T& obj) {
		Instance refl_obj = obj;
		Json ret = write_internal(refl_obj);
		return ret;
	}

	template<class T>
	static void loadFromJsonFile(const std::string& filepath, T& obj) {
		std::ifstream fin(filepath);
		if (!fin) {
			assert(false);
			return;
		}
		std::stringstream buffer;
		buffer << fin.rdbuf();
		std::string json_text(buffer.str());

		std::string error;
		auto&& json = Json::parse(json_text, error);
		if (!error.empty())
		{
			assert(false);
			return;
		}
		Serializer::read(json, obj);
	}

	template<class T>
	static void saveToJsonFile(const std::string& filepath, const T& obj) {
		std::ofstream fout(filepath);
		if (!fout) {
			assert(false);
			return;
		}
		Json j = write(obj);
		dump_pretty_json(fout, json11::Json(j), 0);
		fout << "\n";
		fout.flush();
	}

	//template<class T>
	//static void output_test(const T& obj) {
	//	ReflectionInstance test_refl_obj = ReflectionInstance(obj);
	//	test_output(test_refl_obj);
	//}

protected:
	static Json write_value_by_type(Instance inst) {
		if (inst.isType<char>()) return Json(inst.getValue<char>());
		if (inst.isType<int>()) return Json(inst.getValue<int>());
		if (inst.isType<unsigned int>()) return Json((int)inst.getValue<unsigned int>());
		if (inst.isType<float>()) return Json(inst.getValue<float>());
		if (inst.isType<double>()) return Json(inst.getValue<double>());
		if (inst.isType<bool>()) return Json(inst.getValue<bool>());
		if (inst.isType<std::string>()) return Json(inst.getValue<std::string>());
		return write_internal(inst);
	}

	static void read_value_by_type(const Json& json, Instance inst) {
		if (inst.isType<char>()) {
			if (json.is_number()) inst.setValue((char)json.number_value());
			return;
		}
		if (inst.isType<int>()) {
			if (json.is_number()) inst.setValue((int)json.number_value());
			return;
		}
		if (inst.isType<unsigned int>()) {
			if (json.is_number()) inst.setValue((unsigned int)json.number_value());
			return;
		}
		if (inst.isType<float>()) {
			if (json.is_number()) inst.setValue((float)json.number_value());
			return;
		}
		if (inst.isType<double>()) {
			if (json.is_number()) inst.setValue((double)json.number_value());
			return;
		}
		if (inst.isType<bool>()) {
			if (json.is_bool()) inst.setValue(json.bool_value());
			return;
		}
		if (inst.isType<std::string>()) {
			if (json.is_string()) inst.setValue(json.string_value());
			return;
		}
		read_internal(json, inst);
	}

	static void read_internal(const Json& json, Instance& inst) {
		MetaType meta_type = inst.metaType();
		for (int i = 0; i < meta_type.propertyCount(); i++) {
			auto& prop = meta_type.property(i);
			std::string type_name = prop.type_name;
			std::string name = prop.name;
			Instance prop_value = prop.getValue(inst);
			const Json& field_json = json[name];
			if ((prop.type & Property::Type::SequenceContainer) == Property::Type::SequenceContainer &&
				prop.sequence_resize && prop.sequence_element_ptr && !prop.value_type_name.empty()) {
				if (!field_json.is_array()) continue;
				const auto& arr = field_json.array_items();
				prop.sequence_resize(prop_value, arr.size());
				for (size_t idx = 0; idx < arr.size(); idx++) {
					Instance elem_value = prop.sequence_element_ptr(prop_value, idx);
					read_value_by_type(arr[idx], elem_value);
				}
				continue;
			}
			read_value_by_type(field_json, prop_value);
		}
	}

	static Json write_internal(const Instance& inst) {
		Json::object json_obj;
		MetaType meta_type = inst.metaType();
		for (int i = 0; i < meta_type.propertyCount(); i++) {
			auto& prop = meta_type.property(i);
			std::string type_name = prop.type_name;
			std::string name = prop.name;
			Instance prop_value = prop.getValue(inst);
			if ((prop.type & Property::Type::SequenceContainer) == Property::Type::SequenceContainer &&
				prop.sequence_size && prop.sequence_element_ptr && !prop.value_type_name.empty()) {
				Json::array arr;
				size_t n = prop.sequence_size(prop_value);
				arr.reserve(n);
				for (size_t idx = 0; idx < n; idx++) {
					Instance elem_value = prop.sequence_element_ptr(prop_value, idx);
					arr.push_back(write_value_by_type(elem_value));
				}
				json_obj.insert_or_assign(name, Json(arr));
				continue;
			}
			json_obj.insert_or_assign(name, write_value_by_type(prop_value));
		}
		return Json(json_obj);
	}

private:
	Serializer() = default;
};

}}

#endif
