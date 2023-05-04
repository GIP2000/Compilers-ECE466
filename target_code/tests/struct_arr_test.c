struct X {
    int a;
    int b;
    int c[100];
};
extern void printf();

int main() {
    struct X x;
    x.a = 10;
    x.b = 100;
    int i;
    printf("did the first 2\n");
    printf("%d %d\n", x.a, x.b);

    for (i = 0; i < 100; ++i) {
        x.c[i] = 50;
    }

    printf("x.a = %d, x.b = %d x.c = { %d", x.a, x.b, x.c[0]);
    for (i = 1; i < 100; ++i) {
        printf(",%d ", x.c[i]);
    }
    printf("}");
}
