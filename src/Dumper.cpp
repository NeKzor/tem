/*
 * Copyright (c) 2022 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "Dumper.hpp"
#include "Console.hpp"
#include "Offsets.hpp"
#include "SDK.hpp"
#include "TEM.hpp"
#include "lib/json/json.hpp"
#include <format>
#include <fstream>
#include <set>

auto dump_engine() -> void
{
    auto g_Names = reinterpret_cast<TArray<FNameEntry*>*>(Offsets::g_Names);
    console->Println("[dumper] g_Names: 0x{:04x} (size = {})", uintptr_t(g_Names), g_Names->size);

    auto names = g_Names->data;
    std::ofstream name_stream("tron_evolution_names_dump.txt");

    name_stream << "// NOTE: Index is automatically right-shifted by one!\n";

    for (auto i = 0u; i < g_Names->size; ++i) {
        auto item = names[i];

        if (item && item->index == i << 1 && item->name) {
            name_stream << std::format("{} // 0x{:x}\n", item->name, item->index >> 1);
        }
    }

    auto g_Objects = reinterpret_cast<TArray<UObject*>*>(Offsets::g_Objects);
    console->Println("[dumper] g_Objects: 0x{:04x} (size = {})", uintptr_t(g_Objects), g_Objects->size);

    auto objects = g_Objects->data;
    std::ofstream object_stream("trom_evolution_objects_dump.txt");

    for (auto i = 0u; i < g_Objects->size; ++i) {
        auto item = objects[i];
        if (!item || !item->name.index) {
            continue;
        }

        auto base_name = std::string(names[item->name.index] ? names[item->name.index]->name : "");

        auto outer_name = std::string();
        auto outer = item->outer_object;
        while (outer) {
            outer_name
                = std::string(names[outer->name.index] ? names[outer->name.index]->name : "") + "::" + outer_name;
            outer = outer->outer_object;
        }

        auto class_name = std::string(item->class_object && names[item->class_object->name.index]
                ? names[item->class_object->name.index]->name
                : "");

        auto dump = std::format("{}{}({}) // 0x{:x}\n", outer_name, base_name, class_name, uintptr_t(item));

        object_stream << dump;
    }
}

// TODO: figure this out
auto dump_engine_to_markdown() -> void
{
    auto g_Names = reinterpret_cast<TArray<FNameEntry*>*>(Offsets::g_Names);
    auto names = g_Names->data;

    auto g_Objects = reinterpret_cast<TArray<UObject*>*>(Offsets::g_Objects);
    auto objects = g_Objects->data;

    std::ofstream classes_stream("trom_evolution_classes_dump.md");
    classes_stream << "# Tron: Evolution Classes" << std::endl << std::endl;

    auto unique_classes = std::set<UClass*>();

    for (auto i = 0u; i < g_Objects->size; ++i) {
        auto item = objects[i];

        if (!item || !item->name.index) {
            continue;
        }

        auto class_object = item->class_object;
        if (!class_object || unique_classes.contains(class_object)) {
            continue;
        }

        unique_classes.emplace(class_object);

        auto class_name = std::string(names[class_object->name.index] ? names[class_object->name.index]->name : "");
        auto outer_name = class_object->outer_object && names[class_object->outer_object->name.index]
            ? names[class_object->outer_object->name.index]->name
            : "";

        classes_stream << "## " << class_name << std::endl << std::endl;
        classes_stream << "Package: " << outer_name << std::endl << std::endl;
        classes_stream << "Size: 0x" << std::hex << class_object->property_size << " bytes" << std::endl;

        auto child_field = class_object->children;
        if (child_field) {
            classes_stream << std::endl;
            classes_stream << "### Fields" << std::endl << std::endl;
            classes_stream << "|Field|Type|Size|Offset|" << std::endl;
            classes_stream << "|---|---|---|---|" << std::endl;

            auto has_functions = false;

            while (child_field) {
                auto child_name = names[child_field->name.index];
                auto type_name = child_field->class_object && names[child_field->class_object->name.index]
                    ? names[child_field->class_object->name.index]->name
                    : "unk";

                if (strstr(type_name, "Property")) {
                    auto type_object = child_field->as<UProperty>();
                    classes_stream << "|" << (child_name ? child_name->name : "unk") << "|" << type_name << "|0x"
                                   << std::hex << type_object->property_size << "|0x" << std::hex << type_object->offset
                                   << "|" << std::endl;
                } else if (strcmp(type_name, "Function") != 0) {
                    classes_stream << "|" << (child_name ? child_name->name : "unk") << "|" << type_name << "|-|-|"
                                   << std::endl;
                } else {
                    has_functions = true;
                }

                child_field = child_field->next;
            }

            if (has_functions) {
                classes_stream << std::endl << "### Functions" << std::endl << std::endl;
                classes_stream << "|Signature|Params|Params Sizes|" << std::endl;
                classes_stream << "|---|---|---|" << std::endl;

                child_field = class_object->children;
                while (child_field) {
                    auto type_name = child_field->class_object && names[child_field->class_object->name.index]
                        ? names[child_field->class_object->name.index]->name
                        : "unk";

                    if (strcmp(type_name, "Function") == 0) {
                        auto child_name = names[child_field->name.index];
                        auto type_object = child_field->as<UFunction>();

                        /*auto child_child_field = type_object->children;
                        if (child_child_field) {
                            auto return_value_name = names[child_child_field->name.index];
                            auto child_type_name
                                = child_child_field->class_object && names[child_child_field->class_object->name.index]
                                ? names[child_child_field->class_object->name.index]->name
                                : "unk";

                            classes_stream << "|" << (return_value_name ? return_value_name->name : "unk") << " "
                                           << (child_name ? child_name->name : "unk") << "(";

                            child_child_field = child_child_field->next;

                            while (child_child_field) {
                                auto child_child_name = names[child_child_field->name.index];
                                auto child_type_name = child_child_field->class_object
                                        && names[child_child_field->class_object->name.index]
                                    ? names[child_child_field->class_object->name.index]->name
                                    : "unk";

                                classes_stream << child_type_name << " "
                                               << (child_child_name ? child_child_name->name : "unk");

                                child_child_field = child_child_field->next;

                                if (child_child_field) {
                                    classes_stream << ", ";
                                }
                            }
                            classes_stream << ")|";
                        } else {
                            classes_stream << "|" << (child_name ? child_name->name : "unk")  << "()|";
                        }*/
                        classes_stream << "|" << (child_name ? child_name->name : "unk") << "|";

                        classes_stream << type_object->num_params << "|" << type_object->params_size << "|"
                                       << std::endl;
                    }

                    child_field = child_field->next;
                }
            }
        }

        classes_stream << std::endl;
    }
}

// TODO: figure this out
auto dump_engine_to_json() -> void
{
    auto g_Names = reinterpret_cast<TArray<FNameEntry*>*>(Offsets::g_Names);
    console->Println("[dumper] g_Names: 0x{:04x} (size = {})", uintptr_t(g_Names), g_Names->size);

    auto names = g_Names->data;

    using Json = nlohmann::json;
    {
        auto json = Json{};

        for (auto i = 0u; i < g_Names->size; ++i) {
            auto item = names[i];

            if (item && item->index == i << 1 && item->name) {
                json["data"] += {
                    { "name", item->name },
                    { "index", item->index >> 1 },
                };
            }
        }

        std::ofstream name_stream("tron_evolution_names_dump.json");
        name_stream << json;
    }

    auto g_Objects = reinterpret_cast<TArray<UObject*>*>(Offsets::g_Objects);
    console->Println("[dumper] g_Objects: 0x{:04x} (size = {})", uintptr_t(g_Objects), g_Objects->size);

    auto objects = g_Objects->data;

    {
        auto json = Json{};

        auto add_property_data = [](Json& child, UProperty* type_object) -> void {
            child["arrayDim"] = type_object->array_dim;
            child["elementSize"] = type_object->element_size;
            child["propertyFlags"] = type_object->property_flags;
            child["propertySize"] = type_object->property_size;
            child["offset"] = type_object->offset;
        };
        std::function<void(Json&, UField*)> add_field_data;
        add_field_data = [&names, &add_property_data, &add_field_data](Json& child, UField* child_field) -> void {
            auto type_name = child_field->class_object && names[child_field->class_object->name.index]
                ? names[child_field->class_object->name.index]->name
                : "unk";
            child["type"] = type_name;

            if (strcmp(type_name, "Function") == 0) {
                auto type_object = child_field->as<UFunction>();
                child["paramsSize"] = type_object->params_size;
                child["functionFlags"] = type_object->function_flags;
                child["iNative"] = type_object->i_native;
                child["repOffset"] = type_object->rep_offset;
                child["friendlyName"]
                    = names[type_object->friendly_name.index] ? names[type_object->friendly_name.index]->name : "unk";
                child["numParams"] = type_object->num_params;
                child["paramsSize"] = type_object->params_size;
                child["returnValueOffset"] = type_object->return_value_offset;
                child["func"] = uintptr_t(type_object->func);
            } else if (strcmp(type_name, "ScriptStruct") == 0) {
                auto type_object = child_field->as<UScriptStruct>();
            } else if (strcmp(type_name, "StructProperty") == 0) {
                auto type_object = child_field->as<UStructProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "IntProperty") == 0) {
                auto type_object = child_field->as<UIntProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "ByteProperty") == 0) {
                auto type_object = child_field->as<UByteProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "BoolProperty") == 0) {
                auto type_object = child_field->as<UBoolProperty>();
                add_property_data(child, type_object);
                child["bitMask"] = type_object->bit_mask;
            } else if (strcmp(type_name, "FloatProperty") == 0) {
                auto type_object = child_field->as<UFloatProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "NameProperty") == 0) {
                auto type_object = child_field->as<UNameProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "ArrayProperty") == 0) {
                auto type_object = child_field->as<UArrayProperty>();
                add_property_data(child, type_object);
                if (type_object->inner) {
                    auto array_child = Json::object({});
                    add_field_data(array_child, type_object->inner);
                    child["inner"] = array_child;
                }
            } else if (strcmp(type_name, "StrProperty") == 0) {
                auto type_object = child_field->as<UStrProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "ClassProperty") == 0) {
                auto type_object = child_field->as<UClassProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "ObjectProperty") == 0) {
                auto type_object = child_field->as<UObjectProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "Enum") == 0) {
                auto type_object = child_field->as<UEnum>();
                auto enum_names = Json::array();

                foreach_item(item, type_object->names)
                {
                    if (item.index == i << 1 && names[item.index]) {
                        enum_names += {
                            { "name", names[item.index]->name },
                            { "number", item.mumber },
                        };
                    }
                }

                child["names"] = enum_names;
            } else if (strcmp(type_name, "MapProperty") == 0) {
                auto type_object = child_field->as<UMapProperty>();

                if (type_object->key) {
                    auto key_child = Json::object({});
                    add_field_data(key_child, type_object->key);
                    child["key"] = key_child;
                }

                if (type_object->value) {
                    auto value_child = Json::object({});
                    add_field_data(value_child, type_object->value);
                    child["value"] = value_child;
                }
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "ComponentProperty") == 0) {
                auto type_object = child_field->as<UComponentProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "DelegateProperty") == 0) {
                auto type_object = child_field->as<UDelegateProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "Const") == 0) {
                auto type_object = child_field->as<UConst>();
            } else if (strcmp(type_name, "InterfaceProperty") == 0) {
                auto type_object = child_field->as<UInterfaceProperty>();
                add_property_data(child, type_object);
            } else if (strcmp(type_name, "State") == 0) {
                auto type_object = child_field->as<UState>();
            }
        };

        for (auto i = 0u; i < g_Objects->size; ++i) {
            auto item = objects[i];
            if (!item || !item->name.index) {
                continue;
            }

            auto base_name = std::string(names[item->name.index] ? names[item->name.index]->name : "");

            auto outer_name = std::string();
            auto outer = item->outer_object;
            while (outer) {
                outer_name
                    = std::string(names[outer->name.index] ? names[outer->name.index]->name : "") + "::" + outer_name;
                outer = outer->outer_object;
            }

            auto class_name = std::string(item->class_object && names[item->class_object->name.index]
                    ? names[item->class_object->name.index]->name
                    : "");

            auto class_object = item->class_object;
            if (!class_object) {
                continue;
            }

            auto child_field = class_object->children;
            auto children = Json::array();

            while (child_field) {
                auto child_name = names[child_field->name.index];

                auto child = Json::object({
                    { "name", child_name ? child_name->name : "unk" },
                });

                add_field_data(child, child_field);

                children += child;
                child_field = child_field->next;
            }

            json["data"] += {
                { "outerName", outer_name },
                { "baseName", base_name },
                { "className", class_name },
                { "address", uintptr_t(item) },
                { "children", children },
            };
        }

        std::ofstream object_stream("tron_evolution_objects_dump.json");
        object_stream << json;
    }
}

auto dump_console_commands() -> void
{
    if (!tem.engine || !tem.engine->viewport_client || !tem.engine->viewport_client->viewport_console) {
        return;
    }

    std::wofstream console_commands("tron_evolution_console_commands.md");

    auto list = tem.engine->viewport_client->viewport_console->auto_complete_list;

    console_commands << "# Console Commands" << std::endl << std::endl;
    console_commands << "Count: " << list.size << std::endl << std::endl;
    console_commands << "|Command|Description|" << std::endl;
    console_commands << "|---|---|" << std::endl;

    foreach_item(autocomplete, list)
    {
        console_commands << "|" << autocomplete.command.c_str() << "|" << autocomplete.description.c_str() << "|"
                         << std::endl;
    }

    console->Println("[dumper] Dumped {} console commands", list.size);
}
