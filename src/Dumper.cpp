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

auto dump_engine_to_markdown() -> void
{
    std::ofstream stream("classes.md");
    std::ofstream navigation("classes_navigation.md");

    auto g_Objects = *reinterpret_cast<TArray<UObject*>*>(Offsets::g_Objects);
    auto g_Names = *reinterpret_cast<TArray<FNameEntry*>*>(Offsets::g_Names);

    auto get_name
        = [&g_Names](FName& name) -> const char* { return g_Names[name.index] ? g_Names[name.index]->name : "unk"; };

    auto get_object_name = [&g_Names](UObject* object) -> const char* {
        return object && g_Names[object->name.index] ? g_Names[object->name.index]->name : "unk";
    };

    auto get_outer_object_name = [&g_Names](UObject* object) -> const char* {
        return object->outer_object && g_Names[object->outer_object->name.index]
            ? g_Names[object->outer_object->name.index]->name
            : "unk";
    };

    auto get_class_object_name = [&g_Names](UObject* object) -> const char* {
        return object->class_object && g_Names[object->class_object->name.index]
            ? g_Names[object->class_object->name.index]->name
            : "unk";
    };

    std::function<std::string(UField*)> resolve_type;

    resolve_type = [&get_class_object_name, &get_object_name, &resolve_type](UField* field) -> std::string {
        auto result = std::string("");
        auto type_name = get_class_object_name(field);

        if (strcmp(type_name, "StructProperty") == 0) {
            result = get_object_name(field->as<UStructProperty>()->property_struct);
        } else if (strcmp(type_name, "IntProperty") == 0) {
            result = "i32";
        } else if (strcmp(type_name, "ByteProperty") == 0) {
            result = "i8";
        } else if (strcmp(type_name, "BoolProperty") == 0) {
            result = "bool";
        } else if (strcmp(type_name, "FloatProperty") == 0) {
            result = "f32";
        } else if (strcmp(type_name, "NameProperty") == 0) {
            result = "FName";
        } else if (strcmp(type_name, "ArrayProperty") == 0) {
            result = std::string("TArray\\<") + resolve_type(field->as<UArrayProperty>()->inner) + "\\>";
        } else if (strcmp(type_name, "StrProperty") == 0) {
            result = "FString";
        } else if (strcmp(type_name, "ClassProperty") == 0) {
            result = "UClass*";
        } else if (strcmp(type_name, "ObjectProperty") == 0) {
            result = std::string(get_object_name(field->as<UObjectProperty>()->property_class)) + "*";
        } else if (strcmp(type_name, "MapProperty") == 0) {
            result = "TMap\\<FPair\\>"; // Actual key/value type information seems to be lost :>
        } else if (strcmp(type_name, "ComponentProperty") == 0) {
            result = std::string(get_object_name(field->as<UComponentProperty>()->component)) + "*";
        } else if (strcmp(type_name, "DelegateProperty") == 0) {
            result = "FScriptDelegate";
        } else if (strcmp(type_name, "InterfaceProperty") == 0) {
            result = std::string(get_object_name(field->as<UInterfaceProperty>()->interface_class)) + "*";
        } else if (strcmp(type_name, "State") == 0 || strcmp(type_name, "Enum") == 0 || strcmp(type_name, "Const") == 0
            || strcmp(type_name, "ScriptStruct") == 0 || strcmp(type_name, "Function") == 0) {
            result = "unknown_t"; // should not happen
        } else {
            result = get_object_name(field);
        }
        return result;
    };

    navigation << "|Class|Properties|States|Functions|Enums|Consts|Structs|" << std::endl;
    navigation << "|---|---|---|---|---|---|---|" << std::endl;

    stream << "# Classes" << std::endl << std::endl;

    auto unique_classes = std::set<UClass*>();

    foreach_item(item, g_Objects)
    {
        if (!item || !item->name.index) {
            continue;
        }

        auto class_object = item->class_object;
        if (!class_object || unique_classes.contains(class_object)) {
            continue;
        }

        unique_classes.emplace(class_object);

        auto class_name = get_object_name(class_object);
        auto outer_name = get_outer_object_name(class_object);

        auto class_name_lowercase = _strdup(class_name);
        auto class_name_lowercase_ptr = class_name_lowercase;
        while (*class_name_lowercase_ptr) {
            *class_name_lowercase_ptr = char(tolower(*class_name_lowercase_ptr));
            ++class_name_lowercase_ptr;
        }

        stream << "## " << class_name << std::endl << std::endl;
        navigation << "|[" << class_name << "](./classes.md#" << class_name_lowercase << ")|";

        if (class_object->super_field) {
            stream << "Inherits: ";

            auto super_field = class_object->super_field;
            while (super_field) {
                auto super_field_name = get_object_name(super_field);

                stream << "[" << super_field_name << "](#";

                while (*super_field_name) {
                    stream << char(tolower(*super_field_name));
                    ++super_field_name;
                }

                stream << ")";

                super_field = super_field->super_field;
                if (super_field) {
                    stream << " \\> ";
                }
            }

            stream << std::endl << std::endl;
        }

        stream << "Package: " << outer_name << std::endl << std::endl;
        stream << "Size: 0x" << std::hex << class_object->property_size << " | " << std::dec
               << class_object->property_size << " bytes" << std::endl;

        auto child_field = class_object->children;
        if (child_field) {
            auto has_properties = false;
            auto has_states = false; // These are like mixins or trait functions; is return value type always void?
            auto has_script_structs = false;
            auto has_consts = false;
            auto has_enums = false;
            auto has_functions = false;

            while (child_field) {
                auto type_name = get_class_object_name(child_field);

                if (strstr(type_name, "Property")) {
                    has_properties = true;
                } else if (strcmp(type_name, "State") == 0) {
                    has_states = true;
                } else if (strcmp(type_name, "ScriptStruct") == 0) {
                    has_script_structs = true;
                } else if (strcmp(type_name, "Const") == 0) {
                    has_consts = true;
                } else if (strcmp(type_name, "Enum") == 0) {
                    has_enums = true;
                } else if (strcmp(type_name, "Function") == 0) {
                    has_functions = true;
                }

                child_field = child_field->next;
            }

            if (has_properties) {
                navigation << "[Properties](./classes.md#" << class_name_lowercase << "-properties)|";
                stream << std::endl << "<a id=\"" << class_name_lowercase << "-properties\"></a>" << std::endl;
                stream << "### Properties" << std::endl << std::endl;
                stream << "|Property|Type|Size|Offset|" << std::endl;
                stream << "|---|:-:|:-:|:-:|" << std::endl;

                child_field = class_object->children;
                while (child_field) {
                    auto child_name = get_object_name(child_field);
                    auto type_name = get_class_object_name(child_field);

                    if (strstr(type_name, "Property")) {
                        auto type_object = child_field->as<UProperty>();
                        stream << "|" << child_name << "|" << resolve_type(child_field) << "|0x" << std::hex
                               << type_object->element_size << "|0x" << std::hex << type_object->offset << "|"
                               << std::endl;
                    }

                    child_field = child_field->next;
                }
            } else {
                navigation << "|";
            }

            if (has_states) {
                navigation << "[States](./classes.md#" << class_name_lowercase << "-states)|";
                stream << std::endl <<  "<a id=\"" << class_name_lowercase << "-states\"></a>" << std::endl;
                stream <<"### States" << std::endl << std::endl;
                stream << "|Signature|" << std::endl;
                stream << "|---|" << std::endl;

                child_field = class_object->children;
                while (child_field) {
                    auto type_name = get_class_object_name(child_field);

                    if (strcmp(type_name, "State") == 0) {
                        auto state_name = get_object_name(child_field);
                        auto state_child = child_field->as<UState>()->children;

                        if (state_child) {
                            auto state_child_name = get_object_name(state_child);
                            stream << "|" << state_child_name << "_" << state_name << "(";

                            auto has_parameters = false;

                            if (state_child->super_field) {
                                auto state_parameter = state_child->super_field->as<UState>()->children;
                                if (state_parameter) {
                                    has_parameters = true;
                                }

                                while (state_parameter) {
                                    stream << "<br>&nbsp;&nbsp;&nbsp;&nbsp;" << get_object_name(state_parameter) << ": "
                                           << resolve_type(state_parameter) << ",";

                                    state_parameter = state_parameter->next;
                                }
                            }

                            if (has_parameters) {
                                stream << "<br>";
                            }

                            stream << ") -> ()|" << std::endl;
                        }
                    }

                    child_field = child_field->next;
                }
            } else {
                navigation << "|";
            }

            if (has_functions) {
                navigation << "[Functions](./classes.md#" << class_name_lowercase << "-functions)|";
                stream << std::endl << "<a id=\"" << class_name_lowercase << "-functions\"></a>" << std::endl;
                stream << "### Functions" << std::endl << std::endl;
                stream << "|Signature|" << std::endl;
                stream << "|---|" << std::endl;

                child_field = class_object->children;
                while (child_field) {
                    auto type_name = get_class_object_name(child_field);

                    if (strcmp(type_name, "Function") == 0) {
                        auto function_name = get_object_name(child_field);
                        auto function_parameter = child_field->as<UFunction>()->children;

                        stream << function_name << "(";

                        auto return_value_type = std::string("()");
                        auto has_parameters = false;

                        while (function_parameter) {
                            auto parameter_name = get_object_name(function_parameter);
                            auto parameter_type = get_class_object_name(function_parameter);

                            if (strcmp(parameter_name, "ReturnValue") == 0) {
                                return_value_type = resolve_type(function_parameter);
                            } else {
                                has_parameters = true;

                                stream << "<br>&nbsp;&nbsp;&nbsp;&nbsp;" << parameter_name << ": "
                                       << resolve_type(function_parameter) << ",";
                            }

                            function_parameter = function_parameter->next;
                        }

                        if (has_parameters) {
                            stream << "<br>";
                        }

                        stream << ") -> " << return_value_type << "|" << std::endl;
                    }

                    child_field = child_field->next;
                }
            } else {
                navigation << "|";
            }

            if (has_enums) {
                navigation << "[Enums](./classes.md#" << class_name_lowercase << "-enums)|";
                stream << std::endl << "<a id=\"" << class_name_lowercase << "-enums\"></a>" << std::endl;
                stream << "### Enums" << std::endl << std::endl;
                stream << "|Enum|" << std::endl;
                stream << "|---|" << std::endl;

                child_field = class_object->children;
                while (child_field) {
                    auto child_name = get_object_name(child_field);
                    auto type_name = get_class_object_name(child_field);

                    if (strstr(type_name, "Enum")) {
                        stream << "|" << child_name << " {";

                        auto names = child_field->as<UEnum>()->names;

                        foreach_item(enum_name, names)
                        {
                            stream << "<br>&nbsp;&nbsp;&nbsp;&nbsp;" << get_name(enum_name) << ",";
                        }

                        stream << "<br>}|" << std::endl;
                    }

                    child_field = child_field->next;
                }
            } else {
                navigation << "|";
            }

            if (has_consts) {
                navigation << "[Consts](./classes.md#" << class_name_lowercase << "-consts)|";
                stream << std::endl << "<a id=\"" << class_name_lowercase << "-consts\"></a>" << std::endl;
                stream << "### Consts" << std::endl << std::endl;
                stream << "|Constant|Value|" << std::endl;
                stream << "|---|:-:|" << std::endl;

                child_field = class_object->children;
                while (child_field) {
                    auto child_name = get_object_name(child_field);
                    auto type_name = get_class_object_name(child_field);

                    if (strstr(type_name, "Const")) {
                        auto value = child_field->as<UConst>()->value;
                        stream << "|" << child_name << "|" << value.str() << "|" << std::endl;
                    }

                    child_field = child_field->next;
                }
            } else {
                navigation << "|";
            }

            if (has_script_structs) {
                navigation << "[Structs](./classes.md#" << class_name_lowercase << "-script-structs)|";
                stream << std::endl << "<a id=\"" << class_name_lowercase << "-script-structs\"></a>" << std::endl;
                stream << "### Script Structs" << std::endl << std::endl;
                stream << "|Struct|Size|" << std::endl;
                stream << "|---|:-:|" << std::endl;

                child_field = class_object->children;
                while (child_field) {
                    auto child_name = get_object_name(child_field);
                    auto type_name = get_class_object_name(child_field);

                    if (strstr(type_name, "ScriptStruct")) {
                        stream << "|" << child_name << " {";

                        auto script_struct = child_field->as<UScriptStruct>();
                        auto struct_member = script_struct->children;

                        while (struct_member) {
                            auto member_name = get_object_name(struct_member);
                            auto member_type = get_class_object_name(struct_member);

                            // TODO: recursive
                            if (strstr(member_type, "ScriptStruct")) {
                                stream << "<br>&nbsp;&nbsp;&nbsp;&nbsp;" << member_name << " {";

                                auto member_script_struct = struct_member->as<UScriptStruct>();
                                auto member_struct_member = member_script_struct->children;

                                while (member_struct_member) {
                                    auto member_struct_name = get_object_name(member_struct_member);
                                    auto member_struct_property = member_struct_member->as<UProperty>();

                                    stream << "<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
                                           << member_struct_name << ": " << resolve_type(member_struct_member)
                                           << ", // 0x" << std::hex << member_struct_property->offset;

                                    member_struct_member = member_struct_member->next;
                                }

                                stream << "<br>&nbsp;&nbsp;&nbsp;&nbsp;}";
                            } else {
                                auto member_property = struct_member->as<UProperty>();

                                stream << "<br>&nbsp;&nbsp;&nbsp;&nbsp;" << member_name << ": "
                                       << resolve_type(struct_member) << ", // 0x" << std::hex
                                       << member_property->offset;
                            }

                            struct_member = struct_member->next;
                        }

                        stream << "<br>}|0x" << std::hex << script_struct->property_size << "|" << std::endl;
                    }

                    child_field = child_field->next;
                }
            } else {
                navigation << "|";
            }

            stream << std::endl;
        } else {
            navigation << "||||||";
        }

        free(class_name_lowercase);

        navigation << std::endl;
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
