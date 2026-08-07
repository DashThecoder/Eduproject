//meow
/*meow*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include "cJSON.h"


#include <conio.h>
#include <io.h>


typedef struct{
    char term[16];
    char courseCode[32];
    double grade; 
} Enrollment;

typedef struct{
    char username[64]; 
    char password[64];
    char firstName[64];
    char lastName[64];
    char nationalId[32];
    char major[64]; 
    int entryYear;
    char degree[32];     
    char supervisor[64]; 
    char faculty[64];    
    char birthPlace[64];
    char firstSchool[64]; 
    char firstBike[64];
    char thesisTitle[128];
    char thesisAbstract[256];
    int thesisCitations;

    Enrollment enroll[100];
    int enrollCount;
} Student;

typedef struct{
    char username[64]; 
    char password[64];
    char firstName[64];
    char lastName[64];
    char nationalId[32];
    char major[64];
    int entryYear;
    char lastDegree[32];
    char faculty[64];
} Professor;

typedef struct{
    char name[96];
    char code[32];
    int credits;
    char prerequisites[128]; 
    char degree[32];
    char major[64];
    char faculty[64];
} Course;

typedef struct{
    char courseCode[32];
    char professorId[64];
    char term[16];
    int capacity;
    int enrolledCount;
    char faculty[64];
    char location[96];
    bool is_Approved;
} Offering;

enum{
    REQ_OFFER = 1,
    REQ_REMOVE = 2,
    REQ_CAPACITY = 3
};
enum{
    REQ_PENDING = 0,
    REQ_ACCEPTED = 1,
    REQ_REJECTED = 2
};

typedef struct{
    int type;
    char courseCode[32];
    char professorId[64];
    char term[16];
    int capacity; 
    int extra;    
    char location[96];
    int status;
} Request;

typedef struct{
    char term[16];
    char courseCode[32];
    char professorId[64];
    char studentId[64];
    int score;
} Survey;

typedef struct{
    char text[256];
    int type; 
    char options[4][128];
    int correct; 
} Question;

typedef struct{
    int id;
    int kind; 
    char term[16];
    char courseCode[32];
    char professorId[64];
    char title[128];
    double score;
    int qcount;
    Question q[10];
} Assignment;

typedef struct{
    int assignmentId;
    char studentId[64];
    int answers[10];    
    char text[10][256]; 
    int acount;
    double earned; 
} Submission;

enum{
    PH_OFFERING = 0,
    PH_SELECTION = 1,
    PH_CLASS = 2,
    PH_GRADE = 3,
    PH_COUNT = 4
};
enum{
    ST_DISABLED = 0,
    ST_ENABLED = 1,
    ST_FINISHED = 2
};

static const char *PHASE_NAME[PH_COUNT] = {"offering", "unit selection", "class & exams", "grade recording"};

static Student students[300];
static int student_count = 0;
static Professor professors[300];
static int prof_count = 0;
static Course courses[300];
static int course_count = 0;
static Offering offerings[300];
static int offering_count = 0;
static Request requests[300];
static int request_count = 0;
static Survey surveys[300];
static int survey_count = 0;
static Assignment assigns[300];
static int assign_count = 0;
static Submission subs[2000];
static int sub_count = 0;

static char currentTerm[16] = "14042";
static int phase[PH_COUNT] = {ST_DISABLED, ST_DISABLED, ST_DISABLED, ST_DISABLED};

static int listed[300];
static int listed_count = 0;

static void load_all(void);
static void save_all(void);
static void login_menu(void);
static void admin_dashboard(void);
static void faculty_dashboard(int p);
static void student_dashboard(int s);
static void forgot_password(void);

static void clear_screen(void)
{
    system("cls");
}

static void read_line(char *buf, size_t size){
    if (!fgets(buf, (int)size, stdin))
    {
        buf[0] = '\0';
        save_all();
        printf("\nInput stream closed. Data saved, goodbye.\n");
        exit(0);
    }
    buf[strcspn(buf, "\r\n")] = '\0';
}

static int read_int(void){
    char b[64], *end;
    long v;
    read_line(b, sizeof b);
    v = strtol(b, &end, 10);
    if (end == b)
        return INT_MIN;
    return (int)v;
}

static double read_double(void){
    char b[64], *end;
    double v;
    read_line(b, sizeof b);
    v = strtod(b, &end);
    if (end == b)
        return -1.0;
    return v;
}

static int input_is_console(void){
    return _isatty(_fileno(stdin));
}

static void read_password(char *buf, size_t size){
    if (!input_is_console()){
        read_line(buf, size);
        printf("\n");
        return;
    }
    size_t i = 0;
    int ch;
    while(1){
        ch = _getch();
        if (ch == '\r' || ch == '\n'){
            break;
        }
        if (ch == 8 || ch == 127){
            if (i > 0){
                i--;
                printf("\b \b");
            }
            continue;
        }
        if (ch == 0 || ch == 224){
            _getch();
            continue;
        } 
        if (i + 1 < size){
            buf[i++] = (char)ch;
            printf("*");
        }
    }
    buf[i] = '\0';
    printf("\n");
}

static void press_any_key(const char *msg){
    printf("%s", msg);
    fflush(stdout);
    if (input_is_console()){
        _getch();
    }
    else{
        getchar();
    }
}

static void trim(char *s){
    char *p = s;
    size_t n;
    while (*p == ' ' || *p == '\t')
        p++;
    if (p != s)
        memmove(s, p, strlen(p) + 1);
    n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n'))
        s[--n] = '\0';
}

static bool ci_equal(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return false;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static bool ci_contains(const char *hay, const char *needle)
{
    size_t n = strlen(needle);
    const char *p;
    if (n == 0)
        return true;
    for (p = hay; *p; p++)
    {
        size_t i = 0;
        while (i < n && p[i] &&
               tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i]))
            i++;
        if (i == n)
            return true;
    }
    return false;
}

static void copy_str(char *dst, size_t size, const char *src)
{
    size_t n;
    if (!src || size == 0)
    {
        if (size)
            dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= size)
        n = size - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void js_str(cJSON *o, const char *key, char *dst, size_t size)
{
    cJSON *it = cJSON_GetObjectItem(o, key);
    if (cJSON_IsString(it) && it->valuestring)
        copy_str(dst, size, it->valuestring);
    else
        dst[0] = '\0';
}

static int js_int(cJSON *o, const char *key, int def)
{
    cJSON *it = cJSON_GetObjectItem(o, key);
    if (cJSON_IsNumber(it))
        return it->valueint;
    return def;
}

static double js_num(cJSON *o, const char *key, double def)
{
    cJSON *it = cJSON_GetObjectItem(o, key);
    if (cJSON_IsNumber(it))
        return it->valuedouble;
    return def;
}

static bool js_bool(cJSON *o, const char *key, bool def)
{
    cJSON *it = cJSON_GetObjectItem(o, key);
    if (cJSON_IsBool(it))
        return cJSON_IsTrue(it);
    return def;
}

static char *read_file_to_string(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    long length;
    char *data;
    size_t got;
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (length < 0)
    {
        fclose(f);
        return NULL;
    }
    data = (char *)malloc((size_t)length + 1);
    if (!data)
    {
        fclose(f);
        return NULL;
    }
    got = fread(data, 1, (size_t)length, f);
    data[got] = '\0';
    fclose(f);
    return data;
}

static void write_json_file(const char *filename, cJSON *root)
{
    char *txt = cJSON_Print(root);
    FILE *f;
    if (!txt)
        return;
    f = fopen(filename, "w");
    if (f)
    {
        fputs(txt, f);
        fclose(f);
    }
    free(txt);
}

static int find_student(const char *id)
{
    int i;
    for (i = 0; i < student_count; i++)
        if (strcmp(students[i].username, id) == 0)
            return i;
    return -1;
}

static int find_prof(const char *id)
{
    int i;
    for (i = 0; i < prof_count; i++)
        if (strcmp(professors[i].username, id) == 0)
            return i;
    return -1;
}

static int find_course(const char *code)
{
    int i;
    for (i = 0; i < course_count; i++)
        if (strcmp(courses[i].code, code) == 0)
            return i;
    return -1;
}

static const char *course_name(const char *code)
{
    int i = find_course(code);
    return i >= 0 ? courses[i].name : code;
}

static int course_credits(const char *code)
{
    int i = find_course(code);
    return i >= 0 ? courses[i].credits : 0;
}

static void prof_full_name(const char *id, char *out, size_t size)
{
    int i = find_prof(id);
    if (i >= 0)
        snprintf(out, size, "Dr. %s %s", professors[i].firstName, professors[i].lastName);
    else
        snprintf(out, size, "%s", id);
}


static void next_term(const char *term, char *out, size_t size)
{
    size_t n = strlen(term);
    char year[16];
    int y, s;
    if (n < 2)
    {
        copy_str(out, size, term);
        return;
    }
    copy_str(year, sizeof year, term);
    year[n - 1] = '\0';
    y = atoi(year);
    s = term[n - 1] - '0';
    if (s >= 2)
    {
        y += 1;
        s = 1;
    }
    else
    {
        s += 1;
    }
    snprintf(out, size, "%d%d", y, s);
}

static void load_students(void)
{
    char *data = read_file_to_string("students.json");
    cJSON *json, *item;
    student_count = 0;
    if (!data)
        return;
    json = cJSON_Parse(data);
    if (json && cJSON_IsArray(json))
    {
        cJSON_ArrayForEach(item, json)
        {
            Student *s;
            cJSON *eo, *termNode;
            if (student_count >= 300)
                break;
            s = &students[student_count];
            memset(s, 0, sizeof *s);
            js_str(item, "username", s->username, sizeof s->username);
            js_str(item, "password", s->password, sizeof s->password);
            js_str(item, "firstName", s->firstName, sizeof s->firstName);
            js_str(item, "lastName", s->lastName, sizeof s->lastName);
            js_str(item, "nationalId", s->nationalId, sizeof s->nationalId);
            js_str(item, "major", s->major, sizeof s->major);
            s->entryYear = js_int(item, "entryYear", 0);
            js_str(item, "degree", s->degree, sizeof s->degree);
            js_str(item, "supervisor", s->supervisor, sizeof s->supervisor);
            js_str(item, "faculty", s->faculty, sizeof s->faculty);
            js_str(item, "birthPlace", s->birthPlace, sizeof s->birthPlace);
            js_str(item, "firstSchool", s->firstSchool, sizeof s->firstSchool);
            js_str(item, "firstBike", s->firstBike, sizeof s->firstBike);
            js_str(item, "thesisTitle", s->thesisTitle, sizeof s->thesisTitle);
            js_str(item, "thesisAbstract", s->thesisAbstract, sizeof s->thesisAbstract);
            s->thesisCitations = js_int(item, "thesisCitations", 0);

            eo = cJSON_GetObjectItem(item, "enrolled_offerings");
            if (eo && cJSON_IsObject(eo))
            {
                cJSON_ArrayForEach(termNode, eo)
                {
                    cJSON *courseNode;
                    if (!termNode->string)
                        continue;
                    cJSON_ArrayForEach(courseNode, termNode)
                    {
                        if (s->enrollCount >= 300)
                            break;
                        if (!courseNode->string)
                            continue;
                        copy_str(s->enroll[s->enrollCount].term, 16, termNode->string);
                        copy_str(s->enroll[s->enrollCount].courseCode, 32, courseNode->string);
                        s->enroll[s->enrollCount].grade =
                            cJSON_IsNumber(courseNode) ? courseNode->valuedouble : -1.0;
                        s->enrollCount++;
                    }
                }
            }
            student_count++;
        }
    }
    cJSON_Delete(json);
    free(data);
}

static void load_professors(void)
{
    char *data = read_file_to_string("faculty.json");
    cJSON *json, *item;
    prof_count = 0;
    if (!data)
        return;
    json = cJSON_Parse(data);
    if (json && cJSON_IsArray(json))
    {
        cJSON_ArrayForEach(item, json)
        {
            Professor *p;
            if (prof_count >= 300)
                break;
            p = &professors[prof_count];
            memset(p, 0, sizeof *p);
            js_str(item, "username", p->username, sizeof p->username);
            js_str(item, "password", p->password, sizeof p->password);
            js_str(item, "firstName", p->firstName, sizeof p->firstName);
            js_str(item, "lastName", p->lastName, sizeof p->lastName);
            js_str(item, "nationalId", p->nationalId, sizeof p->nationalId);
            js_str(item, "major", p->major, sizeof p->major);
            p->entryYear = js_int(item, "entryYear", 0);
            js_str(item, "lastDegree", p->lastDegree, sizeof p->lastDegree);
            js_str(item, "faculty", p->faculty, sizeof p->faculty);
            prof_count++;
        }
    }
    cJSON_Delete(json);
    free(data);
}

static void load_courses(void)
{
    char *data = read_file_to_string("courses.json");
    cJSON *json, *item;
    course_count = 0;
    if (!data)
        return;
    json = cJSON_Parse(data);
    if (json && cJSON_IsArray(json))
    {
        cJSON_ArrayForEach(item, json)
        {
            Course *c;
            if (course_count >= 300)
                break;
            c = &courses[course_count];
            memset(c, 0, sizeof *c);
            js_str(item, "name", c->name, sizeof c->name);
            js_str(item, "code", c->code, sizeof c->code);
            c->credits = js_int(item, "credits", 0);
            js_str(item, "prerequisites", c->prerequisites, sizeof c->prerequisites);
            js_str(item, "degree", c->degree, sizeof c->degree);
            js_str(item, "major", c->major, sizeof c->major);
            js_str(item, "faculty", c->faculty, sizeof c->faculty);
            course_count++;
        }
    }
    cJSON_Delete(json);
    free(data);
}

static void load_offerings(void)
{
    char *data = read_file_to_string("offerings.json");
    cJSON *json, *item;
    offering_count = 0;
    if (!data)
        return;
    json = cJSON_Parse(data);
    if (json && cJSON_IsArray(json))
    {
        cJSON_ArrayForEach(item, json)
        {
            Offering *o;
            if (offering_count >= 300)
                break;
            o = &offerings[offering_count];
            memset(o, 0, sizeof *o);
            js_str(item, "courseCode", o->courseCode, sizeof o->courseCode);
            js_str(item, "professorId", o->professorId, sizeof o->professorId);
            js_str(item, "term", o->term, sizeof o->term);
            o->capacity = js_int(item, "capacity", 0);
            o->enrolledCount = js_int(item, "enrolledCount", 0);
            js_str(item, "faculty", o->faculty, sizeof o->faculty);
            js_str(item, "location", o->location, sizeof o->location);
            o->is_Approved = js_bool(item, "is_Approved", true);
            offering_count++;
        }
    }
    cJSON_Delete(json);
    free(data);
}

static void load_requests(void)
{
    char *data = read_file_to_string("requests.json");
    cJSON *json, *item;
    request_count = 0;
    if (!data)
        return;
    json = cJSON_Parse(data);
    if (json && cJSON_IsArray(json))
    {
        cJSON_ArrayForEach(item, json)
        {
            Request *r;
            if (request_count >= 300)
                break;
            r = &requests[request_count];
            memset(r, 0, sizeof *r);
            r->type = js_int(item, "type", REQ_OFFER);
            js_str(item, "courseCode", r->courseCode, sizeof r->courseCode);
            js_str(item, "professorId", r->professorId, sizeof r->professorId);
            js_str(item, "term", r->term, sizeof r->term);
            r->capacity = js_int(item, "capacity", 0);
            r->extra = js_int(item, "extra", 0);
            js_str(item, "location", r->location, sizeof r->location);
            r->status = js_int(item, "status", REQ_PENDING);
            request_count++;
        }
    }
    cJSON_Delete(json);
    free(data);
}

static void load_calendar(void)
{
    char *data = read_file_to_string("calendar.json");
    cJSON *json, *ph;
    int i;
    if (!data)
        return;
    json = cJSON_Parse(data);
    if (json)
    {
        js_str(json, "term", currentTerm, sizeof currentTerm);
        if (currentTerm[0] == '\0')
            copy_str(currentTerm, sizeof currentTerm, "14042");
        ph = cJSON_GetObjectItem(json, "phases");
        if (ph && cJSON_IsArray(ph))
        {
            for (i = 0; i < PH_COUNT && i < cJSON_GetArraySize(ph); i++)
            {
                cJSON *v = cJSON_GetArrayItem(ph, i);
                if (cJSON_IsNumber(v))
                    phase[i] = v->valueint;
            }
        }
    }
    cJSON_Delete(json);
    free(data);
}

static void load_surveys(void)
{
    char *data = read_file_to_string("surveys.json");
    cJSON *json, *item;
    survey_count = 0;
    if (!data)
        return;
    json = cJSON_Parse(data);
    if (json && cJSON_IsArray(json))
    {
        cJSON_ArrayForEach(item, json)
        {
            Survey *s;
            if (survey_count >= 300)
                break;
            s = &surveys[survey_count];
            memset(s, 0, sizeof *s);
            js_str(item, "term", s->term, sizeof s->term);
            js_str(item, "courseCode", s->courseCode, sizeof s->courseCode);
            js_str(item, "professorId", s->professorId, sizeof s->professorId);
            js_str(item, "studentId", s->studentId, sizeof s->studentId);
            s->score = js_int(item, "score", 0);
            survey_count++;
        }
    }
    cJSON_Delete(json);
    free(data);
}

static void load_lms(void)
{
    char *data = read_file_to_string("lms.json");
    cJSON *json, *arr, *item;
    assign_count = 0;
    sub_count = 0;
    if (!data)
        return;
    json = cJSON_Parse(data);
    if (json)
    {
        arr = cJSON_GetObjectItem(json, "assignments");
        if (arr && cJSON_IsArray(arr))
        {
            cJSON_ArrayForEach(item, arr)
            {
                Assignment *a;
                cJSON *qs, *q;
                if (assign_count >= 300)
                    break;
                a = &assigns[assign_count];
                memset(a, 0, sizeof *a);
                a->id = js_int(item, "id", assign_count + 1);
                a->kind = js_int(item, "kind", 0);
                js_str(item, "term", a->term, sizeof a->term);
                js_str(item, "courseCode", a->courseCode, sizeof a->courseCode);
                js_str(item, "professorId", a->professorId, sizeof a->professorId);
                js_str(item, "title", a->title, sizeof a->title);
                a->score = js_num(item, "score", 0);
                qs = cJSON_GetObjectItem(item, "questions");
                if (qs && cJSON_IsArray(qs))
                {
                    cJSON_ArrayForEach(q, qs)
                    {
                        Question *qq;
                        cJSON *opts;
                        int k;
                        if (a->qcount >= 300)
                            break;
                        qq = &a->q[a->qcount];
                        memset(qq, 0, sizeof *qq);
                        js_str(q, "text", qq->text, sizeof qq->text);
                        qq->type = js_int(q, "type", 0);
                        qq->correct = js_int(q, "correct", 0);
                        opts = cJSON_GetObjectItem(q, "options");
                        if (opts && cJSON_IsArray(opts))
                            for (k = 0; k < 4 && k < cJSON_GetArraySize(opts); k++)
                            {
                                cJSON *o = cJSON_GetArrayItem(opts, k);
                                if (cJSON_IsString(o))
                                    copy_str(qq->options[k], 128, o->valuestring);
                            }
                        a->qcount++;
                    }
                }
                assign_count++;
            }
        }
        arr = cJSON_GetObjectItem(json, "submissions");
        if (arr && cJSON_IsArray(arr))
        {
            cJSON_ArrayForEach(item, arr)
            {
                Submission *sb;
                cJSON *ans, *v;
                if (sub_count >= 300)
                    break;
                sb = &subs[sub_count];
                memset(sb, 0, sizeof *sb);
                sb->assignmentId = js_int(item, "assignmentId", 0);
                js_str(item, "studentId", sb->studentId, sizeof sb->studentId);
                sb->earned = js_num(item, "earned", 0);
                ans = cJSON_GetObjectItem(item, "answers");
                if (ans && cJSON_IsArray(ans))
                {
                    cJSON_ArrayForEach(v, ans)
                    {
                        if (sb->acount >= 300)
                            break;
                        if (cJSON_IsNumber(v))
                            sb->answers[sb->acount] = v->valueint;
                        else if (cJSON_IsString(v))
                        {
                            sb->answers[sb->acount] = -1;
                            copy_str(sb->text[sb->acount], 256, v->valuestring);
                        }
                        sb->acount++;
                    }
                }
                sub_count++;
            }
        }
    }
    cJSON_Delete(json);
    free(data);
}

static void load_all(void)
{
    load_students();
    load_professors();
    load_courses();
    load_offerings();
    load_requests();
    load_calendar();
    load_surveys();
    load_lms();
}

static void save_students(void)
{
    cJSON *arr = cJSON_CreateArray();
    int i, j;
    for (i = 0; i < student_count; i++)
    {
        Student *s = &students[i];
        cJSON *o = cJSON_CreateObject();
        cJSON *eo;
        cJSON_AddStringToObject(o, "username", s->username);
        cJSON_AddStringToObject(o, "password", s->password);
        cJSON_AddStringToObject(o, "firstName", s->firstName);
        cJSON_AddStringToObject(o, "lastName", s->lastName);
        cJSON_AddStringToObject(o, "nationalId", s->nationalId);
        cJSON_AddStringToObject(o, "major", s->major);
        cJSON_AddNumberToObject(o, "entryYear", s->entryYear);
        cJSON_AddStringToObject(o, "degree", s->degree);
        cJSON_AddStringToObject(o, "supervisor", s->supervisor);
        cJSON_AddStringToObject(o, "faculty", s->faculty);
        cJSON_AddStringToObject(o, "birthPlace", s->birthPlace);
        cJSON_AddStringToObject(o, "firstSchool", s->firstSchool);
        cJSON_AddStringToObject(o, "firstBike", s->firstBike);
        if (strcmp(s->degree, "PhD") == 0)
        {
            cJSON_AddStringToObject(o, "thesisTitle", s->thesisTitle);
            cJSON_AddStringToObject(o, "thesisAbstract", s->thesisAbstract);
            cJSON_AddNumberToObject(o, "thesisCitations", s->thesisCitations);
        }
        eo = cJSON_CreateObject();
        for (j = 0; j < s->enrollCount; j++)
        {
            cJSON *t = cJSON_GetObjectItem(eo, s->enroll[j].term);
            if (!t)
            {
                t = cJSON_CreateObject();
                cJSON_AddItemToObject(eo, s->enroll[j].term, t);
            }
            cJSON_AddNumberToObject(t, s->enroll[j].courseCode, s->enroll[j].grade);
        }
        cJSON_AddItemToObject(o, "enrolled_offerings", eo);
        cJSON_AddItemToArray(arr, o);
    }
    write_json_file("students.json", arr);
    cJSON_Delete(arr);
}

static void save_professors(void)
{
    cJSON *arr = cJSON_CreateArray();
    int i;
    for (i = 0; i < prof_count; i++)
    {
        Professor *p = &professors[i];
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "username", p->username);
        cJSON_AddStringToObject(o, "password", p->password);
        cJSON_AddStringToObject(o, "firstName", p->firstName);
        cJSON_AddStringToObject(o, "lastName", p->lastName);
        cJSON_AddStringToObject(o, "nationalId", p->nationalId);
        cJSON_AddStringToObject(o, "major", p->major);
        cJSON_AddNumberToObject(o, "entryYear", p->entryYear);
        cJSON_AddStringToObject(o, "lastDegree", p->lastDegree);
        cJSON_AddStringToObject(o, "faculty", p->faculty);
        cJSON_AddItemToArray(arr, o);
    }
    write_json_file("faculty.json", arr);
    cJSON_Delete(arr);
}

static void save_courses(void)
{
    cJSON *arr = cJSON_CreateArray();
    int i;
    for (i = 0; i < course_count; i++)
    {
        Course *c = &courses[i];
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "name", c->name);
        cJSON_AddStringToObject(o, "code", c->code);
        cJSON_AddNumberToObject(o, "credits", c->credits);
        cJSON_AddStringToObject(o, "prerequisites", c->prerequisites);
        cJSON_AddStringToObject(o, "degree", c->degree);
        cJSON_AddStringToObject(o, "major", c->major);
        cJSON_AddStringToObject(o, "faculty", c->faculty);
        cJSON_AddItemToArray(arr, o);
    }
    write_json_file("courses.json", arr);
    cJSON_Delete(arr);
}

static void save_offerings(void)
{
    cJSON *arr = cJSON_CreateArray();
    int i;
    for (i = 0; i < offering_count; i++)
    {
        Offering *f = &offerings[i];
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "courseCode", f->courseCode);
        cJSON_AddStringToObject(o, "professorId", f->professorId);
        cJSON_AddStringToObject(o, "term", f->term);
        cJSON_AddNumberToObject(o, "capacity", f->capacity);
        cJSON_AddNumberToObject(o, "enrolledCount", f->enrolledCount);
        cJSON_AddStringToObject(o, "faculty", f->faculty);
        cJSON_AddStringToObject(o, "location", f->location);
        cJSON_AddBoolToObject(o, "is_Approved", f->is_Approved);
        cJSON_AddItemToArray(arr, o);
    }
    write_json_file("offerings.json", arr);
    cJSON_Delete(arr);
}

static void save_requests(void)
{
    cJSON *arr = cJSON_CreateArray();
    int i;
    for (i = 0; i < request_count; i++)
    {
        Request *r = &requests[i];
        cJSON *o = cJSON_CreateObject();
        cJSON_AddNumberToObject(o, "type", r->type);
        cJSON_AddStringToObject(o, "courseCode", r->courseCode);
        cJSON_AddStringToObject(o, "professorId", r->professorId);
        cJSON_AddStringToObject(o, "term", r->term);
        cJSON_AddNumberToObject(o, "capacity", r->capacity);
        cJSON_AddNumberToObject(o, "extra", r->extra);
        cJSON_AddStringToObject(o, "location", r->location);
        cJSON_AddNumberToObject(o, "status", r->status);
        cJSON_AddItemToArray(arr, o);
    }
    write_json_file("requests.json", arr);
    cJSON_Delete(arr);
}

static void save_calendar(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    int i;
    cJSON_AddStringToObject(root, "term", currentTerm);
    for (i = 0; i < PH_COUNT; i++)
        cJSON_AddItemToArray(arr, cJSON_CreateNumber(phase[i]));
    cJSON_AddItemToObject(root, "phases", arr);
    write_json_file("calendar.json", root);
    cJSON_Delete(root);
}

static void save_surveys(void)
{
    cJSON *arr = cJSON_CreateArray();
    int i;
    for (i = 0; i < survey_count; i++)
    {
        Survey *s = &surveys[i];
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "term", s->term);
        cJSON_AddStringToObject(o, "courseCode", s->courseCode);
        cJSON_AddStringToObject(o, "professorId", s->professorId);
        cJSON_AddStringToObject(o, "studentId", s->studentId);
        cJSON_AddNumberToObject(o, "score", s->score);
        cJSON_AddItemToArray(arr, o);
    }
    write_json_file("surveys.json", arr);
    cJSON_Delete(arr);
}

static void save_lms(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *as = cJSON_CreateArray();
    cJSON *ss = cJSON_CreateArray();
    int i, j, k;
    for (i = 0; i < assign_count; i++)
    {
        Assignment *a = &assigns[i];
        cJSON *o = cJSON_CreateObject();
        cJSON *qs = cJSON_CreateArray();
        cJSON_AddNumberToObject(o, "id", a->id);
        cJSON_AddNumberToObject(o, "kind", a->kind);
        cJSON_AddStringToObject(o, "term", a->term);
        cJSON_AddStringToObject(o, "courseCode", a->courseCode);
        cJSON_AddStringToObject(o, "professorId", a->professorId);
        cJSON_AddStringToObject(o, "title", a->title);
        cJSON_AddNumberToObject(o, "score", a->score);
        for (j = 0; j < a->qcount; j++)
        {
            cJSON *q = cJSON_CreateObject();
            cJSON *opts = cJSON_CreateArray();
            cJSON_AddStringToObject(q, "text", a->q[j].text);
            cJSON_AddNumberToObject(q, "type", a->q[j].type);
            cJSON_AddNumberToObject(q, "correct", a->q[j].correct);
            for (k = 0; k < 4; k++)
                cJSON_AddItemToArray(opts, cJSON_CreateString(a->q[j].options[k]));
            cJSON_AddItemToObject(q, "options", opts);
            cJSON_AddItemToArray(qs, q);
        }
        cJSON_AddItemToObject(o, "questions", qs);
        cJSON_AddItemToArray(as, o);
    }
    for (i = 0; i < sub_count; i++)
    {
        Submission *sb = &subs[i];
        cJSON *o = cJSON_CreateObject();
        cJSON *ans = cJSON_CreateArray();
        cJSON_AddNumberToObject(o, "assignmentId", sb->assignmentId);
        cJSON_AddStringToObject(o, "studentId", sb->studentId);
        cJSON_AddNumberToObject(o, "earned", sb->earned);
        for (j = 0; j < sb->acount; j++)
        {
            if (sb->answers[j] >= 0)
                cJSON_AddItemToArray(ans, cJSON_CreateNumber(sb->answers[j]));
            else
                cJSON_AddItemToArray(ans, cJSON_CreateString(sb->text[j]));
        }
        cJSON_AddItemToObject(o, "answers", ans);
        cJSON_AddItemToArray(ss, o);
    }
    cJSON_AddItemToObject(root, "assignments", as);
    cJSON_AddItemToObject(root, "submissions", ss);
    write_json_file("lms.json", root);
    cJSON_Delete(root);
}

static void save_all(void)
{
    save_students();
    save_professors();
    save_courses();
    save_offerings();
    save_requests();
    save_calendar();
    save_surveys();
    save_lms();
}

static void print_student_header(void)
{
    printf("| %-12s | %-12s | %-10s | %-12s | %-22s | %-13s | %-7s | %-16s | %-22s | %-10s | %-18s | %-8s |\n",
           "first name", "last name", "student id", "national code", "field",
           "entrance year", "section", "mentor", "department",
           "answer 1", "answer 2", "answer 3");
    printf("|--------------|--------------|------------|--------------|"
           "------------------------|---------------|---------|"
           "------------------|------------------------|------------|"
           "--------------------|----------|\n");
}

static void print_student_row(const Student *s)
{
    printf("| %-12s | %-12s | %-10s | %-12s | %-22s | %-13d | %-7s | %-16s | %-22s | %-10s | %-18s | %-8s |\n",
           s->firstName, s->lastName, s->username, s->nationalId, s->major,
           s->entryYear, s->degree, s->supervisor, s->faculty,
           s->birthPlace, s->firstSchool, s->firstBike);
}

static void print_prof_header(void)
{
    printf("| %-12s | %-12s | %-10s | %-12s | %-22s | %-13s | %-12s | %-22s |\n",
           "first name", "last name", "faculty id", "national code", "field",
           "entrance year", "last degree", "department");
    printf("|--------------|--------------|------------|--------------|"
           "------------------------|---------------|--------------|"
           "------------------------|\n");
}

static void print_prof_row(const Professor *p)
{
    printf("| %-12s | %-12s | %-10s | %-12s | %-22s | %-13d | %-12s | %-22s |\n",
           p->firstName, p->lastName, p->username, p->nationalId, p->major,
           p->entryYear, p->lastDegree, p->faculty);
}

static void print_course_header(void)
{
    printf("| %-28s | %-9s | %-5s | %-34s | %-7s | %-22s | %-22s |\n",
           "course name", "course id", "units", "prerequisites (separated by comma)",
           "section", "field", "department");
    printf("|------------------------------|-----------|-------|"
           "------------------------------------|---------|"
           "------------------------|------------------------|\n");
}

static void print_course_row(const Course *c)
{
    printf("| %-28s | %-9s | %-5d | %-34s | %-7s | %-22s | %-22s |\n",
           c->name, c->code, c->credits,
           c->prerequisites[0] ? c->prerequisites : "-",
           c->degree, c->major, c->faculty);
}

/* who = 0 -> faculty id column, who = 1 -> faculty name column */
static void print_offering_header(int who)
{
    printf("| %-6s | %-28s | %-9s | %-20s | %-8s | %-8s | %-15s | %-22s | %-28s |\n",
           "number", "course name", "course id",
           who ? "faculty name" : "faculty id",
           "semester", "capacity", "no. enrollments", "department", "place");
    printf("|--------|------------------------------|-----------|"
           "----------------------|----------|----------|"
           "-----------------|------------------------|"
           "------------------------------|\n");
}

static void print_offering_row(int number, const Offering *o, int who)
{
    char fac[96];
    if (who)
        prof_full_name(o->professorId, fac, sizeof fac);
    else
        copy_str(fac, sizeof fac, o->professorId);
    printf("| %-6d | %-28s | %-9s | %-20s | %-8s | %-8d | %-15d | %-22s | %-28s |\n",
           number, course_name(o->courseCode), o->courseCode, fac, o->term,
           o->capacity, o->enrolledCount, o->faculty, o->location);
}

static bool student_field_match(const Student *s, int field, const char *phrase)
{
    char buf[32];
    switch (field)
    {
    case 1:
        return ci_contains(s->firstName, phrase);
    case 2:
        return ci_contains(s->lastName, phrase);
    case 3:
        return ci_contains(s->username, phrase);
    case 4:
        return ci_contains(s->major, phrase);
    case 5:
        snprintf(buf, sizeof buf, "%d", s->entryYear);
        return ci_contains(buf, phrase);
    case 6:
        return ci_contains(s->degree, phrase);
    case 7:
        return ci_contains(s->faculty, phrase);
    default:
        return false;
    }
}

static void search_students(void)
{
    int opt, i, found = 0;
    char phrase[128];
    printf("\nSearch:\n");
    printf("1. Search by first name\n");
    printf("2. Search by last name\n");
    printf("3. Search by student id\n");
    printf("4. Search by field\n");
    printf("5. Search by entrance year\n");
    printf("6. Search by section\n");
    printf("7. Search by department\n");
    printf("8. Go back\n");
    printf("Enter an option: ");
    opt = read_int();
    if (opt < 1 || opt > 7)
        return;
    printf("The phrase to search: ");
    read_line(phrase, sizeof phrase);
    printf("\n");
    print_student_header();
    for (i = 0; i < student_count; i++)
        if (student_field_match(&students[i], opt, phrase))
        {
            print_student_row(&students[i]);
            found++;
        }
    if (!found)
        printf("No matching student found.\n");
    press_any_key("\nPress any key to go back...");
}

static bool prof_field_match(const Professor *p, int field, const char *phrase)
{
    char buf[32];
    switch (field)
    {
    case 1:
        return ci_contains(p->firstName, phrase);
    case 2:
        return ci_contains(p->lastName, phrase);
    case 3:
        return ci_contains(p->username, phrase);
    case 4:
        return ci_contains(p->major, phrase);
    case 5:
        snprintf(buf, sizeof buf, "%d", p->entryYear);
        return ci_contains(buf, phrase);
    case 6:
        return ci_contains(p->lastDegree, phrase);
    case 7:
        return ci_contains(p->faculty, phrase);
    default:
        return false;
    }
}

static void search_professors(void)
{
    int opt, i, found = 0;
    char phrase[128];
    printf("\nSearch:\n");
    printf("1. Search by first name\n");
    printf("2. Search by last name\n");
    printf("3. Search by faculty id\n");
    printf("4. Search by field\n");
    printf("5. Search by entrance year\n");
    printf("6. Search by last degree\n");
    printf("7. Search by department\n");
    printf("8. Go back\n");
    printf("Enter an option: ");
    opt = read_int();
    if (opt < 1 || opt > 7)
        return;
    printf("The phrase to search: ");
    read_line(phrase, sizeof phrase);
    printf("\n");
    print_prof_header();
    for (i = 0; i < prof_count; i++)
        if (prof_field_match(&professors[i], opt, phrase))
        {
            print_prof_row(&professors[i]);
            found++;
        }
    if (!found)
        printf("No matching faculty member found.\n");
    press_any_key("\nPress any key to go back...");
}

static bool course_field_match(const Course *c, int field, const char *phrase)
{
    char buf[32];
    switch (field)
    {
    case 1:
        return ci_contains(c->name, phrase);
    case 2:
        return ci_contains(c->code, phrase);
    case 3:
        snprintf(buf, sizeof buf, "%d", c->credits);
        return ci_contains(buf, phrase);
    case 4:
        return ci_contains(c->degree, phrase);
    case 5:
        return ci_contains(c->major, phrase);
    case 6:
        return ci_contains(c->faculty, phrase);
    default:
        return false;
    }
}

static void search_courses(void)
{
    int opt, i, found = 0;
    char phrase[128];
    printf("\nSearch:\n");
    printf("1. Search by course name\n");
    printf("2. Search by course id\n");
    printf("3. Search by units\n");
    printf("4. Search by section\n");
    printf("5. Search by field\n");
    printf("6. Search by department\n");
    printf("7. Go back\n");
    printf("Enter an option: ");
    opt = read_int();
    if (opt < 1 || opt > 6)
        return;
    printf("The phrase to search: ");
    read_line(phrase, sizeof phrase);
    printf("\n");
    print_course_header();
    for (i = 0; i < course_count; i++)
        if (course_field_match(&courses[i], opt, phrase))
        {
            print_course_row(&courses[i]);
            found++;
        }
    if (!found)
        printf("No matching course found.\n");
    press_any_key("\nPress any key to go back...");
}

static bool offering_field_match(const Offering *o, int field, const char *phrase)
{
    char fac[96];
    switch (field)
    {
    case 1:
        return ci_contains(course_name(o->courseCode), phrase);
    case 2:
        return ci_contains(o->courseCode, phrase);
    case 3:
        prof_full_name(o->professorId, fac, sizeof fac);
        return ci_contains(fac, phrase) || ci_contains(o->professorId, phrase);
    case 4:
        return ci_contains(o->faculty, phrase);
    case 5:
        return ci_contains(o->location, phrase);
    default:
        return false;
    }
}

static void search_offerings(const char *term, int who)
{
    int opt, i, n = 0;
    char phrase[128];
    printf("\nSearch:\n");
    printf("1. Search by course name\n");
    printf("2. Search by course id\n");
    printf("3. Search by faculty\n");
    printf("4. Search by department\n");
    printf("5. Search by place\n");
    printf("6. Go back\n");
    printf("Enter an option: ");
    opt = read_int();
    if (opt < 1 || opt > 5)
        return;
    printf("The phrase to search: ");
    read_line(phrase, sizeof phrase);
    printf("\n");
    print_offering_header(who);
    for (i = 0; i < offering_count; i++)
    {
        if (term && strcmp(offerings[i].term, term) != 0)
            continue;
        if (!offerings[i].is_Approved)
            continue;
        if (offering_field_match(&offerings[i], opt, phrase))
            print_offering_row(++n, &offerings[i], who);
    }
    if (!n)
        printf("No matching offering found.\n");
    press_any_key("\nPress any key to go back...");
}

static void list_offerings_of_term(const char *term, int who)
{
    int i;
    listed_count = 0;
    printf("List of offerings - %s\n", term);
    print_offering_header(who);
    for (i = 0; i < offering_count; i++)
    {
        if (strcmp(offerings[i].term, term) != 0)
            continue;
        if (!offerings[i].is_Approved)
            continue;
        listed[listed_count] = i;
        print_offering_row(listed_count + 1, &offerings[i], who);
        listed_count++;
    }
    if (listed_count == 0)
        printf("(no offering registered for this semester)\n");
}

static void list_all_courses(void)
{
    int i;
    printf("List of courses\n");
    print_course_header();
    for (i = 0; i < course_count; i++)
        print_course_row(&courses[i]);
    if (course_count == 0)
        printf("(no course registered)\n");
}

static int find_enrollment(const Student *s, const char *term, const char *code)
{
    int i;
    for (i = 0; i < s->enrollCount; i++)
        if (strcmp(s->enroll[i].term, term) == 0 && strcmp(s->enroll[i].courseCode, code) == 0)
            return i;
    return -1;
}

static bool has_passed(const Student *s, const char *code)
{
    int i;
    for (i = 0; i < s->enrollCount; i++)
        if (strcmp(s->enroll[i].courseCode, code) == 0 && s->enroll[i].grade >= 10.0)
            return true;
    return false;
}

static bool prerequisites_ok(const Student *s, const Course *c, char *missing, size_t size)
{
    char buf[128], *tok;
    bool ok = true;
    missing[0] = '\0';
    if (c->prerequisites[0] == '\0' || strcmp(c->prerequisites, "-") == 0)
        return true;
    copy_str(buf, sizeof buf, c->prerequisites);
    tok = strtok(buf, ",");
    while (tok)
    {
        trim(tok);
        if (tok[0] && !has_passed(s, tok))
        {
            ok = false;
            if (missing[0])
                strncat(missing, ", ", size - strlen(missing) - 1);
            strncat(missing, tok, size - strlen(missing) - 1);
        }
        tok = strtok(NULL, ",");
    }
    return ok;
}

static double term_gpa(const Student *s, const char *term, int *enrolled, int *passed, int *failed)
{
    double sum = 0;
    int units = 0, i;
    if (enrolled)
        *enrolled = 0;
    if (passed)
        *passed = 0;
    if (failed)
        *failed = 0;
    for (i = 0; i < s->enrollCount; i++)
    {
        if (term && strcmp(s->enroll[i].term, term) != 0)
            continue;
        if (enrolled)
            (*enrolled)++;
        if (s->enroll[i].grade < 0)
            continue; /* not graded yet */
        {
            int cr = course_credits(s->enroll[i].courseCode);
            sum += s->enroll[i].grade * cr;
            units += cr;
            if (s->enroll[i].grade >= 10.0)
            {
                if (passed)
                    (*passed)++;
            }
            else
            {
                if (failed)
                    (*failed)++;
            }
        }
    }
    return units ? sum / units : 0.0;
}

static double total_gpa(const Student *s)
{
    return term_gpa(s, NULL, NULL, NULL, NULL);
}

static int find_offering(const char *term, const char *code)
{
    int i;
    for (i = 0; i < offering_count; i++)
        if (strcmp(offerings[i].term, term) == 0 &&
            strcmp(offerings[i].courseCode, code) == 0 &&
            offerings[i].is_Approved)
            return i;
    return -1;
}


int main(void)
{
    load_all();
    login_menu();
    return 0;
}

static void login_menu(void)
{
    while(1)
    {
        int option;
        char username[64], password[64];

        clear_screen();
        printf("1. Login as student\n");
        printf("2. Login as faculty\n");
        printf("3. Login as admin\n");
        printf("4. Forgot password\n");
        printf("5. Exit\n");
        printf("Enter an option: ");
        option = read_int();

        if (option == 5)
        {
            save_all();
            printf("Goodbye.\n");
            return;
        }
        if (option == 4)
        {
            forgot_password();
            continue;
        }
        if (option < 1 || option > 3)
            continue;

        clear_screen();
        printf("Enter your username: ");
        read_line(username, sizeof username);

        if (option == 1)
        {
            int idx = find_student(username);
            if (idx < 0)
            {
                printf("Username not found.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            printf("Enter password: ");
            read_password(password, sizeof password);
            if (strcmp(students[idx].password, password) != 0)
            {
                printf("Incorrect password.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            student_dashboard(idx);
        }
        else if (option == 2)
        {
            int idx = find_prof(username);
            if (idx < 0)
            {
                printf("Username not found.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            printf("Enter password: ");
            read_password(password, sizeof password);
            if (strcmp(professors[idx].password, password) != 0)
            {
                printf("Incorrect password.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            faculty_dashboard(idx);
        }
        else
        {
            if (strcmp(username, "admin") != 0)
            {
                printf("Username not found.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            printf("Enter password: ");
            read_password(password, sizeof password);
            if (strcmp(password, "admin") != 0)
            {
                printf("Incorrect password.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            admin_dashboard();
        }
    }
}

static bool ask_security_questions(const Student *s)
{
    char ans[128];
    printf("Where were you born? ");
    read_line(ans, sizeof ans);
    trim(ans);
    if (!ci_equal(ans, s->birthPlace))
    {
        printf("Incorrect answer.\n");
        return false;
    }

    printf("What was the title of the first book you read? ");
    read_line(ans, sizeof ans);
    trim(ans);
    if (!ci_equal(ans, s->firstSchool))
    {
        printf("Incorrect answer.\n");
        return false;
    }

    printf("What was the color of your first bicycle? ");
    read_line(ans, sizeof ans);
    trim(ans);
    if (!ci_equal(ans, s->firstBike))
    {
        printf("Incorrect answer.\n");
        return false;
    }

    return true;
}

static void forgot_password(void)
{
    for (;;)
    { 
        char username[64];
        int idx, opt;

        clear_screen();
        printf("Enter your username: ");
        read_line(username, sizeof username);
        idx = find_student(username);
        if (idx < 0)
        {
            printf("Username not found.\n");
            printf("1. Retry\n2. Go to login menu\nEnter an option: ");
            opt = read_int();
            if (opt == 1)
                continue;
            return;
        }

        if (!ask_security_questions(&students[idx]))
        {
            printf("1. Retry\n2. Go to login menu\nEnter an option: ");
            opt = read_int();
            if (opt == 1)
                continue;
            return;
        }

        printf("Authentication successful.\n");
        for (;;)
        { 
            char p1[64], p2[64];
            printf("Enter your new password: ");
            read_password(p1, sizeof p1);
            printf("Confirm your password: ");
            read_password(p2, sizeof p2);
            if (strcmp(p1, p2) != 0)
            {
                printf("Passwords aren't matching.\n");
                printf("1. Retry.\n2. Cancel (go to login menu).\nEnter an option: ");
                opt = read_int();
                if (opt == 1)
                    continue;
                return;
            }
            copy_str(students[idx].password, sizeof students[idx].password, p1);
            save_students();
            printf("Password changed successfully.\n");
            press_any_key("Pres any key to go to login menu...");
            return;
        }
    }
}

static int split_csv(char *line, char **fields, int max)
{
    int n = 0, i;
    char *p = line;
    if (max <= 0)
        return 0;
    fields[n++] = p;
    for (; *p; p++)
    {
        if (*p == ',')
        {
            *p = '\0';
            if (n < max)
                fields[n++] = p + 1;
        }
    }
    for (i = 0; i < n; i++)
        trim(fields[i]);
    return n;
}

static void admin_calendar(void);
static void admin_students(void);
static void admin_faculty(void);
static void admin_requests(void);
static void admin_offerings(void);
static void admin_courses(void);

static void admin_dashboard(void)
{
    for (;;)
    {
        int opt;
        clear_screen();
        printf("Welcome %s\n", "admin");
        printf("1. Calendar\n");
        printf("2. Students\n");
        printf("3. Faculty members\n");
        printf("4. Requests\n");
        printf("5. Offerings\n");
        printf("6. Courses\n");
        printf("7. Log out\n");
        printf("Enter an option: ");
        opt = read_int();
        switch (opt)
        {
        case 1:
            admin_calendar();
            break;
        case 2:
            admin_students();
            break;
        case 3:
            admin_faculty();
            break;
        case 4:
            admin_requests();
            break;
        case 5:
            admin_offerings();
            break;
        case 6:
            admin_courses();
            break;
        case 7:
            save_all();
            return;
        default:
            break;
        }
    }
}

static const char *state_text(int st)
{
    if (st == ST_DISABLED)
        return "disabled";
    if (st == ST_ENABLED)
        return "enabled";
    return "finished";
}

static void trigger_phase(int i)
{
    if (i == PH_OFFERING && phase[PH_OFFERING] == ST_FINISHED)
    {
        char nt[16];
        int k;
        for (k = 0; k < PH_COUNT; k++)
        {
            if (phase[k] != ST_FINISHED)
            {
                printf("\n\"%s\" of the current semester must be finished first.\n", PHASE_NAME[k]);
                press_any_key("Press any key to go back...");
                return;
            }
        }
        next_term(currentTerm, nt, sizeof nt);
        copy_str(currentTerm, sizeof currentTerm, nt);
        for (k = 0; k < PH_COUNT; k++)
            phase[k] = ST_DISABLED;
        phase[PH_OFFERING] = ST_ENABLED;
        save_calendar();
        printf("\nSemester %s started. \"offering\" is now enabled.\n", currentTerm);
        press_any_key("Press any key to go back...");
        return;
    }

    if (phase[i] == ST_DISABLED)
    {
        if (i > 0 && phase[i - 1] == ST_DISABLED)
        {
            printf("\n\"%s\" must be started before \"%s\".\n", PHASE_NAME[i - 1], PHASE_NAME[i]);
            press_any_key("Press any key to go back...");
            return;
        }
        phase[i] = ST_ENABLED;
        printf("\n\"%s\" is now enabled.\n", PHASE_NAME[i]);
    }
    else if (phase[i] == ST_ENABLED)
    {
        if (i > 0 && phase[i - 1] != ST_FINISHED)
        {
            printf("\n\"%s\" must be finished before \"%s\".\n", PHASE_NAME[i - 1], PHASE_NAME[i]);
            press_any_key("Press any key to go back...");
            return;
        }
        phase[i] = ST_FINISHED;
        printf("\n\"%s\" is now finished.\n", PHASE_NAME[i]);
        if (i == PH_GRADE)
            printf("Course surveys are now open for semester %s.\n", currentTerm);
    }
    else
    {
        printf("\n\"%s\" has already been finished in this semester.\n", PHASE_NAME[i]);
    }
    save_calendar();
    press_any_key("Press any key to go back...");
}

static void admin_calendar(void)
{
    for (;;)
    {
        int opt, i;
        clear_screen();
        printf("Admin: Calendar (semester %s)\n", currentTerm);
        for (i = 0; i < PH_COUNT; i++)
            printf("%d. %-16s %s\n", i + 1, PHASE_NAME[i], state_text(phase[i]));
        printf("5. go to main menu\n");
        printf("Enter a time to trigger: ");
        opt = read_int();
        if (opt == 5)
            return;
        if (opt < 1 || opt > 4)
            continue;
        trigger_phase(opt - 1);
    }
}

static void register_one_student(void)
{
    Student *s;
    char buf[128];
    if (student_count >= 300)
    {
        printf("The system is at maximum student capacity.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    s = &students[student_count];
    memset(s, 0, sizeof *s);

    printf("Enter student id: ");
    read_line(s->username, sizeof s->username);
    trim(s->username);
    if (s->username[0] == '\0')
    {
        printf("Student id cannot be empty.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (find_student(s->username) >= 0)
    {
        printf("A student with this id already exists.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("Enter password: ");
    read_line(s->password, sizeof s->password);
    printf("Enter first name: ");
    read_line(s->firstName, sizeof s->firstName);
    printf("Enter last name: ");
    read_line(s->lastName, sizeof s->lastName);
    printf("Enter national code: ");
    read_line(s->nationalId, sizeof s->nationalId);
    printf("Enter field: ");
    read_line(s->major, sizeof s->major);
    printf("Enter entrance year: ");
    s->entryYear = read_int();
    for (;;)
    {
        printf("Enter section (BSc/MSc/PhD): ");
        read_line(s->degree, sizeof s->degree);
        trim(s->degree);
        if (strcmp(s->degree, "BSc") == 0 || strcmp(s->degree, "MSc") == 0 ||
            strcmp(s->degree, "PhD") == 0)
            break;
        printf("Invalid section. It must be one of BSc, MSc or PhD.\n");
    }
    printf("Enter mentor: ");
    read_line(s->supervisor, sizeof s->supervisor);
    printf("Enter department: ");
    read_line(s->faculty, sizeof s->faculty);
    printf("Where were you born? ");
    read_line(s->birthPlace, sizeof s->birthPlace);
    printf("What was the title of the first book you read? ");
    read_line(s->firstSchool, sizeof s->firstSchool);
    printf("What was the color of your first bicycle? ");
    read_line(s->firstBike, sizeof s->firstBike);
    if (strcmp(s->degree, "PhD") == 0)
    {
        printf("Enter thesis title: ");
        read_line(s->thesisTitle, sizeof s->thesisTitle);
        printf("Enter thesis abstract: ");
        read_line(s->thesisAbstract, sizeof s->thesisAbstract);
        printf("Enter number of citations: ");
        s->thesisCitations = read_int();
        if (s->thesisCitations < 0)
            s->thesisCitations = 0;
    }
    (void)buf;
    student_count++;
    save_students();
    printf("\nStudent registered successfully.\n");
    press_any_key("Press any key to go back...");
}

static void import_students(void)
{
    char path[256], line[1024];
    FILE *f;
    int added = 0, skipped = 0, lineNo = 0;

    printf("Enter the file path (csv): ");
    read_line(path, sizeof path);
    f = fopen(path, "r");
    if (!f)
    {
        printf("File not found.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("Expected columns: first name,last name,student id,national code,field,"
           "entrance year,section,mentor,department,answer 1,answer 2,answer 3,password\n");
    while (fgets(line, sizeof line, f))
    {
        char *fields[16];
        int n;
        Student *s;
        line[strcspn(line, "\r\n")] = '\0';
        lineNo++;
        if (line[0] == '\0')
            continue;
        if (lineNo == 1 && ci_contains(line, "first name"))
            continue; /* header */
        n = split_csv(line, fields, 16);
        if (n < 12)
        {
            skipped++;
            continue;
        }
        if (student_count >= 300)
            break;
        if (find_student(fields[2]) >= 0)
        {
            skipped++;
            continue;
        }
        s = &students[student_count];
        memset(s, 0, sizeof *s);
        copy_str(s->firstName, sizeof s->firstName, fields[0]);
        copy_str(s->lastName, sizeof s->lastName, fields[1]);
        copy_str(s->username, sizeof s->username, fields[2]);
        copy_str(s->nationalId, sizeof s->nationalId, fields[3]);
        copy_str(s->major, sizeof s->major, fields[4]);
        s->entryYear = atoi(fields[5]);
        copy_str(s->degree, sizeof s->degree, fields[6]);
        copy_str(s->supervisor, sizeof s->supervisor, fields[7]);
        copy_str(s->faculty, sizeof s->faculty, fields[8]);
        copy_str(s->birthPlace, sizeof s->birthPlace, fields[9]);
        copy_str(s->firstSchool, sizeof s->firstSchool, fields[10]);
        copy_str(s->firstBike, sizeof s->firstBike, fields[11]);
        copy_str(s->password, sizeof s->password, n > 12 ? fields[12] : "password123");
        student_count++;
        added++;
    }
    fclose(f);
    save_students();
    printf("\n%d student(s) imported, %d row(s) skipped.\n", added, skipped);
    press_any_key("Press any key to go back...");
}

static void remove_student_from_offerings(const Student *s)
{
    int i, k;
    for (i = 0; i < s->enrollCount; i++)
    {
        k = find_offering(s->enroll[i].term, s->enroll[i].courseCode);
        if (k >= 0 && offerings[k].enrolledCount > 0)
            offerings[k].enrolledCount--;
    }
}

static void remove_student(void)
{
    char id[64], yn[16];
    int idx, i;
    printf("Enter student id: ");
    read_line(id, sizeof id);
    trim(id);
    idx = find_student(id);
    if (idx < 0)
    {
        printf("Student id %s not found.\n", id);
        press_any_key("Press any key to go back...");
        return;
    }
    print_student_header();
    print_student_row(&students[idx]);
    printf("Remove student? [y/n] ");
    read_line(yn, sizeof yn);
    if (yn[0] != 'y' && yn[0] != 'Y')
    {
        printf("Cancelled.\n");
        press_any_key("Press any key to go back...");
        return;
    }

    remove_student_from_offerings(&students[idx]);
    for (i = idx; i < student_count - 1; i++)
        students[i] = students[i + 1];
    student_count--;
    save_students();
    save_offerings();
    printf("Student %s removed successfully.\n", id);
    press_any_key("Press any key to go back...");
}

static void admin_students(void)
{
    for (;;)
    {
        int opt;
        clear_screen();
        printf("Admin: Students\n");
        printf("1. students list\n");
        printf("2. register student(s)\n");
        printf("3. remove student(s)\n");
        printf("4. usernames & passwords\n");
        printf("5. go back\n");
        printf("Enter an option: ");
        opt = read_int();

        if (opt == 4)
        {
            int i;
            clear_screen();
            printf("Admin: Students: usernames & passwords\n");
            printf("| %-12s | %-12s | %-12s | %-16s |\n",
                   "student id", "username", "password", "name");
            printf("|--------------|--------------|--------------|------------------|\n");
            for (i = 0; i < student_count; i++)
                printf("| %-12s | %-12s | %-12s | %-16s |\n",
                       students[i].username, students[i].username,
                       students[i].password, students[i].lastName);
            if (student_count == 0)
                printf("(no student registered)\n");
            press_any_key("\nPress any key to go back...");
            continue;
        }

        if (opt == 1)
        {
            int i, sub;
            clear_screen();
            printf("Admin: Students: students list\n");
            printf("Students list\n");
            print_student_header();
            for (i = 0; i < student_count; i++)
                print_student_row(&students[i]);
            if (student_count == 0)
                printf("(no student registered)\n");
            printf("\n1. search\n2. go back\nEnter an option: ");
            sub = read_int();
            if (sub == 1)
                search_students();
        }
        else if (opt == 2)
        {
            int sub;
            clear_screen();
            printf("Admin: Students: register student(s)\n");
            printf("Register student(s)\n");
            printf("1. Register one student\n");
            printf("2. Register a group of students (import a file)\n");
            printf("3. Go back\n");
            printf("Enter an option: ");
            sub = read_int();
            if (sub == 1)
                register_one_student();
            else if (sub == 2)
                import_students();
        }
        else if (opt == 3)
        {
            clear_screen();
            printf("Admin: Students: remove student(s)\n");
            printf("Remove student(s)\n");
            remove_student();
        }
        else if (opt == 5)
        {
            return;
        }
    }
}

static void register_one_prof(void)
{
    Professor *p;
    if (prof_count >= 300)
    {
        printf("The system is at maximum faculty capacity.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    p = &professors[prof_count];
    memset(p, 0, sizeof *p);
    printf("Enter faculty id: ");
    read_line(p->username, sizeof p->username);
    trim(p->username);
    if (p->username[0] == '\0')
    {
        printf("Faculty id cannot be empty.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (find_prof(p->username) >= 0)
    {
        printf("A faculty member with this id already exists.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("Enter password: ");
    read_line(p->password, sizeof p->password);
    printf("Enter first name: ");
    read_line(p->firstName, sizeof p->firstName);
    printf("Enter last name: ");
    read_line(p->lastName, sizeof p->lastName);
    printf("Enter national code: ");
    read_line(p->nationalId, sizeof p->nationalId);
    printf("Enter field: ");
    read_line(p->major, sizeof p->major);
    printf("Enter entrance year: ");
    p->entryYear = read_int();
    printf("Enter last degree: ");
    read_line(p->lastDegree, sizeof p->lastDegree);
    printf("Enter department: ");
    read_line(p->faculty, sizeof p->faculty);
    prof_count++;
    save_professors();
    printf("\nFaculty member registered successfully.\n");
    press_any_key("Press any key to go back...");
}

static void import_professors(void)
{
    char path[256], line[1024];
    FILE *f;
    int added = 0, skipped = 0, lineNo = 0;

    printf("Enter the file path (csv): ");
    read_line(path, sizeof path);
    f = fopen(path, "r");
    if (!f)
    {
        printf("File not found.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("Expected columns: first name,last name,faculty id,national code,field,"
           "entrance year,last degree,department,password\n");
    while (fgets(line, sizeof line, f))
    {
        char *fields[12];
        int n;
        Professor *p;
        line[strcspn(line, "\r\n")] = '\0';
        lineNo++;
        if (line[0] == '\0')
            continue;
        if (lineNo == 1 && ci_contains(line, "first name"))
            continue;
        n = split_csv(line, fields, 12);
        if (n < 8)
        {
            skipped++;
            continue;
        }
        if (prof_count >= 300)
            break;
        if (find_prof(fields[2]) >= 0)
        {
            skipped++;
            continue;
        }
        p = &professors[prof_count];
        memset(p, 0, sizeof *p);
        copy_str(p->firstName, sizeof p->firstName, fields[0]);
        copy_str(p->lastName, sizeof p->lastName, fields[1]);
        copy_str(p->username, sizeof p->username, fields[2]);
        copy_str(p->nationalId, sizeof p->nationalId, fields[3]);
        copy_str(p->major, sizeof p->major, fields[4]);
        p->entryYear = atoi(fields[5]);
        copy_str(p->lastDegree, sizeof p->lastDegree, fields[6]);
        copy_str(p->faculty, sizeof p->faculty, fields[7]);
        copy_str(p->password, sizeof p->password, n > 8 ? fields[8] : "password123");
        prof_count++;
        added++;
    }
    fclose(f);
    save_professors();
    printf("\n%d faculty member(s) imported, %d row(s) skipped.\n", added, skipped);
    press_any_key("Press any key to go back...");
}

static void remove_prof(void)
{
    char id[64], yn[16];
    int idx, i;
    printf("Enter faculty id: ");
    read_line(id, sizeof id);
    trim(id);
    idx = find_prof(id);
    if (idx < 0)
    {
        printf("Faculty id %s not found.\n", id);
        press_any_key("Press any key to go back...");
        return;
    }
    for (i = 0; i < offering_count; i++)
    {
        if (strcmp(offerings[i].professorId, id) == 0 &&
            strcmp(offerings[i].term, currentTerm) == 0 && offerings[i].is_Approved)
        {
            printf("This faculty member has an active offering in the current semester "
                   "and cannot be removed.\n");
            press_any_key("Press any key to go back...");
            return;
        }
    }
    print_prof_header();
    print_prof_row(&professors[idx]);
    printf("Remove faculty member? [y/n] ");
    read_line(yn, sizeof yn);
    if (yn[0] != 'y' && yn[0] != 'Y')
    {
        printf("Cancelled.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    for (i = idx; i < prof_count - 1; i++)
        professors[i] = professors[i + 1];
    prof_count--;
    save_professors();
    printf("Faculty member %s removed successfully.\n", id);
    press_any_key("Press any key to go back...");
}

static void admin_faculty(void)
{
    for (;;)
    {
        int opt;
        clear_screen();
        printf("Admin: Faculty members\n");
        printf("1. faculty list\n");
        printf("2. register faculty member(s)\n");
        printf("3. remove faculty member(s)\n");
        printf("4. go back\n");
        printf("Enter an option: ");
        opt = read_int();

        if (opt == 1)
        {
            int i, sub;
            clear_screen();
            printf("Admin: Faculty members: faculty list\n");
            printf("Faculty list\n");
            print_prof_header();
            for (i = 0; i < prof_count; i++)
                print_prof_row(&professors[i]);
            if (prof_count == 0)
                printf("(no faculty member registered)\n");
            printf("\n1. search\n2. go back\nEnter an option: ");
            sub = read_int();
            if (sub == 1)
                search_professors();
        }
        else if (opt == 2)
        {
            int sub;
            clear_screen();
            printf("Admin: Faculty members: register faculty member(s)\n");
            printf("1. Register one faculty member\n");
            printf("2. Register a group of faculty members (import a file)\n");
            printf("3. Go back\n");
            printf("Enter an option: ");
            sub = read_int();
            if (sub == 1)
                register_one_prof();
            else if (sub == 2)
                import_professors();
        }
        else if (opt == 3)
        {
            clear_screen();
            printf("Admin: Faculty members: remove faculty member(s)\n");
            remove_prof();
        }
        else if (opt == 4)
        {
            return;
        }
    }
}

static void print_request(int number, const Request *r)
{
    char fac[96];
    int off;
    prof_full_name(r->professorId, fac, sizeof fac);
    switch (r->type)
    {
    case REQ_OFFER:
        printf("%d. course offering\n", number);
        printf("   Course: %s (%s)\n", course_name(r->courseCode), r->courseCode);
        printf("   Faculty: %s\n", fac);
        printf("   Department: %s\n", find_prof(r->professorId) >= 0 ? professors[find_prof(r->professorId)].faculty : "-");
        printf("   Semester: %s\n", r->term);
        printf("   Capacity: %d\n", r->capacity);
        printf("   Place: %s\n", r->location);
        break;
    case REQ_REMOVE:
        off = find_offering(r->term, r->courseCode);
        printf("%d. course removing\n", number);
        printf("   Course: %s (%s)\n", course_name(r->courseCode), r->courseCode);
        printf("   Faculty: %s\n", fac);
        printf("   Department: %s\n", off >= 0 ? offerings[off].faculty : "-");
        printf("   Semester: %s\n", r->term);
        printf("   Capacity: %d\n", off >= 0 ? offerings[off].capacity : 0);
        printf("   No. enrollments: %d\n", off >= 0 ? offerings[off].enrolledCount : 0);
        break;
    case REQ_CAPACITY:
        off = find_offering(r->term, r->courseCode);
        printf("%d. capacity increment\n", number);
        printf("   Course: %s (%s)\n", course_name(r->courseCode), r->courseCode);
        printf("   Faculty: %s\n", fac);
        printf("   Department: %s\n", off >= 0 ? offerings[off].faculty : "-");
        printf("   Semester: %s\n", r->term);
        printf("   Capacity: %d\n", off >= 0 ? offerings[off].capacity : 0);
        printf("   No. enrollments: %d\n", off >= 0 ? offerings[off].enrolledCount : 0);
        printf("   Requested extra capacity: %d\n", r->extra);
        break;
    default:
        break;
    }
}

static void accept_request(int idx)
{
    Request *r = &requests[idx];
    int off;
    if (r->type == REQ_OFFER)
    {
        if (offering_count >= 300)
        {
            printf("Offering list is full.\n");
            return;
        }
        {
            Offering *o = &offerings[offering_count];
            int pi = find_prof(r->professorId);
            memset(o, 0, sizeof *o);
            copy_str(o->courseCode, sizeof o->courseCode, r->courseCode);
            copy_str(o->professorId, sizeof o->professorId, r->professorId);
            copy_str(o->term, sizeof o->term, r->term);
            o->capacity = r->capacity;
            o->enrolledCount = 0;
            copy_str(o->faculty, sizeof o->faculty, pi >= 0 ? professors[pi].faculty : "-");
            copy_str(o->location, sizeof o->location, r->location[0] ? r->location : "TBA");
            o->is_Approved = true;
            offering_count++;
        }
        printf("The offering has been approved.\n");
    }
    else if (r->type == REQ_REMOVE)
    {
        off = find_offering(r->term, r->courseCode);
        if (off < 0)
        {
            printf("The offering no longer exists.\n");
        }
        else
        {
            int i, j, k;
            for (i = 0; i < student_count; i++)
            {
                k = find_enrollment(&students[i], r->term, r->courseCode);
                if (k >= 0)
                {
                    for (j = k; j < students[i].enrollCount - 1; j++)
                        students[i].enroll[j] = students[i].enroll[j + 1];
                    students[i].enrollCount--;
                }
            }
            for (i = off; i < offering_count - 1; i++)
                offerings[i] = offerings[i + 1];
            offering_count--;
            printf("The offering has been removed.\n");
        }
    }
    else if (r->type == REQ_CAPACITY)
    {
        off = find_offering(r->term, r->courseCode);
        if (off < 0)
            printf("The offering no longer exists.\n");
        else
        {
            offerings[off].capacity += r->extra;
            printf("Capacity increased to %d.\n", offerings[off].capacity);
        }
    }
    r->status = REQ_ACCEPTED;
    save_offerings();
    save_students();
    save_requests();
}

static void admin_requests(void)
{
    for (;;)
    {
        int i, opt, n = 0;
        int map[300];

        clear_screen();
        printf("Admin: Requests\n");
        printf("List of requests\n");
        for (i = 0; i < request_count; i++)
        {
            if (requests[i].status != REQ_PENDING)
                continue;
            map[n] = i;
            print_request(n + 1, &requests[i]);
            n++;
        }
        if (n == 0)
            printf("(there is no pending request)\n");
        printf("\n1. Go to request number\n2. Go back\nEnter an option: ");
        opt = read_int();
        if (opt != 1)
            return;
        printf("Enter request number: ");
        opt = read_int();
        if (opt < 1 || opt > n)
        {
            printf("Invalid request number.\n");
            press_any_key("Press any key to go back...");
            continue;
        }
        {
            int idx = map[opt - 1], sub;
            printf("\n");
            print_request(opt, &requests[idx]);
            printf("\n1. Accept\n2. Reject\n3. Go back\nEnter an option: ");
            sub = read_int();
            if (sub == 1)
                accept_request(idx);
            else if (sub == 2)
            {
                requests[idx].status = REQ_REJECTED;
                save_requests();
                printf("The request has been rejected.\n");
            }
            else
                continue;
            press_any_key("Press any key to go back...");
        }
    }
}

static void admin_add_student_to_offering(void)
{
    int num, oi, si;
    char id[64];
    printf("Enter offering number: ");
    num = read_int();
    if (num < 1 || num > listed_count)
    {
        printf("Invalid offering number.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    oi = listed[num - 1];
    printf("Enter student id: ");
    read_line(id, sizeof id);
    trim(id);
    si = find_student(id);
    if (si < 0)
    {
        printf("Student id %s not found.\n", id);
        press_any_key("Press any key to go back...");
        return;
    }
    if (find_enrollment(&students[si], offerings[oi].term, offerings[oi].courseCode) >= 0)
    {
        printf("The student is already enrolled in this offering.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (offerings[oi].enrolledCount >= offerings[oi].capacity)
    {
        printf("This offering is full.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (students[si].enrollCount >= 300)
    {
        printf("The student has reached the maximum number of enrollments.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    {
        Enrollment *e = &students[si].enroll[students[si].enrollCount++];
        copy_str(e->term, sizeof e->term, offerings[oi].term);
        copy_str(e->courseCode, sizeof e->courseCode, offerings[oi].courseCode);
        e->grade = -1.0;
    }
    offerings[oi].enrolledCount++;
    save_students();
    save_offerings();
    printf("The student was added to the offering.\n");
    press_any_key("Press any key to go back...");
}

static void admin_remove_student_from_offering(void)
{
    int num, oi, si, k, j;
    char id[64];
    printf("Enter offering number: ");
    num = read_int();
    if (num < 1 || num > listed_count)
    {
        printf("Invalid offering number.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    oi = listed[num - 1];
    printf("Enter student id: ");
    read_line(id, sizeof id);
    trim(id);
    si = find_student(id);
    if (si < 0)
    {
        printf("Student id %s not found.\n", id);
        press_any_key("Press any key to go back...");
        return;
    }
    k = find_enrollment(&students[si], offerings[oi].term, offerings[oi].courseCode);
    if (k < 0)
    {
        printf("The student is not enrolled in this offering.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    for (j = k; j < students[si].enrollCount - 1; j++)
        students[si].enroll[j] = students[si].enroll[j + 1];
    students[si].enrollCount--;
    if (offerings[oi].enrolledCount > 0)
        offerings[oi].enrolledCount--;
    save_students();
    save_offerings();
    printf("The student was removed from the offering.\n");
    press_any_key("Press any key to go back...");
}

static void admin_add_capacity(void)
{
    int num, oi, extra;
    printf("Enter offering number: ");
    num = read_int();
    if (num < 1 || num > listed_count)
    {
        printf("Invalid offering number.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    oi = listed[num - 1];
    printf("Current capacity of %s is %d.\n", course_name(offerings[oi].courseCode), offerings[oi].capacity);
    printf("Enter the extra capacity: ");
    extra = read_int();
    if (extra <= 0)
    {
        printf("The extra capacity must be a positive number.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    offerings[oi].capacity += extra;
    save_offerings();
    printf("The capacity was increased to %d.\n", offerings[oi].capacity);
    press_any_key("Press any key to go back...");
}

static void admin_offerings(void)
{
    char term[16];
    clear_screen();
    printf("Admin: Offerings\n");
    printf("Enter semester number: ");
    read_line(term, sizeof term);
    trim(term);
    if (term[0] == '\0')
        copy_str(term, sizeof term, currentTerm);

    for (;;)
    {
        int opt;
        clear_screen();
        printf("Admin: Offerings\n");
        list_offerings_of_term(term, 0);
        printf("\n1. Search\n2. Add student to an offering\n3. Remove student from an offering\n");
        printf("4. Add capacity to an offering\n5. Go back\n");
        printf("Enter an option: ");
        opt = read_int();
        if (opt == 4)
        {
            admin_add_capacity();
            continue;
        }
        if (opt == 1)
            search_offerings(term, 0);
        else if (opt == 2)
            admin_add_student_to_offering();
        else if (opt == 3)
            admin_remove_student_from_offering();
        else
            return;
    }
}

static void admin_add_course(void)
{
    Course *c;
    if (course_count >= 300)
    {
        printf("The course list is full.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    c = &courses[course_count];
    memset(c, 0, sizeof *c);
    printf("Enter course id: ");
    read_line(c->code, sizeof c->code);
    trim(c->code);
    if (c->code[0] == '\0')
    {
        printf("Course id cannot be empty.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (find_course(c->code) >= 0)
    {
        printf("A course with this id already exists.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("Enter course name: ");
    read_line(c->name, sizeof c->name);
    printf("Enter units: ");
    c->credits = read_int();
    if (c->credits < 0)
        c->credits = 0;
    printf("Enter prerequisites (separated by comma, \"-\" if none): ");
    read_line(c->prerequisites, sizeof c->prerequisites);
    trim(c->prerequisites);
    if (c->prerequisites[0] == '\0')
        copy_str(c->prerequisites, sizeof c->prerequisites, "-");
    for (;;)
    {
        printf("Enter section (BSc/MSc/PhD): ");
        read_line(c->degree, sizeof c->degree);
        trim(c->degree);
        if (strcmp(c->degree, "BSc") == 0 || strcmp(c->degree, "MSc") == 0 ||
            strcmp(c->degree, "PhD") == 0)
            break;
        printf("Invalid section. It must be one of BSc, MSc or PhD.\n");
    }
    printf("Enter field: ");
    read_line(c->major, sizeof c->major);
    printf("Enter department: ");
    read_line(c->faculty, sizeof c->faculty);
    course_count++;
    save_courses();
    printf("\nCourse registered successfully.\n");
    if (phase[PH_OFFERING] != ST_DISABLED)
        printf("Note: the offering stage of semester %s has already started, so this course "
               "cannot be offered in the current semester.\n",
               currentTerm);
    press_any_key("Press any key to go back...");
}

static void admin_remove_course(void)
{
    char code[32], yn[16];
    int idx, i;
    printf("Enter course id: ");
    read_line(code, sizeof code);
    trim(code);
    idx = find_course(code);
    if (idx < 0)
    {
        printf("Course id %s not found.\n", code);
        press_any_key("Press any key to go back...");
        return;
    }
    for (i = 0; i < offering_count; i++)
        if (strcmp(offerings[i].courseCode, code) == 0 && strcmp(offerings[i].term, currentTerm) == 0)
        {
            printf("This course is offered in the current semester and cannot be removed.\n");
            press_any_key("Press any key to go back...");
            return;
        }
    print_course_header();
    print_course_row(&courses[idx]);
    printf("Remove course? [y/n] ");
    read_line(yn, sizeof yn);
    if (yn[0] != 'y' && yn[0] != 'Y')
    {
        printf("Cancelled.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    for (i = idx; i < course_count - 1; i++)
        courses[i] = courses[i + 1];
    course_count--;
    save_courses();
    printf("Course %s removed successfully.\n", code);
    press_any_key("Press any key to go back...");
}

static void admin_courses(void)
{
    for (;;)
    {
        int opt;
        clear_screen();
        printf("Admin: Courses\n");
        list_all_courses();
        printf("\n1. Search\n2. Add a course\n3. Remove a course\n4. Go back\n");
        printf("Enter an option: ");
        opt = read_int();
        if (opt == 1)
            search_courses();
        else if (opt == 2)
            admin_add_course();
        else if (opt == 3)
            admin_remove_course();
        else
            return;
    }
}

static void faculty_my_offerings(int p);
static void faculty_offer_course(int p);
static void faculty_survey_results(int p);

static void faculty_dashboard(int p)
{
    for (;;)
    {
        int opt;
        clear_screen();
        printf("Welcome %s %s\n", professors[p].firstName, professors[p].lastName);
        printf("1. My offerings\n");
        printf("2. List of offerings in semester\n");
        printf("3. List of courses\n");
        printf("4. Offer a course\n");
        printf("5. Survey results\n");
        printf("6. Log out\n");
        printf("Enter an option: ");
        opt = read_int();

        if (opt == 1)
            faculty_my_offerings(p);
        else if (opt == 2)
        {
            char term[16];
            int sub;
            clear_screen();
            printf("Enter semester number: ");
            read_line(term, sizeof term);
            trim(term);
            if (term[0] == '\0')
                copy_str(term, sizeof term, currentTerm);
            clear_screen();
            list_offerings_of_term(term, 0);
            printf("\n1. Search\n2. Go back\nEnter an option: ");
            sub = read_int();
            if (sub == 1)
                search_offerings(term, 0);
        }
        else if (opt == 3)
        {
            int sub;
            clear_screen();
            list_all_courses();
            printf("\n1. Search\n2. Go back\nEnter an option: ");
            sub = read_int();
            if (sub == 1)
                search_courses();
        }
        else if (opt == 4)
            faculty_offer_course(p);
        else if (opt == 5)
            faculty_survey_results(p);
        else if (opt == 6)
        {
            save_all();
            return;
        }
    }
}

static void faculty_offer_course(int p)
{
    char code[32], place[96];
    int ci, cap, i;

    clear_screen();
    printf("Faculty: Offer a course\n");
    if (phase[PH_OFFERING] != ST_ENABLED)
    {
        printf("The offering stage is not active at the moment.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("Enter the course id: ");
    read_line(code, sizeof code);
    trim(code);
    ci = find_course(code);
    if (ci < 0)
    {
        printf("Course id %s not found.\n", code);
        press_any_key("Press any key to go back...");
        return;
    }
    print_course_row(&courses[ci]);

    if (find_offering(currentTerm, code) >= 0)
    {
        printf("This course has already been offered in semester %s.\n", currentTerm);
        press_any_key("Press any key to go back...");
        return;
    }
    for (i = 0; i < request_count; i++)
        if (requests[i].status == REQ_PENDING && requests[i].type == REQ_OFFER &&
            strcmp(requests[i].courseCode, code) == 0 &&
            strcmp(requests[i].term, currentTerm) == 0)
        {
            printf("There is already a pending offering request for this course.\n");
            press_any_key("Press any key to go back...");
            return;
        }

    printf("Enter the capacity: ");
    cap = read_int();
    if (cap <= 0)
    {
        printf("The capacity must be a positive number.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("Enter the place: ");
    read_line(place, sizeof place);
    trim(place);
    if (place[0] == '\0')
        copy_str(place, sizeof place, "TBA");

    if (request_count >= 300)
    {
        printf("The request list is full.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    {
        Request *r = &requests[request_count++];
        memset(r, 0, sizeof *r);
        r->type = REQ_OFFER;
        copy_str(r->courseCode, sizeof r->courseCode, code);
        copy_str(r->professorId, sizeof r->professorId, professors[p].username);
        copy_str(r->term, sizeof r->term, currentTerm);
        r->capacity = cap;
        copy_str(r->location, sizeof r->location, place);
        r->status = REQ_PENDING;
    }
    save_requests();
    printf("Sent request to admin.\n");
    press_any_key("Press any key to go to offerings...");
}

static void record_grades_manual(int oi)
{
    int i, k, any = 0;
    for (i = 0; i < student_count; i++)
    {
        k = find_enrollment(&students[i], offerings[oi].term, offerings[oi].courseCode);
        if (k < 0)
            continue;
        any = 1;
        printf("%s %s (%s) - current grade: ",
               students[i].firstName, students[i].lastName, students[i].username);
        if (students[i].enroll[k].grade < 0)
            printf("not recorded");
        else
            printf("%.2f", students[i].enroll[k].grade);
        printf("\nEnter grade (0-20, empty to skip): ");
        {
            char buf[64], *end;
            double g;
            read_line(buf, sizeof buf);
            trim(buf);
            if (buf[0] == '\0')
                continue;
            g = strtod(buf, &end);
            if (end == buf || g < 0 || g > 20)
            {
                printf("Invalid grade, skipped.\n");
                continue;
            }
            students[i].enroll[k].grade = g;
        }
    }
    if (!any)
        printf("There is no student enrolled in this offering.\n");
    save_students();
    printf("\nGrades saved.\n");
    press_any_key("Press any key to go back...");
}

static void record_grades_csv(int oi)
{
    char path[256], line[512];
    FILE *f;
    int updated = 0, skipped = 0, lineNo = 0;

    printf("Enter the file path (csv: student id,grade): ");
    read_line(path, sizeof path);
    f = fopen(path, "r");
    if (!f)
    {
        printf("File not found.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    while (fgets(line, sizeof line, f))
    {
        char *fields[4];
        int n, si, k;
        double g;
        char *end;
        line[strcspn(line, "\r\n")] = '\0';
        lineNo++;
        if (line[0] == '\0')
            continue;
        if (lineNo == 1 && ci_contains(line, "student"))
            continue;
        n = split_csv(line, fields, 4);
        if (n < 2)
        {
            skipped++;
            continue;
        }
        si = find_student(fields[0]);
        if (si < 0)
        {
            skipped++;
            continue;
        }
        k = find_enrollment(&students[si], offerings[oi].term, offerings[oi].courseCode);
        if (k < 0)
        {
            skipped++;
            continue;
        }
        g = strtod(fields[1], &end);
        if (end == fields[1] || g < 0 || g > 20)
        {
            skipped++;
            continue;
        }
        students[si].enroll[k].grade = g;
        updated++;
    }
    fclose(f);
    save_students();
    printf("\n%d grade(s) recorded, %d row(s) skipped.\n", updated, skipped);
    press_any_key("Press any key to go back...");
}

static void faculty_record_grades(int oi)
{
    int opt;
    if (phase[PH_GRADE] != ST_ENABLED || strcmp(offerings[oi].term, currentTerm) != 0)
    {
        printf("The grade recording stage is not active at the moment.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("1. Enter grades one by one\n2. Import a csv file\n3. Go back\nEnter an option: ");
    opt = read_int();
    if (opt == 1)
        record_grades_manual(oi);
    else if (opt == 2)
        record_grades_csv(oi);
}

static void faculty_publish(int p, int oi, int kind)
{
    Assignment *a;
    int nq, i;

    if (phase[PH_CLASS] != ST_ENABLED || strcmp(offerings[oi].term, currentTerm) != 0)
    {
        printf("The \"class & exams\" stage is not active at the moment.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (assign_count >= 300)
    {
        printf("The assignment list is full.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    a = &assigns[assign_count];
    memset(a, 0, sizeof *a);
    a->id = assign_count + 1;
    a->kind = kind;
    copy_str(a->term, sizeof a->term, offerings[oi].term);
    copy_str(a->courseCode, sizeof a->courseCode, offerings[oi].courseCode);
    copy_str(a->professorId, sizeof a->professorId, professors[p].username);

    printf("Enter the title: ");
    read_line(a->title, sizeof a->title);
    printf("Enter the score: ");
    a->score = read_double();
    if (a->score < 0)
        a->score = 0;

    if (kind == 0)
    {
        printf("Enter the number of questions (at least 1, at most %d): ", 10);
        nq = read_int();
        if (nq < 1)
            nq = 1;
    }
    else
    {
        printf("Enter the number of questions (at most %d): ", 10);
        nq = read_int();
        if (nq < 1)
            nq = 1;
    }
    if (nq > 10)
        nq = 10;

    for (i = 0; i < nq; i++)
    {
        Question *q = &a->q[i];
        memset(q, 0, sizeof *q);
        printf("\nQuestion %d text: ", i + 1);
        read_line(q->text, sizeof q->text);
        if (kind == 0)
            q->type = 0; 
        else
        {
            printf("Type (1. multiple choice  2. descriptive): ");
            q->type = (read_int() == 2) ? 1 : 0;
        }
        if (q->type == 0)
        {
            int k;
            for (k = 0; k < 4; k++)
            {
                printf("Option %d: ", k + 1);
                read_line(q->options[k], sizeof q->options[k]);
            }
            printf("Correct option (1-4): ");
            q->correct = read_int() - 1;
            if (q->correct < 0 || q->correct > 3)
                q->correct = 0;
        }
        a->qcount++;
    }
    assign_count++;
    save_lms();
    printf("\n%s published successfully.\n", kind == 0 ? "Homework" : "Exam");
    press_any_key("Press any key to go back...");
}


static void faculty_offering_menu(int p, int oi)
{
    for (;;)
    {
        int opt;
        clear_screen();
        printf("Faculty: My offerings: offering\n");
        print_offering_header(0);
        print_offering_row(1, &offerings[oi], 0);
        printf("\n1. Add capacity\n2. Record grades\n3. Remove offering\n");
        printf("4. Publish a homework\n5. Publish an exam\n6. Go back\n");
        printf("Enter an option: ");
        opt = read_int();

        if (opt == 1)
        {
            int extra;
            printf("Enter the extra capacity: ");
            extra = read_int();
            if (extra <= 0)
            {
                printf("The extra capacity must be a positive number.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            if (request_count >= 300)
            {
                printf("The request list is full.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            {
                Request *r = &requests[request_count++];
                memset(r, 0, sizeof *r);
                r->type = REQ_CAPACITY;
                copy_str(r->courseCode, sizeof r->courseCode, offerings[oi].courseCode);
                copy_str(r->professorId, sizeof r->professorId, professors[p].username);
                copy_str(r->term, sizeof r->term, offerings[oi].term);
                r->extra = extra;
                r->status = REQ_PENDING;
            }
            save_requests();
            printf("Sent request to admin.\n");
            press_any_key("Press any key to go back...");
        }
        else if (opt == 2)
        {
            faculty_record_grades(oi);
        }
        else if (opt == 3)
        {
            if (strcmp(offerings[oi].term, currentTerm) != 0 || phase[PH_OFFERING] != ST_ENABLED)
            {
                printf("A removal request must be sent before the end of the offering stage.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            if (request_count >= 300)
            {
                printf("The request list is full.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            {
                Request *r = &requests[request_count++];
                memset(r, 0, sizeof *r);
                r->type = REQ_REMOVE;
                copy_str(r->courseCode, sizeof r->courseCode, offerings[oi].courseCode);
                copy_str(r->professorId, sizeof r->professorId, professors[p].username);
                copy_str(r->term, sizeof r->term, offerings[oi].term);
                r->status = REQ_PENDING;
            }
            save_requests();
            printf("Sent request to admin.\n");
            press_any_key("Press any key to go back...");
            return;
        }
        else if (opt == 4)
        {
            faculty_publish(p, oi, 0);
        }
        else if (opt == 5)
        {
            faculty_publish(p, oi, 1);
        }
        else
        {
            return;
        }
    }
}

static void faculty_my_offerings(int p)
{
    for (;;)
    {
        int i, j, opt;
        clear_screen();
        listed_count = 0;
        for (i = 0; i < offering_count; i++)
            if (strcmp(offerings[i].professorId, professors[p].username) == 0)
                listed[listed_count++] = i;
        for (i = 0; i < listed_count; i++)
            for (j = i + 1; j < listed_count; j++)
                if (strcmp(offerings[listed[j]].term, offerings[listed[i]].term) > 0)
                {
                    int t = listed[i];
                    listed[i] = listed[j];
                    listed[j] = t;
                }

        printf("Faculty: My offerings\n");
        printf("List of my offerings\n");
        print_offering_header(0);
        for (i = 0; i < listed_count; i++)
            print_offering_row(i + 1, &offerings[listed[i]], 0);
        if (listed_count == 0)
            printf("(you have no offering yet)\n");

        printf("\n1. Go to offering\n2. Search\n3. Go back\nEnter an option: ");
        opt = read_int();
        if (opt == 1)
        {
            int num;
            printf("Enter offering number: ");
            num = read_int();
            if (num < 1 || num > listed_count)
            {
                printf("Invalid offering number.\n");
                press_any_key("Press any key to go back...");
                continue;
            }
            faculty_offering_menu(p, listed[num - 1]);
        }
        else if (opt == 2)
        {
            search_offerings(NULL, 0);
        }
        else
            return;
    }
}

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static double quantile(const int *v, int n, double p)
{
    double pos = p * (n - 1);
    int lo = (int)floor(pos), hi = (int)ceil(pos);
    if (hi >= n)
        hi = n - 1;
    if (lo == hi)
        return v[lo];
    return v[lo] + (pos - lo) * (v[hi] - v[lo]);
}

static void faculty_survey_results(int p)
{
    int i, j, n;
    int v[300];
    char seen[300][64];
    int seenCount = 0;

    clear_screen();
    printf("Faculty: Survey results\n");
    for (i = 0; i < offering_count; i++)
    {
        double sum = 0, mean, var = 0;
        int hist[11];
        if (strcmp(offerings[i].professorId, professors[p].username) != 0)
            continue;
        { 
            char key[64];
            int dup = 0;
            snprintf(key, sizeof key, "%.31s@%.15s", offerings[i].courseCode, offerings[i].term);
            for (j = 0; j < seenCount; j++)
                if (strcmp(seen[j], key) == 0)
                    dup = 1;
            if (dup)
                continue;
            if (seenCount < 300)
                copy_str(seen[seenCount++], 64, key);
        }
        n = 0;
        for (j = 0; j < survey_count; j++)
            if (strcmp(surveys[j].professorId, professors[p].username) == 0 &&
                strcmp(surveys[j].courseCode, offerings[i].courseCode) == 0 &&
                strcmp(surveys[j].term, offerings[i].term) == 0)
                v[n++] = surveys[j].score;

        printf("\n%s (%s) - semester %s\n",
               course_name(offerings[i].courseCode), offerings[i].courseCode, offerings[i].term);
        if (n == 0)
        {
            printf("  no survey has been submitted yet\n");
            continue;
        }

        qsort(v, n, sizeof(int), cmp_int);
        for (j = 0; j < n; j++)
            sum += v[j];
        mean = sum / n;
        for (j = 0; j < n; j++)
            var += (v[j] - mean) * (v[j] - mean);
        var /= n;
        printf("  responses: %d\n", n);
        printf("  mean: %.2f\n", mean);
        printf("  standard deviation: %.2f\n", sqrt(var));
        printf("  Q1: %.2f   Q2 (median): %.2f   Q3: %.2f\n",
               quantile(v, n, 0.25), quantile(v, n, 0.50), quantile(v, n, 0.75));
        for (j = 0; j <= 10; j++)
            hist[j] = 0;
        for (j = 0; j < n; j++)
            if (v[j] >= 1 && v[j] <= 10)
                hist[v[j]]++;
        printf("  chart:\n");
        for (j = 1; j <= 10; j++)
        {
            int k;
            printf("  %2d | ", j);
            for (k = 0; k < hist[j]; k++)
                printf("#");
            printf(" %d\n", hist[j]);
        }
    }
    press_any_key("\nPress any key to go back...");
}

static void student_offerings(int s);
static void student_report_card(int s);
static void student_survey(int s);
static void student_lms(int s);

static void student_dashboard(int s)
{
    for (;;)
    {
        int opt;
        clear_screen();
        printf("Welcome %s %s\n", students[s].firstName, students[s].lastName);
        printf("1. Offerings\n");
        printf("2. Courses\n");
        printf("3. Report card\n");
        printf("4. Course survey\n");
        printf("5. Assignments & exams\n");
        printf("6. Log out\n");
        printf("Enter an option: ");
        opt = read_int();

        if (opt == 1)
            student_offerings(s);
        else if (opt == 2)
        {
            int sub;
            clear_screen();
            list_all_courses();
            printf("\n1. Search\n2. Go back\nEnter an option: ");
            sub = read_int();
            if (sub == 1)
                search_courses();
        }
        else if (opt == 3)
            student_report_card(s);
        else if (opt == 4)
            student_survey(s);
        else if (opt == 5)
            student_lms(s);
        else if (opt == 6)
        {
            save_all();
            return;
        }
    }
}

static bool is_thesis_course(const Course *c)
{
    return ci_contains(c->name, "thesis");
}

static void student_enroll(int s)
{
    int num, oi, ci;
    char missing[256];

    printf("Enter offering number: ");
    num = read_int();
    if (num < 1 || num > listed_count)
    {
        printf("Invalid offering number.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    oi = listed[num - 1];

    if (phase[PH_SELECTION] != ST_ENABLED || strcmp(offerings[oi].term, currentTerm) != 0)
    {
        printf("The unit selection stage is not active at the moment.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (find_enrollment(&students[s], offerings[oi].term, offerings[oi].courseCode) >= 0)
    {
        printf("You are already enrolled in this offering.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (offerings[oi].enrolledCount >= offerings[oi].capacity)
    {
        printf("This offering has no free capacity.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    ci = find_course(offerings[oi].courseCode);
    if (ci < 0)
    {
        printf("The course of this offering no longer exists.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (strcmp(courses[ci].degree, students[s].degree) != 0)
    {
        printf("This course is offered for %s students only.\n", courses[ci].degree);
        press_any_key("Press any key to go back...");
        return;
    }
    if (is_thesis_course(&courses[ci]))
    {
        char sup[96];
        prof_full_name(offerings[oi].professorId, sup, sizeof sup);
        if (strcmp(students[s].degree, "PhD") != 0)
        {
            printf("Only PhD students can enroll in a thesis.\n");
            press_any_key("Press any key to go back...");
            return;
        }
        {
            int pi = find_prof(offerings[oi].professorId);
            char full[128];
            if (pi >= 0)
                snprintf(full, sizeof full, "%s %s", professors[pi].firstName, professors[pi].lastName);
            else
                copy_str(full, sizeof full, offerings[oi].professorId);
            if (!ci_equal(full, students[s].supervisor))
            {
                printf("You can only take your thesis with your own supervisor (%s).\n",
                       students[s].supervisor);
                press_any_key("Press any key to go back...");
                return;
            }
        }
    }
    if (!prerequisites_ok(&students[s], &courses[ci], missing, sizeof missing))
    {
        printf("You have not passed the prerequisite(s): %s\n", missing);
        press_any_key("Press any key to go back...");
        return;
    }
    if (students[s].enrollCount >= 300)
    {
        printf("You have reached the maximum number of enrollments.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    {
        Enrollment *e = &students[s].enroll[students[s].enrollCount++];
        copy_str(e->term, sizeof e->term, offerings[oi].term);
        copy_str(e->courseCode, sizeof e->courseCode, offerings[oi].courseCode);
        e->grade = -1.0;
    }
    offerings[oi].enrolledCount++;
    save_students();
    save_offerings();
    printf("You enrolled in %s successfully.\n", course_name(offerings[oi].courseCode));
    press_any_key("Press any key to go back...");
}

static void student_withdraw(int s)
{
    int num, oi, k, j;
    printf("Enter offering number: ");
    num = read_int();
    if (num < 1 || num > listed_count)
    {
        printf("Invalid offering number.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    oi = listed[num - 1];
    if (phase[PH_SELECTION] != ST_ENABLED || strcmp(offerings[oi].term, currentTerm) != 0)
    {
        printf("The unit selection stage is not active at the moment.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    k = find_enrollment(&students[s], offerings[oi].term, offerings[oi].courseCode);
    if (k < 0)
    {
        printf("You are not enrolled in this offering.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    for (j = k; j < students[s].enrollCount - 1; j++)
        students[s].enroll[j] = students[s].enroll[j + 1];
    students[s].enrollCount--;
    if (offerings[oi].enrolledCount > 0)
        offerings[oi].enrolledCount--;
    save_students();
    save_offerings();
    printf("You withdrew from %s successfully.\n", course_name(offerings[oi].courseCode));
    press_any_key("Press any key to go back...");
}

static void student_offerings(int s)
{
    char term[16];
    clear_screen();
    printf("Student: Offerings\n");
    printf("Enter semester number: ");
    read_line(term, sizeof term);
    trim(term);
    if (term[0] == '\0')
        copy_str(term, sizeof term, currentTerm);

    for (;;)
    {
        int opt;
        clear_screen();
        printf("Student: Offerings\n");
        list_offerings_of_term(term, 1);
        printf("\n1. Search\n2. Enroll in course\n3. Withdraw course\n4. Go back\n");
        printf("Enter an option: ");
        opt = read_int();
        if (opt == 1)
            search_offerings(term, 1);
        else if (opt == 2)
            student_enroll(s);
        else if (opt == 3)
            student_withdraw(s);
        else
            return;
    }
}

static void print_semester_report(const Student *s, const char *term)
{
    int i, enrolled = 0, passed = 0, failed = 0;
    double gpa;

    printf("Report card - %s %s - %s\n", s->firstName, s->lastName, term);
    printf("| %-28s | %-9s | %-5s | %-6s | %-6s | %-22s |\n",
           "course name", "course id", "units", "grade", "passed", "instructor's name");
    printf("|------------------------------|-----------|-------|--------|--------|"
           "------------------------|\n");
    for (i = 0; i < s->enrollCount; i++)
    {
        char inst[96];
        int oi;
        if (strcmp(s->enroll[i].term, term) != 0)
            continue;
        oi = find_offering(term, s->enroll[i].courseCode);
        if (oi >= 0)
            prof_full_name(offerings[oi].professorId, inst, sizeof inst);
        else
            copy_str(inst, sizeof inst, "-");
        printf("| %-28s | %-9s | %-5d | ",
               course_name(s->enroll[i].courseCode), s->enroll[i].courseCode,
               course_credits(s->enroll[i].courseCode));
        if (s->enroll[i].grade < 0)
            printf("%-6s | %-6s | ", "-", "-");
        else
            printf("%-6.2f | %-6s | ", s->enroll[i].grade,
                   s->enroll[i].grade >= 10.0 ? "Yes" : "No");
        printf("%-22s |\n", inst);
    }
    gpa = term_gpa(s, term, &enrolled, &passed, &failed);
    if (enrolled == 0)
        printf("(you have no course in this semester)\n");
    printf("Enrolled courses: %d\n", enrolled);
    printf("Passed courses: %d\n", passed);
    printf("Failed courses: %d\n", failed);
    printf("GPA: %.2f\n", gpa);
}


static void search_passed_courses(int s)
{
    int opt, i, found = 0;
    char phrase[128];
    const Student *st = &students[s];

    printf("\nSearch in passed courses:\n");
    printf("1. Search by course name\n");
    printf("2. Search by course id\n");
    printf("3. Search by semester\n");
    printf("4. Show all passed courses\n");
    printf("5. Go back\n");
    printf("Enter an option: ");
    opt = read_int();
    if (opt < 1 || opt > 4)
        return;
    if (opt != 4)
    {
        printf("The phrase to search: ");
        read_line(phrase, sizeof phrase);
    }
    else
        phrase[0] = '\0';

    printf("\n| %-28s | %-9s | %-5s | %-8s | %-6s |\n",
           "course name", "course id", "units", "semester", "grade");
    printf("|------------------------------|-----------|-------|----------|--------|\n");
    for (i = 0; i < st->enrollCount; i++)
    {
        const Enrollment *e = &st->enroll[i];
        bool hit;
        if (e->grade < 10.0)
            continue; 
        switch (opt)
        {
        case 1:
            hit = ci_contains(course_name(e->courseCode), phrase);
            break;
        case 2:
            hit = ci_contains(e->courseCode, phrase);
            break;
        case 3:
            hit = ci_contains(e->term, phrase);
            break;
        default:
            hit = true;
            break;
        }
        if (!hit)
            continue;
        printf("| %-28s | %-9s | %-5d | %-8s | %-6.2f |\n",
               course_name(e->courseCode), e->courseCode,
               course_credits(e->courseCode), e->term, e->grade);
        found++;
    }
    if (!found)
        printf("No matching passed course found.\n");
    press_any_key("\nPress any key to go back...");
}

static void student_report_card(int s)
{
    for (;;)
    {
        int opt;
        const Student *st = &students[s];
        clear_screen();
        printf("Student: Report Card\n");
        printf("| %-14s | %-24s |\n", "student id", st->username);
        printf("| %-14s | %-24s |\n", "first name", st->firstName);
        printf("| %-14s | %-24s |\n", "last name", st->lastName);
        printf("| %-14s | %-24s |\n", "national code", st->nationalId);
        printf("| %-14s | %-24s |\n", "field", st->major);
        printf("| %-14s | %-24d |\n", "entrance year", st->entryYear);
        printf("| %-14s | %-24s |\n", "section", st->degree);
        printf("| %-14s | %-24s |\n", "mentor", st->supervisor);
        printf("| %-14s | %-24s |\n", "department", st->faculty);
        printf("| %-14s | %-24.2f |\n", "GPA", total_gpa(st));
        if (strcmp(st->degree, "PhD") == 0 && st->thesisTitle[0])
        {
            printf("| %-14s | %-24s |\n", "thesis", st->thesisTitle);
            printf("| %-14s | %-24d |\n", "citations", st->thesisCitations);
        }
        printf("\n1. Go to semester\n2. Search in passed courses\n3. Go back\nEnter an option: ");
        opt = read_int();
        if (opt == 2)
        {
            search_passed_courses(s);
            continue;
        }
        if (opt != 1)
            return;
        {
            char term[16];
            printf("Enter semester number: ");
            read_line(term, sizeof term);
            trim(term);
            printf("\n");
            print_semester_report(st, term);
            press_any_key("Pres any key to go back...");
        }
    }
}

static void student_survey(int s)
{
    int i, n = 0, num, score;
    int map[300];
    char term[16];

    clear_screen();
    printf("Student: Course survey\n");
    printf("Enter semester number: ");
    read_line(term, sizeof term);
    trim(term);
    if (term[0] == '\0')
        copy_str(term, sizeof term, currentTerm);

    if (strcmp(term, currentTerm) == 0 && phase[PH_GRADE] != ST_FINISHED)
    {
        printf("The survey opens after the grade recording stage of the semester is finished.\n");
        press_any_key("Press any key to go back...");
        return;
    }

    printf("\nYour courses in semester %s:\n", term);
    for (i = 0; i < students[s].enrollCount; i++)
    {
        int done = 0, j, oi;
        if (strcmp(students[s].enroll[i].term, term) != 0)
            continue;
        for (j = 0; j < survey_count; j++)
            if (strcmp(surveys[j].studentId, students[s].username) == 0 &&
                strcmp(surveys[j].term, term) == 0 &&
                strcmp(surveys[j].courseCode, students[s].enroll[i].courseCode) == 0)
                done = 1;
        oi = find_offering(term, students[s].enroll[i].courseCode);
        map[n] = i;
        printf("%d. %s (%s)%s\n", n + 1,
               course_name(students[s].enroll[i].courseCode),
               students[s].enroll[i].courseCode,
               done ? "  [already submitted]" : "");
        (void)oi;
        n++;
    }
    if (n == 0)
    {
        printf("(you have no course in this semester)\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("\nEnter course number (0 to go back): ");
    num = read_int();
    if (num < 1 || num > n)
        return;

    for (i = 0; i < survey_count; i++)
        if (strcmp(surveys[i].studentId, students[s].username) == 0 &&
            strcmp(surveys[i].term, term) == 0 &&
            strcmp(surveys[i].courseCode, students[s].enroll[map[num - 1]].courseCode) == 0)
        {
            printf("You have already submitted a survey for this course.\n");
            press_any_key("Press any key to go back...");
            return;
        }

    printf("Enter your score (1-10): ");
    score = read_int();
    if (score < 1 || score > 10)
    {
        printf("The score must be between 1 and 10.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    if (survey_count >= 2000)
    {
        printf("The survey list is full.\n");
        press_any_key("Press any key to go back...");
        return;
    }
    {
        Survey *sv = &surveys[survey_count++];
        int oi = find_offering(term, students[s].enroll[map[num - 1]].courseCode);
        memset(sv, 0, sizeof *sv);
        copy_str(sv->term, sizeof sv->term, term);
        copy_str(sv->courseCode, sizeof sv->courseCode, students[s].enroll[map[num - 1]].courseCode);
        copy_str(sv->professorId, sizeof sv->professorId, oi >= 0 ? offerings[oi].professorId : "-");
        copy_str(sv->studentId, sizeof sv->studentId, students[s].username);
        sv->score = score;
    }
    save_surveys();
    printf("Your survey has been submitted. Thank you.\n");
    press_any_key("Press any key to go back...");
}


static void student_lms(int s)
{
    int i, j, n = 0, num;
    int map[300];

    clear_screen();
    printf("Student: Assignments & exams - semester %s\n", currentTerm);
    for (i = 0; i < assign_count; i++)
    {
        int done = 0;
        if (strcmp(assigns[i].term, currentTerm) != 0)
            continue;
        if (find_enrollment(&students[s], assigns[i].term, assigns[i].courseCode) < 0)
            continue;
        for (j = 0; j < sub_count; j++)
            if (subs[j].assignmentId == assigns[i].id &&
                strcmp(subs[j].studentId, students[s].username) == 0)
                done = 1;
        map[n] = i;
        printf("%d. [%s] %s - %s (%s) - score: %.2f%s\n", n + 1,
               assigns[i].kind == 0 ? "homework" : "exam",
               assigns[i].title, course_name(assigns[i].courseCode), assigns[i].courseCode,
               assigns[i].score, done ? "  [answered]" : "");
        n++;
    }
    if (n == 0)
    {
        printf("(there is nothing published for your courses)\n");
        press_any_key("Press any key to go back...");
        return;
    }
    printf("\nEnter the number to answer (0 to go back): ");
    num = read_int();
    if (num < 1 || num > n)
        return;

    {
        Assignment *a = &assigns[map[num - 1]];
        Submission *sb;
        double earned = 0;
        int mcCount = 0, correct = 0;

        for (j = 0; j < sub_count; j++)
            if (subs[j].assignmentId == a->id && strcmp(subs[j].studentId, students[s].username) == 0)
            {
                printf("You have already answered this one.\n");
                press_any_key("Press any key to go back...");
                return;
            }
        if (sub_count >= 2000)
        {
            printf("The submission list is full.\n");
            press_any_key("Press any key to go back...");
            return;
        }
        sb = &subs[sub_count];
        memset(sb, 0, sizeof *sb);
        sb->assignmentId = a->id;
        copy_str(sb->studentId, sizeof sb->studentId, students[s].username);

        for (i = 0; i < a->qcount; i++)
        {
            printf("\nQ%d. %s\n", i + 1, a->q[i].text);
            if (a->q[i].type == 0)
            {
                int k, ans;
                for (k = 0; k < 4; k++)
                    printf("   %d) %s\n", k + 1, a->q[i].options[k]);
                printf("Your answer (1-4): ");
                ans = read_int();
                if (ans < 1 || ans > 4)
                    ans = 1;
                sb->answers[i] = ans - 1;
                mcCount++;
                if (ans - 1 == a->q[i].correct)
                    correct++;
            }
            else
            {
                printf("Your answer: ");
                sb->answers[i] = -1;
                read_line(sb->text[i], sizeof sb->text[i]);
            }
            sb->acount++;
        }
        if (mcCount > 0)
            earned = a->score * ((double)correct / a->qcount);
        sb->earned = earned;
        sub_count++;
        save_lms();
        printf("\nYour answers have been submitted.\n");
        if (mcCount == a->qcount)
            printf("Auto-corrected score: %.2f of %.2f\n", earned, a->score);
        else
            printf("The descriptive parts will be graded by the instructor.\n");
        press_any_key("Press any key to go back...");
    }
}
