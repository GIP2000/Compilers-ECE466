extern void printf();
struct X {
    char c;
    char b;
    int a;
};

int main() {
    struct X x, y, z;
    x.c = 'a';
    x.a = 100;
    y.c = 'b';
    z.c = 'c';
    printf("%d %c %c %c sizeof X = %d\n", x.a, x.c, y.c, z.c, sizeof(struct X));
}
