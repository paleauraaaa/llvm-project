int other_global = 73;
static volatile int other_static = 91;

extern "C" int read_other_globals() { return other_global + other_static; }
