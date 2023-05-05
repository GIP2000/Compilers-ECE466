extern void printf();
struct X {
    char c;
};

int main() {
    struct X x, y, z;
    x.c = 'a';
    y.c = 'b';
    z.c = 'c';
    printf("%c %c %c sizeof X = %d\n", x.c, y.c, z.c, sizeof(struct X));
}
