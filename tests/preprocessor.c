#define ANSWER 42
#if 0
int ignored = 0;
#elif 1
int selected = ANSWER;
#else
int also_ignored = 0;
#endif
int main() { return selected; }
