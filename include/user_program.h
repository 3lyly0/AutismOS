#ifndef USER_PROGRAM_H
#define USER_PROGRAM_H

#include "types.h"

// User mode program entry point (from assembly)
extern void user_program_start(void);
extern void user_program_end(void);

// Calculate program size
#define user_program_size ((uint32)&user_program_end - (uint32)&user_program_start)
#define user_program_code ((void*)&user_program_start)

#endif
