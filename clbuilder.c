#include "cjson.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KEY_SIZE 40

struct key_t {
  char key[KEY_SIZE];
  char value[KEY_SIZE];
  struct key_t *next;
};

struct key_t *read_list(FILE *in) {
  char line[2 * KEY_SIZE + 5];
  char *comma;
  char *trail;

  struct key_t *TOP = NULL;

  while (!feof(in)) {
    fgets(line, sizeof(line), in);
    comma = strchr(line, ',');
    if (comma) {
      struct key_t *mbr = (struct key_t *)calloc(1, sizeof(struct key_t));
      strncpy(mbr->key, line, comma - line);
      strncpy(mbr->value, comma + 1, strlen(comma) - 1);

      trail = &mbr->value[strlen(mbr->value) - 1];
      if (isspace(*trail))
        *trail = 0;

      mbr->next = TOP;
      TOP = mbr;
    }
  }

  return TOP;
}

void generate_test(FILE *out, struct key_t *TOP) {
  struct key_t *cur = TOP;

  while (cur) {
    fprintf(out, "Assert.Equal(\"%s\", tbl[\"%s\"].ToString());\n", cur->value,
            cur->key);
    cur = cur->next;
  }
}

void generate_json(FILE *out, struct key_t *TOP, char *tablename) {
  struct key_t *cur = TOP;

  cJSON *doc = cJSON_CreateObject();
  cJSON_AddStringToObject(doc, "utcDateTime", "2017-12-18T12:48:22.8535780");
  cJSON_AddStringToObject(doc, "transactionId", "18122013");
  cJSON_AddStringToObject(doc, "operation", "I");
  cJSON_AddStringToObject(doc, "tableName", tablename);
  cJSON_AddStringToObject(doc, "schema", "C05DBSYNC");

  cJSON *uniqueKey = cJSON_CreateArray();
  cJSON_AddItemToObject(doc, "uniqueKey", uniqueKey);

  cJSON *key = cJSON_CreateObject();
  cJSON_AddStringToObject(key, "Name", "CourtType");
  cJSON_AddStringToObject(key, "dataType", "varchar");
  cJSON_AddNumberToObject(key, "length", 1);
  cJSON_AddNumberToObject(key, "precision", 0);
  cJSON_AddNumberToObject(key, "scale", 0);
  cJSON_AddItemToArray(uniqueKey, key);

  cJSON *columns = cJSON_AddArrayToObject(doc, "columns");
  while (cur) {
    cJSON *col = cJSON_CreateObject();
    cJSON_AddItemToArray(columns, col);

    cJSON_AddStringToObject(col, "name", cur->key);
    cJSON *val = cJSON_AddObjectToObject(col, "value");
    cJSON_AddStringToObject(val, "new", cur->value);
    cJSON_AddStringToObject(col, "dataType", "varchar");
    cJSON_AddNumberToObject(col, "length", 3);
    cJSON_AddNumberToObject(col, "precision", 0);
    cJSON_AddNumberToObject(col, "scale", 0);
    cur = cur->next;
  }

  fputs(cJSON_Print(doc), out);

  cJSON_Delete(doc);
}

int main(int argc, char **argv) {
  char *tablename = "TABLENAME";
  char *objectname = NULL;
  char testfilename[80];
  char jsonfilename[80];
  FILE *json = NULL;
  FILE *test = NULL;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--table") == 0) {
      tablename = argv[i + 1];
    }
    if (strcmp(argv[i], "--object") == 0) {
      objectname = argv[i + 1];
    }
  }

  if (NULL == tablename) {
    strcpy(testfilename, "test.cs");
  } else {
    snprintf(testfilename, sizeof(testfilename), "%s.cs", tablename);
  }

  if (NULL == objectname) {
    strcpy(jsonfilename, "changelog.json");
  } else {
    snprintf(jsonfilename, sizeof(jsonfilename), "%s.json", objectname);
  }

  struct key_t *keys = read_list(stdin);

  json = fopen(jsonfilename, "w");
  test = fopen(testfilename, "w");

  generate_json(json, keys, tablename);
  generate_test(test, keys);

  return 0;
}