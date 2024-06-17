#ifndef COMMON_H
#define COMMON_H

#define BUFFER_SIZE 1024

typedef enum {
    NEW_PROGRAM = 0,
    CHECK_RESULT = 1,
    FIXED_PROGRAM = 2
} message_type_t;

typedef struct {
    message_type_t type;
    int sender_id;
    int receiver_id;
    char content[BUFFER_SIZE];
} message_t;

typedef enum {
    WRITING_PROGRAM,
    CHECKING_PROGRAM,
    FIXING_PROGRAM,
    SLEEPING
} programmer_state_t;

#endif // COMMON_H

