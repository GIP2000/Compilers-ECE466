struct X {
    int a;
    int b;
    int c[100];
};
extern void printf();

int main() {
    struct X x;

    int i;
    for (i = 0; i < 3; ++i) {
        x.c[i] = 100;
    }
    printf("%d %d %d\n", x.c[0], x.c[1], x.c[2]);
}
