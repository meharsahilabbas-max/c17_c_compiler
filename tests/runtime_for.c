int main(void) {
    int total = 0;
    for (int i = 0; i < 5; i = i + 1) total = total + i;
    if (total == 10) return 0;
    return 1;
}
