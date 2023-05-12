int arr[3];
extern void printf();
int main() {
    arr[0] = 10;
    arr[1] = 100;
    arr[2] = 1000;
    printf("{%d, %d, %d}\n", arr[0], arr[1], arr[2]);
}
