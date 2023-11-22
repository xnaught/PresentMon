#pragma once

#define XSTR_(macro) #macro
#define STRINGIFY_MACRO_CALL(macro) XSTR_(macro)
#define MAKE_MASTER_SYMBOL(enum_frag) PM_ENUM_##enum_frag
#define MAKE_ENUM_SYMBOL(enum_frag) PM_##enum_frag
#define CREATE_INTROSPECTION_ENUM(enum_frag, description) \
	std::make_unique<IntrospectionEnum>(MAKE_MASTER_SYMBOL(enum_frag), STRINGIFY_MACRO_CALL( \
	MAKE_ENUM_SYMBOL(enum_frag)), description)
#define MAKE_LIST_SYMBOL(enum_frag) ENUM_KEY_LIST_##enum_frag
#define MAKE_KEY_SYMBOL(enum_frag, key_frag) PM_##enum_frag##_##key_frag
#define REGISTER_ENUM_KEY(p_enum_obj, enum_frag, key_frag, name, short_name, description) \
	p_enum_obj->AddKey(std::make_unique<IntrospectionEnumKey>(MAKE_MASTER_SYMBOL(enum_frag), \
	MAKE_KEY_SYMBOL(enum_frag, key_frag), STRINGIFY_MACRO_CALL(MAKE_KEY_SYMBOL(enum_frag, key_frag)), \
	name, short_name, description))
#define FULL_STATS PM_STAT_AVG, PM_STAT_PERCENTILE_99, PM_STAT_PERCENTILE_95, \
	PM_STAT_PERCENTILE_90, PM_STAT_MAX, PM_STAT_MIN, PM_STAT_RAW