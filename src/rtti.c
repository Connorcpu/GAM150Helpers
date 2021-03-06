////////////////////////////////////////////
// File    : rtti.c
// Author  : Connor Hilarides
// Created : 2015/11/19
////////////////////////////////////////////

#include "rtti.h"
#include "helpers.h"
#include "string.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const Field *Type$find_field(const Type *this, const char *name)
{
    for (unsigned i = 0; i < this->field_count; ++i)
    {
        const Field *field = this->fields[i];
        if (strcmp(field->name, name) == 0)
        {
            return field;
        }
    }

    return NULL;
}

const Member *Type$find_member(const Type *this, const char *name)
{
    for (unsigned i = 0; i < this->member_count; ++i)
    {
        const Member *member = this->members[i];
        if (strcmp(member->name, name) == 0)
        {
            return member;
        }
    }

    return NULL;
}

const Any Member$invoke(const Member *this, void *obj, unsigned arg_count, Any *args)
{
    Any result = this->invoke(obj, arg_count, args);

    // Clean up unused arguments
    for (unsigned i = 0; i < arg_count; ++i)
    {
        // Free is a nop on already-freed values (i.e. ones set to Any$EMPTY)
        Any$free(&args[i]);
    }

    return result;
}

Any Any$make_default(const Type *type)
{
    if (type == NULL)
    {
        return Any$EMPTY;
    }

    switch (type->kind)
    {
        case TK_VOID:
        case TK_PRIMITIVE:
        case TK_POINTER:
        {
            // Primitives and Pointers default to 0
            Any result = Any$EMPTY;
            result.type = type;
            return result;
        }
        case TK_COMPLEX:
        {
            // Call the default constructor
            return Member$invoke(type->constructor, NULL, 0, NULL);
        }
        default:
        {
            return Any$EMPTY;
        }
    }
}

Any Any$from_int8(int8_t i)
{
    Any result = { &type_int8_t };
    result.value.i8 = i;
    return result;
}

Any Any$from_uint8(uint8_t u)
{
    Any result = { &type_uint8_t };
    result.value.u8 = u;
    return result;
}

Any Any$from_int16(int16_t i)
{
    Any result = { &type_int16_t };
    result.value.i16 = i;
    return result;
}

Any Any$from_uint16(uint16_t u)
{
    Any result = { &type_uint16_t };
    result.value.u16 = u;
    return result;
}

Any Any$from_int32(int32_t i)
{
    Any result = { &type_int32_t };
    result.value.i32 = i;
    return result;
}

Any Any$from_uint32(uint32_t u)
{
    Any result = { &type_uint8_t };
    result.value.u32 = u;
    return result;
}

Any Any$from_int64(int64_t i)
{
    Any result = { &type_int64_t };
    result.value.i64 = i;
    return result;
}

Any Any$from_uint64(uint64_t u)
{
    Any result = { &type_uint64_t };
    result.value.u64 = u;
    return result;
}

Any Any$from_size_t(size_t s)
{
    #ifdef x86_64
    return Any$from_uint64(s);
    #else
    return Any$from_uint32(s);
    #endif
}

Any Any$from_float(float f)
{
    Any result = { &type_float };
    result.value.f32 = f;
    return result;
}

Any Any$from_double(double d)
{
    Any result = { &type_double };
    result.value.f64 = d;
    return result;
}

Any Any$from_cstr(const char *cstr)
{
    Any result = { &type_cstr };
    result.value.cstr = cstr;
    return result;
}

Any Any$from_complex(const Type *type, void *value)
{
    switch (type->kind)
    {
        case TK_PRIMITIVE:
        {
            Any result;
            result.type = type;
            memcpy(&result.value, value, type->size);
            return result;
        }
        case TK_POINTER:
        {
            Any result;
            result.type = type;
            result.value.ptr = value;
            return result;
        }
        case TK_COMPLEX:
        {
            Any result;
            void *storage = malloc(type->size);

            assert(storage && "Uh oh, failed to allocate memory!");
            memcpy(storage, value, type->size);

            result.type = type;
            result.value.ptr = storage;
            return result;
        }
        case TK_VOID:
        default:
        {
            return Any$EMPTY;
        }
    }
}

Any Any$ref_complex(const Type *type, void *value)
{
    Any any;
    any.type = type;
    any.value.ptr = value;
    return any;
}

Any Any$copy(Any obj)
{
    if (!obj.type)
    {
        return Any$EMPTY;
    }

    switch (obj.type->kind)
    {
        case TK_COMPLEX:
        {
            return Member$invoke(obj.type->constructor, obj.value.ptr, 1, &obj);
        }
        case TK_VOID:
        case TK_PRIMITIVE:
        case TK_POINTER:
        default:
        {
            return obj;
        }
    }
}

void Any$unpack(Any boxed, void *placement)
{
    switch (boxed.type->kind)
    {
        case TK_COMPLEX:
        {
            memcpy(placement, boxed.value.ptr, boxed.type->size);
        }
        default:
        {
            memcpy(placement, &boxed.value, boxed.type->size);
        }
    }
}

void Any$free(Any *boxed)
{
    if (boxed->type && boxed->type->kind == TK_COMPLEX)
    {
        Member$invoke(boxed->type->destructor, boxed->value.ptr, 0, NULL);
        free(boxed->value.ptr);
    }
    *boxed = Any$EMPTY;
}

void Any$freev(Any boxed)
{
    Any$free(&boxed);
}

void Any$soft_release(Any *boxed)
{
    if (boxed->type && boxed->type->kind == TK_COMPLEX)
    {
        free(boxed->value.ptr);
    }
    *boxed = Any$EMPTY;
}

void Any$delete_ref(Any *boxed)
{
    if (boxed->type && boxed->type->kind == TK_COMPLEX)
    {
        Member$invoke(boxed->type->destructor, boxed->value.ptr, 0, NULL);
    }
    *boxed = Any$EMPTY;
}

Any Any$invoke(Any self, const char *member_name, unsigned arg_count, Any *args)
{
    if (!self.type) { return Any$EMPTY; }
    const Member *member = Type$find_member(self.type, member_name);
    if (!member) { return Any$EMPTY; }
    return Member$invoke(member, self.value.ptr, arg_count, args);
}

void Any$print(Any obj, FILE *stream)
{
    if (!obj.type)
    {
        fputs("Any$EMPTY", stream);
        return;
    }

    switch (obj.type->kind)
    {
        case TK_VOID:
        {
            fputs("void", stream);
            return;
        }
        case TK_PRIMITIVE:
        {
            if (obj.type == &type_int8_t)
            {
                fprintf(stream, "%I32d", (int32_t)obj.value.i8);
            }
            else if (obj.type == &type_uint8_t)
            {
                fprintf(stream, "%I32u", (uint32_t)obj.value.u8);
            }
            else if (obj.type == &type_int16_t)
            {
                fprintf(stream, "%I32d", (int32_t)obj.value.i16);
            }
            else if (obj.type == &type_uint16_t)
            {
                fprintf(stream, "%I32u", (uint32_t)obj.value.u16);
            }
            else if (obj.type == &type_int32_t)
            {
                fprintf(stream, "%I32d", obj.value.i32);
            }
            else if (obj.type == &type_uint32_t)
            {
                fprintf(stream, "%I32u", obj.value.u32);
            }
            else if (obj.type == &type_int64_t)
            {
                fprintf(stream, "%I64d", obj.value.i64);
            }
            else if (obj.type == &type_uint64_t)
            {
                fprintf(stream, "%I64u", obj.value.u64);
            }
            else if (obj.type == &type_float)
            {
                fprintf(stream, "%f", obj.value.f32);
            }
            else if (obj.type == &type_double)
            {
                fprintf(stream, "%f", obj.value.f64);
            }
            else if (obj.type == &type_cstr)
            {
                fputs(obj.value.cstr, stream);
            }
            else
            {
                fprintf(stream, "#<%s:0x%p>", obj.type->name, obj.value.ptr);
            }
            return;
        }
        case TK_POINTER:
        case TK_COMPLEX:
        {
            if (obj.type == &type_string || obj.type == &type_string_ptr)
            {
                fprintf(stream, "\"%s\"", String$cstr((const String *)obj.value.ptr));
                return;
            }
            else
            {
                fprintf(stream, "#<%s:0x%p>", obj.type->name, obj.value.ptr);
                return;
            }
        }
    }
}

#define DEF_PRIMITIVE(T) { TK_PRIMITIVE, sizeof(T), sizeof(T), #T }
Type type_void = { TK_VOID, 0, 0, "void" };
Type type_int8_t = DEF_PRIMITIVE(int8_t);
Type type_uint8_t = DEF_PRIMITIVE(uint8_t);
Type type_int16_t = DEF_PRIMITIVE(int16_t);
Type type_uint16_t = DEF_PRIMITIVE(uint16_t);
Type type_int32_t = DEF_PRIMITIVE(int32_t);
Type type_uint32_t = DEF_PRIMITIVE(uint32_t);
Type type_int64_t = DEF_PRIMITIVE(int64_t);
Type type_uint64_t = DEF_PRIMITIVE(uint64_t);
#ifdef x86_64
Type type_size_t = DEF_PRIMITIVE(uint64_t);
#else
Type type_size_t = DEF_PRIMITIVE(uint32_t);
#endif
Type type_float = DEF_PRIMITIVE(float);
Type type_double = DEF_PRIMITIVE(double);
Type type_cstr = { TK_POINTER, sizeof(const char *), sizeof(const char *), "cstr" };

static Any any_ctor(void *this, unsigned arg_count, Any *args)
{
    (this); // unused
    if (arg_count)
    {
        Any copy = Any$copy(args[0]);
        return Any$from_complex(&type_any, &copy);
    }
    else
    {
        return Any$EMPTY;
    }
}

static Type *any_ctor_args[] =
{
    &type_any,
};

static Member any_ctor_member =
{
    ".ctor",
    any_ctor,
    ARRAY_SIZE(any_ctor_args), any_ctor_args, // Args
    &type_any, // Return type
    true, // static
    true, // overloaded
};

static Any any_dtor(void *this, unsigned arg_count, Any *args)
{
    (args); // unused
    assert(arg_count == 0);
    Any$free((Any *)this);
    return Any$VOID;
}

static Member any_dtor_member =
{
    ".dtor",
    any_dtor,
    0, NULL, // Args
    &type_void, // Return type
    false, // static
    false, // overloaded
};

static const Member *any_member_list[] =
{
    &any_ctor_member,
    &any_dtor_member,
};

Type type_any =
{
    TK_COMPLEX,
    sizeof(Any),
    sizeof(void *),
    "Any",
    NULL,
    &any_ctor_member,
    &any_dtor_member,

    0, NULL, // Fields
    ARRAY_SIZE(any_member_list), any_member_list,
    0, NULL, // Interfaces
};

Any Any$EMPTY = { NULL };
Any Any$VOID = { &type_void };
