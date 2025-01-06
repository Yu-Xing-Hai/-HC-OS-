#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H
void panic_spin(char* filename, int line, const char* func, const char* condition);  // the type of pointer can point a memory where the string is stored.

#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)  // __VA_ARGS__ : Unique identifiers. More means of __VA_ARGS__ can see the book page of P382

#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0) //When we define NDEBUG,the ASSERT's commend become 0.We can use gcc's commend of "-D" to define NDEBUG.
#else
    #define ASSERT(CONDITION)\
    if(CONDITION){}\
    else {      \
    /*"#": translate CONDITION to const string.*/ \
        PANIC(#CONDITION); \
    }
#endif /*__NDEBUG*/
#endif/*__KERNEL_DEBUG_H*/