extern void printf();

int add(int a, int b) { return a + b; }

int main() {
    int result;
    result = add(1, 1);
    printf("1 + 1 = %d", result);
    return 0;
}
