extern void printf();

int add(int a, int b) {
    printf("a = %d, b = %d\n", a, b);
    return a + b;
}

int main() {
    int result;
    result = add(10, 21);
    printf("result = %d\n", result);
    return 0;
}
