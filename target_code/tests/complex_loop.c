extern void printf();
int a = 1;
int b = 2;
int main() {
    int i;
    if (a < b && a == 1) {
        for (i = 0; i < 10 || b == 2; ++i) {
            ++b;
        }
    }
    printf("a = %d\nb = %d\n", a, b);
}
