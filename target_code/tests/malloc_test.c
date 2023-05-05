extern void *malloc(unsigned int);
extern void *realloc(void *, unsigned int);
extern void printf();

struct Data {
    char *first_name;
    char *last_name;
    int age;
};

struct DynamicDataArr {
    struct Data **arr;
    int cap;
    int len;
};

void init_arr(struct DynamicDataArr *arr) {
    printf("before local init\n");
    arr->cap = 100;
    arr->len = 0;
    printf("malloc -> size or arr = %d\n", sizeof(struct Data *) * arr->cap);
    arr->arr = malloc(sizeof(struct Data *) * arr->cap);
    printf("after malloc %p\n", arr->arr);
}

void append(struct DynamicDataArr *arr, struct Data *data) {
    if (arr->len >= arr->cap) {
        arr->cap = 2;
        arr->arr = realloc(arr->arr, sizeof(struct Data *) * arr->cap);
    }
    arr->arr[arr->len++] = data;
}

int main() {
    struct DynamicDataArr arr;
    printf("before init\n");
    init_arr(&arr);
    printf("after init\n");
    printf("arr cap %d\narr len %d\n", arr.cap, arr.len);

    printf("after malloc in main %p\n", arr.arr);

    struct Data person1;
    person1.first_name = "Greg";
    person1.last_name = "Presser";
    person1.age = 23;
    printf("before append\n");

    append(&arr, &person1);

    return 0;
}
