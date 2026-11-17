#include "EnumCapability.h"

EnumCapability::EnumCapability(const String& name, uint32_t initialValue,
                               ChangeCallback callback, void* context)
    : Capability(name, initialValue, callback, context) {}

EnumCapability::EnumCapability(const String& name, uint32_t initialValue,
                               const std::vector<String>& enumOptions,
                               ChangeCallback callback, void* context)
    : Capability(name, initialValue, callback, context), values(enumOptions) {}

bool EnumCapability::setOptions(const std::vector<String>& options) {
    if (options.empty()) return false;

    values = options;
    if (value >= values.size()) {
        value = 0;
    }
    return true;
}

bool EnumCapability::validate(uint32_t candidate) const {
    return candidate < values.size();
}

const std::vector<String>* EnumCapability::enumOptions() const {
    return &values;
}

const char* EnumCapability::typeName() const {
    return "enum";
}

void EnumCapability::writeParams(JsonDocument& document) const {
    Capability::writeParams(document);

    JsonArray optionsArray = document["options"].to<JsonArray>();
    for (const String& option : values) {
        optionsArray.add(option);
    }
}
