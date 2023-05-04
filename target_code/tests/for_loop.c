extern void printf();
int main() {
    int i;
    for (i = 0; i < 10; ++i) {
        printf("i = %d\n", i);
    }
    printf("end loop\n");
}
