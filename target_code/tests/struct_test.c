struct X {
    int a;
    int b;
    int c[100];
};

struct NO {
    char a;
    char b;
    int c;
};

extern void printf();

int main() {
    struct X x;

    int i;
    for (i = 0; i < 100; ++i) {
        x.c[i] = 100 * (i + 1);
    }

    printf("%d %d %d\n", x.c[0], x.c[1], x.c[2]);
    printf("NO: sizeof = %d\n", sizeof(struct NO));
    printf("X: sizeof = %d\n", sizeof(struct X));
}
