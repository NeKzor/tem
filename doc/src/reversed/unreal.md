## Unreal Engine

Documenting UE3 at the lowest level :)

- [Objects](#objects)
- [Classes](#classes)
- [Properties](#properties)
- [Definitions](#definitions)
- [Primitives](#primitives)
- [Internals](#internals)

### Objects

Engine objects are of type `UObject` which contain enough information to be able to work with the engine's scripting system [UnrealScript][] and [Kismet][].

[UnrealScript]: https://docs.unrealengine.com/udk/Three/UnrealScriptReference.html
[kismet]: https://docs.unrealengine.com/udk/Three/KismetUserGuide.html

```cpp
struct UObject {
    void* vtable;
    char unk0[36];
    UObject* outer_object;
    FName name;                // Name of object
    UClass* class_object;      // Type information of object
    UObject* object_archetype;
};
```

### Classes

The class `UClass` is needed to get the type information of an object.
From there it is possible to access all the object's children in a linked list which are of type `UField`. They contain [properties](#properties) but also structs, enums, constants, functions or state [definitions](#definitions).

```cpp
struct UField : UObject {
    UField* super_field; // 0x3C
    UField* next;        // 0x40
};

struct UStruct : UField {
    char pad_0044[8];  // 0x44
    UField* children;  // 0x4C
    int property_size; // 0x50
    char pad_0054[60]; // 0x54
};

struct UState : UStruct {
    char pad_0090[84]; // 0x90
};

struct UClass : UState {
    char pad_00E4[248]; // 0xE4
};
```

### Properties

```admonish warning
Element size is the actual size of the property in bytes.
```

```cpp
struct UProperty : UField {
    int array_dim;      // 0x44
    int element_size;   // 0x48
    int property_flags; // 0x4C
    int property_size;  // 0x50
    char pad_0054[16];  // 0x54
    int offset;         // 0x64
    char pad_0064[28];  // 0x68
};
```

|Type|C++ Equivalent|Size in bytes|
|---|---|---|
|[StructProperty][]|struct T|variable|
|[IntProperty][]|int|4|
|[ByteProperty][]|char|1|
|[BoolProperty][]|int foo : 1|4|
|[FloatProperty][]|float|4|
|[NameProperty][]|[FName][]|8|
|[ArrayProperty][]|[TArray\<T\>][]|12|
|[StrProperty][]|[FString][]|12|
|[ClassProperty][]|T*|4|
|[ObjectProperty][]|T*|4|
|[MapProperty][]|[TMap\<T\>][]|60|
|[ComponentProperty][]|T*|4|
|[DelegateProperty][]|[FScriptDelegate][]|12|
|[InterfaceProperty][]|[FScriptInterface\<T\>][]|8|

[StructProperty]: #structproperty
[IntProperty]: #intproperty
[ByteProperty]: #byteproperty
[BoolProperty]: #boolproperty
[FloatProperty]: #floatproperty
[NameProperty]: #nameproperty
[ArrayProperty]: #arrayproperty
[StrProperty]: #strproperty
[ClassProperty]: #classproperty
[ObjectProperty]: #objectproperty
[MapProperty]: #mapproperty
[ComponentProperty]: #componentproperty
[DelegateProperty]: #delegateproperty
[InterfaceProperty]: #interfaceproperty
[FName]: #fname
[TArray\<T\>]: #tarrayt
[FString]: #fstring
[TMap\<T\>]: #tmapt
[FScriptDelegate]: #fscriptdelegate
[FScriptInterface\<T\>]: #fscriptinterfacet

#### StructProperty

```cpp
struct UStructProperty : UProperty {
    UStruct* property_struct; // 0x84
};
```

#### IntProperty

```cpp
struct UIntProperty : UProperty {};
```

#### ByteProperty

```cpp
struct UEnum : UField {
    TArray<FName> names; // 0x44
};

struct UByteProperty : UProperty {
    UEnum* enum_object; // 0x84
};
```

#### BoolProperty

```cpp
struct UBoolProperty : UProperty {
    int bit_mask; // 0x84
};
```

#### FloatProperty

```cpp
struct UFloatProperty : UProperty {};
```

#### NameProperty

```cpp
struct UNameProperty : UProperty {};
```

#### ArrayProperty

```cpp
struct UArrayProperty : UProperty {
    UProperty* inner; // 0x84
};
```

#### StrProperty

```cpp
struct UStrProperty : UProperty {};
```

#### ClassProperty

```cpp
struct UClassProperty : UProperty {
    UStruct* meta_class; // 0x84
    char pad_0088[4];    // 0x88
};
```

#### ObjectProperty

```cpp
struct UObjectProperty : UProperty {
    UStruct* property_class; // 0x84
};
```

#### MapProperty

Unfortunately this engine version did not properly preserve the types of key-value pairs.

```cpp
struct UMapProperty : UProperty {
    UProperty* key;   // 0x84
    UProperty* value; // 0x88
};
```

#### ComponentProperty

```cpp
struct UComponentProperty : UProperty {
    UObject* component; // 0x84
};
```

#### DelegateProperty

```cpp
struct UDelegateProperty : UProperty {
    char pad_0084[8]; // 0x84
};
```

#### InterfaceProperty

```cpp
struct UInterfaceProperty : UProperty {
    UStruct* interface_class; // 0x84
};
```

### Definitions

|Definition|C++ Equivalent|
|---|---|
|[Const][]|const auto|
|[Enum][]|enum|
|[ScriptStruct][]|struct T|
|[Function][]|auto foo_bar() -> T|
|[State][]|auto foo_bar() -> void|

[Const]: #const
[Enum]: #enum
[ScriptStruct]: #scriptstruct
[Function]: #function
[State]: #state

#### Const

Value can be of type int, float etc. Strings are declared in double quotes and FName values are declared in single quotes.

```cpp
struct UConst : UField {
    FString value; // 0x44
};
```

#### Enum

Enum values always start from zero and then increment by one.
Also every enum always ends with a `MAX` declaration.

```cpp
struct UEnum : UField {
    TArray<FName> names; // 0x44
};
```

#### ScriptStruct

Struct definitions inside a class which can be nested, although it only has been observed to be a depth of two levels.

```cpp
struct UScriptStruct : UStruct {
    char pad_0090[28]; // 0x90
};
```

#### Function

Parameters are linked in its children.
A child can have the name of `ReturnValue` which contains the return value.

```cpp
struct UFunction : UStruct {
    int function_flags;      // 0x90
    uint16_t i_native;       // 0x94
    uint16_t rep_offset;     // 0x96
    FName friendly_name;     // 0x98
    uint16_t num_params;     // 0xA0
    uint16_t params_size;    // 0xA2
    int return_value_offset; // 0xA4
    int padding;             // 0xA8
    void* func;              // 0xAC
};
```

To execute a function see [ProcessEvent](#processevent).

#### State

As seen [above](#classes), `UClass` inherits this.

```cpp
struct UState : UStruct {
    char pad_0090[84]; // 0x90
};
```

### Primitives

#### FName

All names (indexed strings) are stored in a global array of type `TArray<FNameEntry>`. `FName` contains an `index` into this global array and an optional `number` value which is used to differentiate names of the same `index` e.g. Parameter_1, Parameter_2. Most of the time the value of `index` is 0 but it can start at value 2.

```cpp
struct FName {
    unsigned int index;  // 0x00
    unsigned int mumber; // 0x04
};

// NOTE: Simplified because size of name is variable
struct FNameEntry {
    int unk0;        // 0x00
    int unk1;        // 0x04
    int index;       // 0x08
    int unk2;        // 0x0c
    char name[1024]; // 0x10
};
```

#### TArray\<T\>

```cpp
template <typename T>
struct TArray {
    T* data;           // 0x00
    unsigned int size; // 0x04
    unsigned int max;  // 0x08
};
```

#### FString

`FString` class is basically a `TArray` of UTF-16LE encoded characters. The engine uses this mainly for string operations such as `FString::Printf` etc.

```cpp
struct FString
     : TArray<wchar_t> {
};
```

#### TMap\<T\>

```admonish warning
`FPair` is `TPair` which can variable in size, so this simplified definition is not correct.
```

```cpp
template <
    typename K = void*,
    typename V = void*>
struct TPair {
    K key;   // variable size
    V value; // variable size
    int unk0;
    int unk1;
};

struct FPair
     : TPair<void*, void*> {
};

template <typename T = FPair>
struct TMap {
    TArray<T> value; // 0x00
    char unk0[48];   // 0x12
};
```

#### FScriptDelegate

```admonish todo
Generic?
```

```cpp
struct FScriptDelegate {
    UObject* object;     // 0x00
    FName function_name; // 0x04
};
```

#### FScriptInterface\<T\>

```admonish todo
Object and interface seem to always point to the same address?
```

```cpp
template <typename T>
struct FScriptInterface {
    T* object;    // 0x00
    T* interface; // 0x04
};
```

### Internals

#### ProcessEvent

This virtual function exists in every object of type `UObject`.
It is used to dispatch events like functions.

Virtual function offset: 62

```cpp
// The parameter `result` is unused
auto UObject::ProcessEvent(UFunction* func, void* params, int result) -> void
{
}
```

```admonish todo
Dispatch a function in TEM
```

```admonish todo
Figure out how to dispatch remote function
```

```cpp
// Find function in class
UFunction* some_function = object->FindFunction("some_function");

// Write parameter values into an anonymous struct
struct { int param1; int param2; int ReturnValue } params = { 123, 456 };

// Dispatch with pointers to function and params 
object->ProcessEvent(some_function, &params, null);

// Return value will be set if the `params` struct has one (since it's only optional)
params.ReturnValue
```
