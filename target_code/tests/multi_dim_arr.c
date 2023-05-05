extern void printf();
int main() {
    int arr[10][50];
    int i, j;

    for (i = 0; i < 10; ++i) {
        for (j = 0; j < 50; ++j) {
            arr[i][j] = (i * 50) + j;
        }
    }
    for (i = 0; i < 10; ++i) {
        for (j = 0; j < 50; ++j) {
            printf("%d, ", arr[i][j]);
        }
    }
    printf("\n");
    return 0;
}
