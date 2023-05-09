extern void printf();
int main() {
    int i;
    for (i = 'a'; i <= 'z'; ++i) {
        printf("char = %c\n", i);
    }
    for (i = 'A'; i <= 'Z'; ++i) {
        printf("char = %c\n", i);
    }
    return 0;
}
