#include "../include/GPXParser.h"
#include <ctype.h>

int main(int argc, char **argv) {
    printf("TESTER\n");
    GPXdoc *doc = createGPXdoc(argv[1]);
    printf("namespace: %s\n", doc->namespace);
    printf("------------------------\n");
    char *besttt = GPXdocToString(doc);
    printf("%s\n", besttt);
    printf("------------------------\n");
    return 0;
}
