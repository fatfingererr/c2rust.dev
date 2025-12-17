char *get_str()
{
    char s[] = "hello";
    return s;
}

int main()
{
    char *str = get_str();
    return 0;
}
/*
cd examples\lifetime\elision_rules
gcc bad_ref.c -O3 -march=native -o bad_ref
./bad_ref

bad_ref.c: In function 'get_str':
bad_ref.c:4:12: warning: function returns address of local variable [-Wreturn-local-addr]
    4 |     return s;
      |            ^
*/