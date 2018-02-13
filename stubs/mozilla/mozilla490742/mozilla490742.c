#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <sqlite3.h>

typedef struct stChildTransaction {
    int id;
    char sql[1000];
} ChildTransaction;

ChildTransaction **_childTransactions;
sqlite3 *db;
char *zErrMsg = NULL;
int rc;
int iNum;


static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");

    return 0;
}


void Create_ChildTransaction(ChildTransaction *pChild, char *Name, char *URL) {
    const char *sql1 = "BEGIN; INSERT INTO BOOKMARK (NAME,URL) VALUES ('";
    const char *sql2 = "', '";
    const char *sql3 = "' ); END TRANSACTION;";

    int length1 = strlen(sql1);
    int length2 = strlen(sql2);
    int length3 = strlen(sql3);

    int lengthName = strlen(Name);
    int lengthURL = strlen(URL);

    memset(pChild->sql, 0, 1000);
    memcpy(pChild->sql, sql1, strlen(sql1));
    memcpy(pChild->sql + length1, Name, strlen(Name));
    memcpy(pChild->sql + length1 + lengthName, sql2, strlen(sql2));
    memcpy(pChild->sql + length1 + lengthName + length2, URL, strlen(URL));
    memcpy(pChild->sql + length1 + lengthName + length2 + lengthURL, sql3, strlen(sql3));

    printf("%s\n", pChild->sql);
}

void doTransaction(ChildTransaction *pChild) {
    rc = sqlite3_exec(db, pChild->sql, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        printf("%s\n", pChild->sql);
        exit(-1);
    }
}

void placesCreateItemTransactions() {
    int id = 0;
    int i = 0;

    for (i = 0; i < iNum; i++) {
        ChildTransaction *pChild = _childTransactions[i];
        doTransaction(pChild);
    }
}

void InitDB() {
    const char *sql;
    const char *data = "Callback function called";

    rc = sqlite3_open(NULL, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    sql = "CREATE TABLE BOOKMARK(" \
          "NAME           TEXT PRIMARY KEY     NOT NULL," \
          "URL            TEXT    NOT NULL);";


    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(-1);
    }
}

void DumpDB() {
    const char *sql;
    const char *data = "Callback function called";

    sql = "SELECT * from BOOKMARK";
    rc = sqlite3_exec(db, sql, callback, (void *) data, &zErrMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        exit(-1);
    }
}

void CloseDB() {
    sqlite3_close(db);
}


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Input Parameter Number is Wrong\n");
        exit(-1);
    }

    int i;
    iNum = atoi(argv[1]);

    FILE *fr = fopen(argv[2], "rt");
    char line[100];
    char URL[50];
    char Name[50];

    int iInput = 0;

    _childTransactions = (ChildTransaction **) malloc(sizeof(ChildTransaction *) * iNum);

    while (fgets(line, 100, fr) != NULL) {
        sscanf(line, "%s %s", URL, Name);
        ChildTransaction *pChild = (ChildTransaction *) malloc(sizeof(ChildTransaction));
        Create_ChildTransaction(pChild, Name, URL);
        _childTransactions[iInput] = pChild;
        iInput++;
    }

    assert(iNum == iInput);
    fclose(fr);

    InitDB();
    placesCreateItemTransactions();

    DumpDB();
    CloseDB();

    for (i = 0; i < iNum; i++) {
        free(_childTransactions[i]);
    }

    free(_childTransactions);

}
