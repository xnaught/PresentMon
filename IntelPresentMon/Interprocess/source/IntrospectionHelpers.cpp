#include "IntrospectionHelpers.h"
#include "metadata/EnumDataType.h"
#include "IntrospectionMacroHelpers.h"
#include "IntrospectionDataTypeMapping.h"


namespace pmon::ipc::intro
{
	// TODO: use bridge for this
	size_t GetDataTypeSize(PM_DATA_TYPE v)
	{
#define X_REG_KEYS(enum_frag, key_frag, name, short_name, description) case MAKE_KEY_SYMBOL(enum_frag, key_frag): return DataTypeToStaticType_sz<MAKE_KEY_SYMBOL(enum_frag, key_frag)>;
		switch (v) {
			ENUM_KEY_LIST_DATA_TYPE(X_REG_KEYS)
		}
#undef X_REG_KEYS
		return 0;
	}
}