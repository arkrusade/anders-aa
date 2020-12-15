// #include <stdio.h>
int main() {
    // int a = -1;
    // int b = 1;
    // printf("hellow world\n");
    // int* bp = &b;
    // int* ap = &a;
    // ap = &b;
    // ap = 0;
    // int* p;
    // p = ap;
    // int ** c = NULL;
    int x, y, *p, **r, *q, **s;
    p = &x;
    r = &p;
    q = &y;
    s = &q;
    r = s;
    return 0;
}
