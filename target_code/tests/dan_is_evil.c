void printf();
int main() {
    int x;
    int *p;
    int **pp;
    int ***ppp;
    x = 100;
    p = &x - 1;
    pp = &p - 1;
    ppp = &pp - 1;
    printf("%d\n", *++*++*++ppp);
    return 0;
}
