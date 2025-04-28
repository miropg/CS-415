int value = 5;    //Global
int main()
{
    int *p;       //stack
    p = (int*)malloc(sizeof(int)); //heap
    if (p == 0){
        printf("Error: Out of memory\n");
    return 1;
    }
    *p = value;
    printf("%d\n", *p);
    free(p);
    return 0;
}