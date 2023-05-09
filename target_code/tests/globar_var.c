int g_data = 10;
int bss;

extern void printf();

int main() {
    int local;
    printf("g_data = %d", g_data++);
    printf("g_data = %d", g_data);
    bss = 1000;
    printf("bss = %d", bss--);
    printf("bss = %d", bss);

    local = 2000;
    printf("local = %d", local++);
    printf("local = %d", local);
}
