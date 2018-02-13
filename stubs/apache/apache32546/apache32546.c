#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>


#define true 1
#define false 0

typedef char byte;

//InputStream and related functions
struct InputStream {
    FILE *fr;
};

int InputStream_read(struct InputStream *input, byte *buffer, int length) {
    return fread(buffer, sizeof(byte), length, input->fr);
}

void InputStream_InputStream(struct InputStream *input, char *sFileName) {
    input->fr = fopen(sFileName, "r");
}

void InputStream_destroy(struct InputStream *input) {
    fclose(input->fr);
}

//OutputStream and related functions
struct OutputStream {
    FILE *fw;
};

int OutputStream_write(struct OutputStream *output, byte *buffer, int idx, int length) {
    return fwrite(buffer + idx, sizeof(byte), length, output->fw);
}

void OutputStream_OutputStream(struct OutputStream *output, char *sFileName) {
    output->fw = fopen(sFileName, "w");
}

void OutputStream_destroy(struct OutputStream *output) {
    fclose(output->fw);
}

//GetMethod and related functions
struct GetMethod {
    int input;
};

void GetMethod_GetMethod(struct GetMethod *get) {
    get->input = 2048;
}

void GetMethod_copyRange(struct GetMethod *method, struct InputStream *istream, struct OutputStream *ostream) {
    // Copy the input stream to the output stream
    byte *buffer = (byte *) malloc(sizeof(byte) * method->input);
    int len = method->input;
    while (true) {
        len = InputStream_read(istream, buffer, method->input);
        if (len == 0)
            break;
        OutputStream_write(ostream, buffer, 0, len);
    }
    free(buffer);
}

void GetMethod_copy(struct InputStream *input, struct OutputStream *output) {
    struct GetMethod getMethod;
    GetMethod_GetMethod(&getMethod);
    GetMethod_copyRange(&getMethod, input, output);
}

void GetMethod_executeRequest(char *iFile, char *oFile) {
    struct InputStream input;
    InputStream_InputStream(&input, iFile);
    struct OutputStream output;
    OutputStream_OutputStream(&output, oFile);
    GetMethod_copy(&input, &output);

    OutputStream_destroy(&output);
    InputStream_destroy(&input);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("input parameters are not correct\n");
        exit(-1);
    }

    char *iFile = argv[1];
    char *oFile = argv[2];
    int iTimes = atoi(argv[3]);
    int i = 0;

    for (i = 0; i < iTimes; i++) {
        GetMethod_executeRequest(iFile, oFile);
    }

}
