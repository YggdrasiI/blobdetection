#include "enums.h"

// Note NDEBUG is defined by cmake for build type 'Release'.
// We strip the strings from the build in this case.

#ifdef NDEBUG
void printf_enums(void){
    printf("Not available if compiled without debug information.\n");
}
void fprintf_enum_name(FILE *stream, int id) {
  fprintf(stream, "'%d'", id);
}
void fprintf_err(FILE *stream, int err) {
  fprintf(stream, "Error (%d)", err);
}
#else

void printf_enums(void)
{
#define PRINT_NAME_AND_VALUE(ID, NAME, ...) printf(#NAME " = %d\n", NAME);
  FOR_LIST_OF_TREE_ERRORS( PRINT_NAME_AND_VALUE );
  FOR_LIST_OF_TREE_COMPARE_TYPES( PRINT_NAME_AND_VALUE );
  FOR_LIST_OF_TREE_SORTING_TYPES( PRINT_NAME_AND_VALUE );
#undef PRINT_NAME_AND_VALUE
}

void fprintf_enum_name(FILE *stream, int id) {
#define PRINT_NAME_FOR_ENUM_ID(ID, NAME, ...) \
  if(id == ID) {\
    fprintf(stream, "%s", #NAME); \
    return; \
  }
  FOR_LIST_OF_TREE_ERRORS( PRINT_NAME_FOR_ENUM_ID );
  FOR_LIST_OF_TREE_COMPARE_TYPES( PRINT_NAME_FOR_ENUM_ID );
  FOR_LIST_OF_TREE_SORTING_TYPES( PRINT_NAME_FOR_ENUM_ID );
#undef PRINT_NAME_FOR_ENUM_ID

  fprintf(stream, "'%d' (no name defined)", id); // Fallback if no enum name defined.
}

void fprintf_err(FILE *stream, int err) {
#define PRINT_NAME_AND_VALUE(ID, NAME, ...) \
  if(err == ID) {\
    fprintf(stream, "Error " #NAME " (%d)\n", NAME); \
    return; \
  }
  FOR_LIST_OF_TREE_ERRORS( PRINT_NAME_AND_VALUE )

#undef PRINT_NAME_AND_VALUE
  fprintf(stream, "Error name?! (%d)", err);
}

#endif
