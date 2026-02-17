#include <stdio.h>
#include <ctype.h>
#include <string.h>

static const char *path_basename(const char *path)
{
    const char *base = path;
    const char *p = path;

    while(*p)
    {
        if(*p == '/' || *p == '\\')
            base = p + 1;
        p++;
    }

    return base;
}

int main(int argc, char **argv)
{
    FILE *ifp;
    FILE *ofp;
    char buffer[1024];
    char base[1024];

    if(argc < 3)
        return 1;

    char *inFile = argv[1];
    char *outFile = argv[2];

    ifp = fopen(inFile, "r");
    if(!ifp)
        return 2;

    ofp = fopen(outFile, "w");
    if(!ofp)
        return 3;

    // Strip extension from input filename for generated symbol name.
    snprintf(base, sizeof(base), "%s", path_basename(inFile));
    char *dot = strrchr(base, '.');
    if(dot)
        *dot = '\0';

    fprintf(ofp, "const char *fallbackShader_%s =\n", base);

    while(fgets(buffer, sizeof(buffer), ifp))
    {
        // Strip trailing whitespace from line
        char *end = buffer + strlen(buffer) - 1;
        while(end >= buffer && isspace(*end))
            end--;

        end[1] = '\0';

        // Write line enquoted, with a newline
        fprintf(ofp, "\"%s\\n\"\n", buffer);
    }

    fprintf(ofp, ";\n");

    fclose(ifp);
    fclose(ofp);

    return 0;
}
