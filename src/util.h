#ifndef UTIL_H
#define UTIL_H
enum t_typename {
	T_BOOL,
	T_UCHAR,
	T_CHAR,
	T_SCHAR,
	T_SHORT,
	T_USHORT,
	T_INT,
	T_UINT,
	T_LONG,
	T_ULONG,
	T_LLONG,
	T_ULLONG,
	T_FLOAT,
	T_DOUBLE,
	T_LDOUBLE,
	T_CHAR_PTR,
	T_VOID_PTR,
	T_INT_PTR,
	T_OTHER
};
#define typeid(x) _Generic((x),                                                 \
	_Bool: T_BOOL,                  unsigned char: T_UCHAR,          \
	char: T_CHAR,                     signed char: T_SCHAR,            \
	short int: T_SHORT,         unsigned short int: T_USHORT,     \
	int: T_INT,                     unsigned int: T_UINT,           \
	long int: T_LONG,           unsigned long int: T_ULONG,      \
	long long int: T_LLONG, unsigned long long int: T_ULLONG, \
	float: T_FLOAT,                         double: T_DOUBLE,                 \
	long double: T_LDOUBLE,                   char *: T_CHAR_PTR,        \
	void *: T_VOID_PTR,                int *: T_INT_PTR,         \
	default: T_OTHER)
#define typename(x) _Generic((x),                                                 \
	_Bool: "_Bool",                  unsigned char: "unsigned char",          \
	char: "char",                     signed char: "signed char",            \
	short int: "short int",         unsigned short int: "unsigned short int",     \
	int: "int",                     unsigned int: "unsigned int",           \
	long int: "long int",           unsigned long int: "unsigned long int",      \
	long long int: "long long int", unsigned long long int: "unsigned long long int", \
	float: "float",                         double: "double",                 \
	long double: "long double",                   char *: "pointer to char",        \
	void *: "pointer to void",                int *: "pointer to int",         \
	default: "other")
#endif // UTIL_H
